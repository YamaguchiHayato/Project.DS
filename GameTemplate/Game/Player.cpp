#include "stdafx.h"
#include "Player.h"
#include "LocalPlayerController.h"
#include "Weapon.h"
#include "Tracer.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"

namespace
{
	const char* sPlayerModelPath_ = "Assets/modelData/unityChan.tkm";	//! 仮プレイヤーモデル。

	/* 基本アニメーションのファイルパス。EnPlayerAnimationの並び順と一致させること。*/
	const char* sAnimationPathList_[] =
	{
		"Assets/animData/idle.tka",	//! Idle。
		"Assets/animData/walk.tka",	//! Walk。
		"Assets/animData/run.tka",	//! Run。
	};

	const float	kMuzzleForward = 50.0f;		//! 銃口の前方オフセット(照準方向)。
	const float	kMuzzleHeight = 150.0f;		//! 銃口の高さ(目線付近から撃つ)。
	const float	kAnimInterpolateTime = 0.2f;	//! アニメーション補間時間(秒)。

	const float	kEyeHeight = 160.0f;			//! 目(カメラ)の高さ。DebugPlayerScene側の kEyeHeight_ と合わせる。
	const float	kViewModelRight = 22.0f;		//! ビューモデル銃の右オフセット。
	const float	kViewModelDown = -26.0f;		//! ビューモデル銃の下オフセット(負で下)。
	const float	kTargetGunSize = 45.0f;			//! ビューモデル銃の目標サイズ(一番長い辺をこの長さに自動スケール)。
	const float	kWeaponRange = 3000.0f;			//! 射程(ヒットスキャンのレイ・トレーサーの長さ)。
}

namespace nsApp
{
	namespace nsActor
	{
		Player::Player()
		{}

		Player::~Player()
		{
			/* コントローラを破棄する。*/
			delete pController_;
			pController_ = nullptr;
		}


		bool Player::Start()
		{
			/* 操作意図の供給源を作る(ローカル実機。将来ここをネット受信用に差し替えられる)。*/
			pController_ = new LocalPlayerController();

			/* モデルとアニメーションを読み込む。*/
			InitModel();

			/* テスト用のHPを設定する。*/
			stCharacterStatus_.stHp_.iCurrentHP_ = 100;
			stCharacterStatus_.stHp_.iMaxHP_ = 100;

			/* 初期所持武器を登録する。マウスホイールで順番に切り替わる。*/
			stWeaponInventory_.AddWeapon(nsWeapon::EnWeaponType::Handgun);
			stWeaponInventory_.AddWeapon(nsWeapon::EnWeaponType::AssaultRifle);

			return true;
		}


		void Player::Update()
		{
			/* ICharacter(Actor)の共通更新処理(ステートマシン更新等)。*/
			ICharacter::Update();

			/* 1フレームの経過時間(秒)。*/
			const float fDeltaTime = g_gameTime->GetFrameDeltaTime();

			/* 1.操作意図を取得する(ローカル入力 or ネット受信をコントローラが吸収する)。*/
			pController_->PollIntent(stIntent_);

			/* 2.入力結果を使って移動する。*/
			UpdateMove(fDeltaTime);

			/* 3.入力結果を使って武器の更新・発射・切り替えを行う。*/
			UpdateWeapon(fDeltaTime);

			/* 4.入力結果を使ってインタラクト・ライト等その他の操作を行う。*/
			UpdateAction();

			/* 5.移動状態をモデル(位置・回転・アニメーション)へ反映する。*/
			UpdateModel();
		}


		void Player::Render(RenderContext& rc)
		{
			/*
			 * FPS(一人称)なので、自分の体モデルは描画しない(カメラが頭の位置にあり視界を塞ぐため)。
			 * TODO: 他プレイヤーからは3人称で見える必要があるので、将来「自分視点か否か」で
			 *       描画を切り替える(今はデバッグ用に一人称固定)。
			 */
			/* stModelRender_.Draw(rc); */

			/* 現在装備中の武器モデル(ビューモデル)だけ描画する。*/
			const int iType = static_cast<int>(enEquippedType_);
			if (aWeaponModelLoaded_[iType])
				aWeaponModels_[iType].Draw(rc);

			/* TODO: マズルフラッシュ等、武器に関する描画があればここに追加する。*/
		}


		void Player::InitModel()
		{
			/* 基本アニメーションを読み込み、ループ設定する。*/
			const int iNumAnimation = static_cast<int>(EnPlayerAnimation::Num);
			for (int i = 0; i < iNumAnimation; i++)
			{
				aAnimationClip_[i].Load(sAnimationPathList_[i]);
				aAnimationClip_[i].SetLoopFlag(true);
			}

			/* モデルをアニメーション付きで初期化する。*/
			stModelRender_.Init(sPlayerModelPath_, aAnimationClip_, iNumAnimation, enModelUpAxisY);
			stModelRender_.SetPosition(vPosition_);
			stModelRender_.Update();

			/* 最初は待機アニメーションを再生する。*/
			PlayAnimation(EnPlayerAnimation::Idle);
		}


		void Player::UpdateMove(float fDeltaTime)
		{
			/*
			 * マウスの横移動量でカメラの旋回角を更新する(原神風の旋回カメラ)。
			 * PlayerInput側でカーソルをウィンドウ中央にロックしているので、
			 * 端で止まらず無限に旋回できる(GetMouseDeltaXは1フレームの移動量)。
			 */
			fCameraYaw_ += stIntent_.fLookYawDelta_;

			/* カメラの旋回角から、地面基準の前方・右方向ベクトルを作る。*/
			const Vector3 vCameraForward = { sinf(fCameraYaw_), 0.0f, cosf(fCameraYaw_) };
			const Vector3 vCameraRight = { vCameraForward.z, 0.0f, -vCameraForward.x };

			const Vector3& vMoveAxis = stIntent_.vMoveAxis_;

			/* 移動入力が無ければ座標も向きも変えない(向きは維持してアイドルへ)。*/
			if (vMoveAxis.x == 0.0f && vMoveAxis.z == 0.0f)
			{
				bIsMoving_ = false;
				return;
			}

			/* カメラ相対の移動方向を作る(W=カメラ奥, S=手前, A/D=左右)。*/
			Vector3 vMoveDir = vCameraForward * vMoveAxis.z + vCameraRight * vMoveAxis.x;
			vMoveDir.y = 0.0f;
			vMoveDir.Normalize();

			/* 移動方向へ進む。*/
			vPosition_ += vMoveDir * fMoveSpeed_ * fDeltaTime;

			/*
			 * 原神風:常に進行方向を向いて前歩きする(Sで下がっても振り向くので後ろ歩きにならない)。
			 * TODO: 今は瞬時に向きが変わる。滑らかにするなら現在の向きから目標の向きへ補間する。
			 */
			vForward_ = vMoveDir;
			qRotation_.SetRotationY(atan2f(vMoveDir.x, vMoveDir.z));

			bIsMoving_ = true;
		}


		void Player::UpdateWeapon(float fDeltaTime)
		{
			/* 武器のクールタイム・リロード等を進める。*/
			stWeaponInventory_.Update(fDeltaTime);

			/* リロード。*/
			if (stIntent_.bReloadTrigger_)
				stWeaponInventory_.Reload();

			/* 照準方向はカメラの前方(一人称なので常に画面中央=クロスヘア方向)。*/
			const Vector3 vAimDir = { sinf(fCameraYaw_), 0.0f, cosf(fCameraYaw_) };

			/* 現在の武器に応じて、発射入力を押しっぱなし(フルオート)か押した瞬間(単発)で判定する。*/
			nsWeapon::Weapon* pWeapon = stWeaponInventory_.GetCurrentWeapon();
			if (pWeapon != nullptr)
			{
				const bool bWantFire = pWeapon->IsFullAuto()
					? stIntent_.bFirePress_
					: stIntent_.bFireTrigger_;

				if (bWantFire)
				{
					/* 銃口位置はプレイヤーの少し前・胸の高さを仮に使う。*/
					Vector3 vMuzzlePos = vPosition_;
					vMuzzlePos += vAimDir * kMuzzleForward;
					vMuzzlePos.y += kMuzzleHeight;

					/* 発射に成功したら、ヒットスキャン判定＋トレーサー表示を行う。*/
					if (stWeaponInventory_.Fire(vMuzzlePos, vAimDir))
					{
						/* レイの終点(最大射程)。命中したらここを命中点に置き換える。*/
						Vector3 vRayEnd = vMuzzlePos + vAimDir * kWeaponRange;

						/*
						 * ヒットスキャン命中判定: レイ(銃口→射程)に対し、敵を球とみなして
						 * 最も手前で交差する1体を探す。コライダー未整備のため自前の レイvs球。
						 * (将来コライダーが付いたら PhysicsWorld::RayTest へ置換してよい)
						 */
						const float kEnemyHitRadius = 40.0f;	// 敵の被弾半径(暫定)。
						nsActor::CommonEnemy* pHitEnemy = nullptr;
						float fNearest = kWeaponRange;			// 命中点までの前方距離(近いほど手前)。
						for (nsActor::CommonEnemy* pEnemy : FindGOs<nsActor::CommonEnemy>("commonEnemy"))
						{
							if (pEnemy == nullptr)
								continue;

							/* 狙点は敵の胴(足元＋銃口高さ)あたりに寄せる。*/
							Vector3 vCenter = pEnemy->GetPosition();
							vCenter.y += kMuzzleHeight;

							/* レイ方向への射影(=前方距離)。後方や、既存の命中より奥なら除外。*/
							const Vector3 vToCenter = vCenter - vMuzzlePos;
							const float fForward = vToCenter.Dot(vAimDir);
							if (fForward < 0.0f || fForward > fNearest)
								continue;

							/* レイ上の最近点との距離が被弾半径以内なら命中とみなす。*/
							const Vector3 vClosest = vMuzzlePos + vAimDir * fForward;
							if ((vCenter - vClosest).Length() <= kEnemyHitRadius)
							{
								fNearest = fForward;
								pHitEnemy = pEnemy;
							}
						}

						/* 命中していたら、トレーサーを命中点で止めてダメージを与える。*/
						if (pHitEnemy != nullptr)
						{
							vRayEnd = vMuzzlePos + vAimDir * fNearest;
							pHitEnemy->ApplyDamage(pWeapon->GetAttackPower());

							/* 倒したら退場させる(暫定。将来はDirector管理＋撃破エフェクト)。*/
							if (pHitEnemy->IsDead())
								DeleteGO(pHitEnemy);
						}

						/* 見せるためのトレーサー(曳光弾)を一瞬だけ表示する。*/
						nsWeapon::Tracer* pTracer = NewGO<nsWeapon::Tracer>(0, "tracer");
						pTracer->Setup(vMuzzlePos, vRayEnd);
					}
				}
			}

			/* 武器切り替え(マウスホイール)。*/
			if (stIntent_.bWeaponNextTrigger_)
				stWeaponInventory_.SwitchNext();
			else if (stIntent_.bWeaponPrevTrigger_)
				stWeaponInventory_.SwitchPrev();
		}


		void Player::UpdateAction()
		{
			/* インタラクト。*/
			if (stIntent_.bUseTrigger_)
			{
				/* TODO: 近くのインタラクト可能なオブジェクトを探して処理する。*/
				OutputDebugStringA("[Player] Interact!\n");
			}

			/* ライトのON/OFF切り替え。*/
			if (stIntent_.bLightTrigger_)
			{
				bIsLightOn_ = !bIsLightOn_;
				/* TODO: 実際のライト(スポットライト等)のON/OFF処理に差し替える。*/
			}

			/* メニュー・ポーズ画面。*/
			if (stIntent_.bPauseTrigger_)
			{
				/* TODO: ポーズ画面/メニューを開く処理に差し替える。*/
				OutputDebugStringA("[Player] Pause!\n");
			}
		}


		void Player::UpdateModel()
		{
			/* 移動状態に応じてアニメーションを切り替える。*/
			PlayAnimation(bIsMoving_ ? EnPlayerAnimation::Walk : EnPlayerAnimation::Idle);

			/* 位置と回転をモデルへ反映して更新する。*/
			stModelRender_.SetPosition(vPosition_);
			stModelRender_.SetRotation(qRotation_);
			stModelRender_.Update();

			/* 手に持つ武器モデルを更新する。*/
			UpdateWeaponModel();
		}


		void Player::UpdateWeaponModel()
		{
			nsWeapon::Weapon* pWeapon = stWeaponInventory_.GetCurrentWeapon();
			if (pWeapon == nullptr)
				return;

			enEquippedType_ = pWeapon->GetType();
			const int iType = static_cast<int>(enEquippedType_);

			/* この武器のモデルをまだ読み込んでいなければ一度だけ読み込む(再Initは壊れるので避ける)。*/
			if (!aWeaponModelLoaded_[iType])
			{
				/* TODO: 銃モデルが横倒し等になる場合は enModelUpAxisY に変える。*/
				aWeaponModels_[iType].Init(pWeapon->GetModelPath(), nullptr, 0, enModelUpAxisZ);
				aWeaponModelLoaded_[iType] = true;

				/* 読み込み時に一度だけ、モデルの中心と自動サイズ合わせスケールを計算しておく。*/
				CalcWeaponModelFit(iType);
			}

			/* FPSのビューモデルとして、カメラ(目)基準で画面手前(前・右・下)に銃を置く。*/
			ModelRender& weaponModel = aWeaponModels_[iType];
			const Vector3 vLook = { sinf(fCameraYaw_), 0.0f, cosf(fCameraYaw_) };
			const Vector3 vRight = { vLook.z, 0.0f, -vLook.x };

			/* 最終スケール = 自動サイズ合わせ × 武器データの微調整倍率(1.0基準)。*/
			const float fScale = aWeaponModelAutoScale_[iType] * pWeapon->GetModelScale();

			/* 銃の「中心」を置きたい位置(目の前・右・下)。*/
			Vector3 vGunPos = vPosition_;
			vGunPos.y += kEyeHeight;						// 目の高さ
			vGunPos += vLook * pWeapon->GetViewModelForward();	// 前(武器ごと)
			vGunPos += vRight * kViewModelRight;			// 右
			vGunPos.y += kViewModelDown;					// 下(負)

			/*
			 * モデル原点のズレを打ち消す。モデルはヨー回転のみなので、ローカル中心を
			 * 世界の基底(右=X, 上=Y, 前=Z)に写して vGunPos から引けば、中心がぴったり vGunPos に来る。
			 * これで原点がズレたモデルでもカメラにめり込まなくなる。
			 */
			const Vector3& vCenter = aWeaponModelCenter_[iType];
			Vector3 vCenterOffset =
				  vRight * (vCenter.x * fScale)
				+ Vector3(0.0f, vCenter.y * fScale, 0.0f)
				+ vLook  * (vCenter.z * fScale);

			weaponModel.SetPosition(vGunPos - vCenterOffset);
			Quaternion qGun;
			qGun.SetRotationY(atan2f(vLook.x, vLook.z));
			weaponModel.SetRotation(qGun);
			weaponModel.SetScale(Vector3(fScale, fScale, fScale));
			weaponModel.Update();
		}


		void Player::CalcWeaponModelFit(int iType)
		{
			/* 全メッシュの全頂点を走査してローカルAABB(最小・最大)を求める。*/
			Vector3 vMin = { 1e30f, 1e30f, 1e30f };
			Vector3 vMax = { -1e30f, -1e30f, -1e30f };

			aWeaponModels_[iType].GetModel().GetTkmFile().QueryMeshParts(
				[&](const TkmFile::SMesh& mesh)
				{
					for (const auto& vertex : mesh.vertexBuffer)
					{
						const Vector3& p = vertex.pos;
						if (p.x < vMin.x) vMin.x = p.x;
						if (p.y < vMin.y) vMin.y = p.y;
						if (p.z < vMin.z) vMin.z = p.z;
						if (p.x > vMax.x) vMax.x = p.x;
						if (p.y > vMax.y) vMax.y = p.y;
						if (p.z > vMax.z) vMax.z = p.z;
					}
				});

			/* 頂点が取れなかった場合は無補正(中心0・スケール1)にする。*/
			if (vMax.x < vMin.x)
			{
				aWeaponModelCenter_[iType] = Vector3(0.0f, 0.0f, 0.0f);
				aWeaponModelAutoScale_[iType] = 1.0f;
				return;
			}

			/* 中心 = (最小+最大)/2。*/
			Vector3 vCenter = vMin + vMax;
			vCenter *= 0.5f;
			aWeaponModelCenter_[iType] = vCenter;

			/* 一番長い辺を kTargetGunSize に合わせる自動スケール。*/
			Vector3 vSize = vMax - vMin;
			float fLongest = vSize.x;
			if (vSize.y > fLongest) fLongest = vSize.y;
			if (vSize.z > fLongest) fLongest = vSize.z;
			aWeaponModelAutoScale_[iType] = (fLongest > 0.0001f) ? (kTargetGunSize / fLongest) : 1.0f;

			/* デバッグ: AABBに余分なジオメトリが混じっていないか確認する。*/
			DebugPrintW(L"[Player] weapon%d AABB min(%.1f,%.1f,%.1f) max(%.1f,%.1f,%.1f) size(%.1f,%.1f,%.1f) center(%.1f,%.1f,%.1f) autoScale=%.4f\n",
				iType, vMin.x, vMin.y, vMin.z, vMax.x, vMax.y, vMax.z,
				vSize.x, vSize.y, vSize.z, vCenter.x, vCenter.y, vCenter.z, aWeaponModelAutoScale_[iType]);
		}


		void Player::PlayAnimation(EnPlayerAnimation enAnimation)
		{
			/* 同じアニメーションが指定されたら再生し直さない。*/
			if (enPlayingAnimation_ == enAnimation)
				return;

			enPlayingAnimation_ = enAnimation;
			stModelRender_.PlayAnimation(static_cast<int>(enAnimation), kAnimInterpolateTime);
		}
	}
}
