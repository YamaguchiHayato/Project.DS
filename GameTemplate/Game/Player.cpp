#include "stdafx.h"
#include "Player.h"
#include "LocalPlayerController.h"
#include "Weapon.h"
#include "Tracer.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Event/EventBus.h"
#include "Src/Effect/HitEffect.h"
#include "Src/System/GamePause.h"
#include "Src/Item/Grenade.h"

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
	const float	kMuzzleRight = 40.0f;		//! トレーサー始点を目線から右へずらす量(線を斜めに見せる)。
	const float	kMuzzleDown = -32.0f;		//! トレーサー始点を目線から下へずらす量(負で下)。
	const float	kAnimInterpolateTime = 0.2f;	//! アニメーション補間時間(秒)。

	const float	kEyeHeight = 160.0f;			//! 目(カメラ)の高さ。DebugPlayerScene側の kEyeHeight_ と合わせる。
	const float	kViewModelRight = 22.0f;		//! ビューモデル銃の右オフセット。
	const float	kViewModelDown = -26.0f;		//! ビューモデル銃の下オフセット(負で下)。
	const float	kTargetGunSize = 45.0f;			//! ビューモデル銃の目標サイズ(一番長い辺をこの長さに自動スケール)。
	const float	kWeaponRange = 3000.0f;			//! 射程(ヒットスキャンのレイ・トレーサーの長さ)。
	const float	kBleedOutTime = 15.0f;			//! ダウンしてから死亡するまでの出血時間(秒)。
	const int	kReviveHP = 30;					//! 救助で復帰したときのHP。
	const float	kShoveRange = 180.0f;			//! 突き飛ばしが届く距離。
	const float	kShovePush = 120.0f;			//! 突き飛ばしで敵を押し返す距離。
	const float	kShoveFrontDot = 0.5f;			//! 正面判定のしきい値(0.5=正面±60度)。
	const float	kShoveCooldownTime = 0.7f;		//! 突き飛ばしのクールダウン(秒)。
	const float	kMaxPitch = 1.4f;				//! カメラピッチの上下限(rad, ≈±80度)。真上/真下での破綻防止。
	const float	kSprintMul = 1.6f;			//! スプリント時の移動速度倍率。
	const float	kBobWalkSpeed = 9.0f;		//! 歩行ボブの速さ(歩き)。
	const float	kBobWalkAmp = 2.0f;			//! 歩行ボブの振れ幅(歩き)。
	const float	kBobSprintSpeed = 13.0f;	//! 歩行ボブの速さ(走り)。
	const float	kBobSprintAmp = 4.0f;		//! 歩行ボブの振れ幅(走り)。
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

			/* イベント発行先(勝敗管理などが購読)を取得する。デバッグシーン等、無い場合は発行しない。*/
			pEventBus_ = FindGO<nsEvent::EventBus>("eventBus");

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

			/* ポーズのトグル(生死・ポーズ状態に関わらず常に効く)。*/
			if (stIntent_.bPauseTrigger_)
				nsSystem::ToggleGamePaused();

			/* ポーズ中は更新を止める(モデル位置だけ維持)。*/
			if (nsSystem::IsGamePaused())
			{
				UpdateModel();
				return;
			}

			/* 2.生命状態を更新する(HP0でダウン→出血タイマー切れで死亡)。*/
			UpdateLifeState(fDeltaTime);

			/* 3.生存しているときだけ移動・武器・アクションを処理する(ダウン/死亡中は行動不能)。*/
			if (enLifeState_ == EnLifeState::Alive)
			{
				UpdateMove(fDeltaTime);
				UpdateWeapon(fDeltaTime);
				UpdateShove(fDeltaTime);
				UpdateItems();
				UpdateAction();
			}
			else
			{
				/* 行動不能中も武器のクールダウン等は進め、移動フラグは倒しておく。*/
				stWeaponInventory_.Update(fDeltaTime);
				bIsMoving_ = false;
			}

			/* 4.移動状態をモデル(位置・回転・アニメーション)へ反映する。*/
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

			/* マウスの縦移動量でカメラのピッチ(上下)を更新し、真上/真下付近で止める。*/
			fCameraPitch_ += stIntent_.fLookPitchDelta_;
			if (fCameraPitch_ > kMaxPitch)
				fCameraPitch_ = kMaxPitch;
			else if (fCameraPitch_ < -kMaxPitch)
				fCameraPitch_ = -kMaxPitch;

			/* カメラの旋回角から、地面基準の前方・右方向ベクトルを作る。*/
			const Vector3 vCameraForward = { sinf(fCameraYaw_), 0.0f, cosf(fCameraYaw_) };
			const Vector3 vCameraRight = { vCameraForward.z, 0.0f, -vCameraForward.x };

			const Vector3& vMoveAxis = stIntent_.vMoveAxis_;

			/* 移動入力が無ければ座標も向きも変えない(向きは維持してアイドルへ)。*/
			if (vMoveAxis.x == 0.0f && vMoveAxis.z == 0.0f)
			{
				bIsMoving_ = false;
				bIsSprinting_ = false;
				return;
			}

			/* カメラ相対の移動方向を作る(W=カメラ奥, S=手前, A/D=左右)。*/
			Vector3 vMoveDir = vCameraForward * vMoveAxis.z + vCameraRight * vMoveAxis.x;
			vMoveDir.y = 0.0f;
			vMoveDir.Normalize();

			/* スプリント(Shift)中は移動速度を上げて進む。*/
			bIsSprinting_ = stIntent_.bSprintPress_;
			const float fSpeed = bIsSprinting_ ? (fMoveSpeed_ * kSprintMul) : fMoveSpeed_;
			vPosition_ += vMoveDir * fSpeed * fDeltaTime;

			/*
			 * 原神風:常に進行方向を向いて前歩きする(Sで下がっても振り向くので後ろ歩きにならない)。
			 * TODO: 今は瞬時に向きが変わる。滑らかにするなら現在の向きから目標の向きへ補間する。
			 */
			vForward_ = vMoveDir;
			qRotation_.SetRotationY(atan2f(vMoveDir.x, vMoveDir.z));

			bIsMoving_ = true;
		}


		Vector3 Player::GetLookDirection() const
		{
			/* ヨー(水平)＋ピッチ(上下)から視線の単位ベクトルを作る。カメラと照準で共通。*/
			const float fCosPitch = cosf(fCameraPitch_);
			return Vector3(
				fCosPitch * sinf(fCameraYaw_),
				sinf(fCameraPitch_),
				fCosPitch * cosf(fCameraYaw_));
		}


		void Player::UpdateWeapon(float fDeltaTime)
		{
			/* 武器のクールタイム・リロード等を進める。*/
			stWeaponInventory_.Update(fDeltaTime);

			/* リロード。*/
			if (stIntent_.bReloadTrigger_)
				stWeaponInventory_.Reload();

			/* 照準方向はカメラの視線(ヨー＋ピッチ。画面中央=クロスヘア方向)。*/
			const Vector3 vAimDir = GetLookDirection();

			/* 現在の武器に応じて、発射入力を押しっぱなし(フルオート)か押した瞬間(単発)で判定する。*/
			nsWeapon::Weapon* pWeapon = stWeaponInventory_.GetCurrentWeapon();
			if (pWeapon != nullptr)
			{
				const bool bWantFire = pWeapon->IsFullAuto()
					? stIntent_.bFirePress_
					: stIntent_.bFireTrigger_;

				if (bWantFire)
				{
					/* 命中判定はカメラ(目)=クロスヘアから飛ばす。銃口はトレーサーの見た目始点にだけ使う。*/
						Vector3 vEyePos = vPosition_;
						vEyePos.y += kEyeHeight;

						/* トレーサーの始点=画面の銃口あたり(目線から右・下・前へずらす)。
					   目線=射線と横にずらすことで、線が点に潰れず斜めの線として見える。*/
					const Vector3 vMuzzleRight = { cosf(fCameraYaw_), 0.0f, -sinf(fCameraYaw_) };
					Vector3 vMuzzlePos = vEyePos;
					vMuzzlePos += vMuzzleRight * kMuzzleRight;
					vMuzzlePos += vAimDir * kMuzzleForward;
					vMuzzlePos.y += kMuzzleDown;

					/* 発射に成功したら、ヒットスキャン判定＋トレーサー表示を行う。*/
					if (stWeaponInventory_.Fire(vMuzzlePos, vAimDir))
					{
						/* レイの終点(最大射程)。命中したらここを命中点に置き換える。*/
						/* マズルフラッシュ(銃口の閃光を一瞬)。*/
							nsEffect::HitEffect* pFlash = NewGO<nsEffect::HitEffect>(0, "hitEffect");
							pFlash->Setup(vMuzzlePos, 8.0f, 22.0f, 0.04f);

							Vector3 vHitPoint = vEyePos + vAimDir * kWeaponRange;

						/*
						 * ヒットスキャン命中判定: レイ(銃口→射程)に対し、敵を球とみなして
						 * 最も手前で交差する1体を探す。コライダー未整備のため自前の レイvs球。
						 * (将来コライダーが付いたら PhysicsWorld::RayTest へ置換してよい)
						 */
						const float kEnemyRadius = 45.0f;		// 敵の水平被弾半径(人型を縦シリンダー近似)。
							const float kEnemyHeight = 175.0f;	// 足元(y=0)から頭までのおおよその高さ。
						nsActor::CommonEnemy* pHitEnemy = nullptr;
						float fNearest = kWeaponRange;			// 命中点までの前方距離(近いほど手前)。
						for (nsActor::CommonEnemy* pEnemy : FindGOs<nsActor::CommonEnemy>("commonEnemy"))
						{
							if (pEnemy == nullptr)
								continue;

							/* 敵を「足元(y=0)〜頭」の縦シリンダーとみなし、まず体の中心で前方距離を測る。*/
								const Vector3 vFeet = pEnemy->GetPosition();
								Vector3 vBodyMid = vFeet;
								vBodyMid.y += kEnemyHeight * 0.5f;

								const Vector3 vToMid = vBodyMid - vEyePos;
								const float fForward = vToMid.Dot(vAimDir);
								if (fForward < 0.0f || fForward > fNearest)
									continue;

								/* レイ上の最近点で、水平距離が半径以内 かつ 当たった高さが体の範囲内なら命中。*/
								const Vector3 vClosest = vEyePos + vAimDir * fForward;
								const float fDx = vClosest.x - vFeet.x;
								const float fDz = vClosest.z - vFeet.z;
								const float fHorizDist = sqrtf(fDx * fDx + fDz * fDz);
								if (fHorizDist <= kEnemyRadius &&
									vClosest.y >= vFeet.y - 20.0f &&
									vClosest.y <= vFeet.y + kEnemyHeight + 20.0f)
								{
									fNearest = fForward;
									pHitEnemy = pEnemy;
								}
						}

						/* 命中していたら、トレーサーを命中点で止めてダメージを与える。*/
						if (pHitEnemy != nullptr)
						{
							vHitPoint = vEyePos + vAimDir * fNearest;
							pHitEnemy->ApplyDamage(pWeapon->GetAttackPower());

							/* 倒したら撃破エフェクト＋撃破イベントを出して退場させる。*/
								if (pHitEnemy->IsDead())
								{
									Vector3 vKillPos = pHitEnemy->GetPosition();
									vKillPos.y += 85.0f;
									nsEffect::HitEffect* pKill = NewGO<nsEffect::HitEffect>(0, "hitEffect");
									pKill->Setup(vKillPos, 30.0f, 110.0f, 0.25f);
									PublishGameEvent(nsEvent::EnGameEvent::EnemyKilled);

									DeleteGO(pHitEnemy);
								}
						}

						/* 見せるためのトレーサー(曳光弾)を一瞬だけ表示する。*/
						nsWeapon::Tracer* pTracer = NewGO<nsWeapon::Tracer>(0, "tracer");
						pTracer->Setup(vMuzzlePos, vHitPoint);
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


		void Player::UpdateShove(float fDeltaTime)
		{
			/* クールダウンを進める。*/
			if (fShoveCooldown_ > 0.0f)
				fShoveCooldown_ -= fDeltaTime;

			/* 突き飛ばし入力が無い、またはクールダウン中なら何もしない。*/
			if (!stIntent_.bShoveTrigger_ || fShoveCooldown_ > 0.0f)
				return;

			/* クールダウンを設定する。*/
			fShoveCooldown_ = kShoveCooldownTime;

			/* 正面(水平)方向。*/
			const Vector3 vAimDir = { sinf(fCameraYaw_), 0.0f, cosf(fCameraYaw_) };

			/* 前方の近距離にいる敵を押し返す。*/
			for (nsActor::CommonEnemy* pEnemy : FindGOs<nsActor::CommonEnemy>("commonEnemy"))
			{
				if (pEnemy == nullptr)
					continue;

				/* プレイヤーから敵への水平ベクトル。*/
				Vector3 vToEnemy = pEnemy->GetPosition() - vPosition_;
				vToEnemy.y = 0.0f;
				const float fDist = vToEnemy.Length();

				/* 近距離かつ正面(約120度以内)のみ対象。*/
				if (fDist <= 0.0001f || fDist > kShoveRange)
					continue;
				Vector3 vDir = vToEnemy;
				vDir.Normalize();
				if (vDir.Dot(vAimDir) < kShoveFrontDot)
					continue;

				/* 敵を外側へ押し返す(のけぞりの簡易版)。*/
				pEnemy->GetPosition() += vDir * kShovePush;
			}

			/* TODO: 突き飛ばしのSE/モーション、特殊感染者への効果差など。*/
			OutputDebugStringA("[Player] Shove!\n");
		}


		void Player::UpdateItems()
		{
			/* 回復(メディキット): HPが減っていれば1個消費して全回復。*/
			if (stIntent_.bHealTrigger_ && iMedkitCount_ > 0 && GetCurrentHP() < GetMaxHP())
			{
				iMedkitCount_--;
				stCharacterStatus_.stHp_.iCurrentHP_ = GetMaxHP();

				Vector3 vHealPos = vPosition_;
				vHealPos.y += 80.0f;
				nsEffect::HitEffect* pHeal = NewGO<nsEffect::HitEffect>(0, "hitEffect");
				pHeal->Setup(vHealPos, 40.0f, 130.0f, 0.3f);
			}

			/* 投擲(グレネード): 1個消費して視線方向へ投げる。*/
			if (stIntent_.bThrowTrigger_ && iGrenadeCount_ > 0)
			{
				iGrenadeCount_--;

				const Vector3 vLook = GetLookDirection();
				Vector3 vThrowPos = vPosition_;
				vThrowPos.y += kEyeHeight;
				vThrowPos += vLook * kMuzzleForward;
				nsItem::Grenade* pGrenade = NewGO<nsItem::Grenade>(0, "grenade");
				pGrenade->Setup(vThrowPos, vLook);
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
			const Vector3 vLook = GetLookDirection();								// 前(ヨー＋ピッチ。上下を向くと銃も追従)
			const Vector3 vRight = { cosf(fCameraYaw_), 0.0f, -sinf(fCameraYaw_) };	// 右(水平・単位)

			/* 最終スケール = 自動サイズ合わせ × 武器データの微調整倍率(1.0基準)。*/
			const float fScale = aWeaponModelAutoScale_[iType] * pWeapon->GetModelScale();

			/* 銃の「中心」を置きたい位置(目の前・右・下)。*/
			Vector3 vGunPos = vPosition_;
			vGunPos.y += kEyeHeight;						// 目の高さ
			vGunPos += vLook * pWeapon->GetViewModelForward();	// 前(武器ごと)
			vGunPos += vRight * kViewModelRight;			// 右
			vGunPos.y += kViewModelDown;					// 下(負)

			/* 歩行ボブ: 移動中は銃を軽く揺らす(スプリントで大きく速く)。*/
			{
				const float fBobDelta = g_gameTime->GetFrameDeltaTime();
				const float fBobSpeed = bIsSprinting_ ? kBobSprintSpeed : kBobWalkSpeed;
				const float fBobAmp = bIsSprinting_ ? kBobSprintAmp : kBobWalkAmp;

				/* 停止時にピタッと消えないよう重みを補間する。*/
				const float fBobTarget = bIsMoving_ ? 1.0f : 0.0f;
				float fBobLerp = fBobDelta * 8.0f;
				if (fBobLerp > 1.0f) fBobLerp = 1.0f;
				fBobWeight_ += (fBobTarget - fBobWeight_) * fBobLerp;

				if (bIsMoving_)
					fBobTimer_ += fBobDelta * fBobSpeed;

				vGunPos += vRight * (sinf(fBobTimer_) * fBobAmp * fBobWeight_);
				vGunPos.y += sinf(fBobTimer_ * 2.0f) * fBobAmp * fBobWeight_;
			}

			/*
			 * モデル原点のズレを打ち消す。ローカル中心を世界の基底(右=vRight, 上=Y, 前=vLook)に
			 * 写して vGunPos から引けば、中心がぴったり vGunPos に来る(上下の微差は無視できる範囲)。
			 * これで原点がズレたモデルでもカメラにめり込まなくなる。
			 */
			const Vector3& vCenter = aWeaponModelCenter_[iType];
			Vector3 vCenterOffset =
				  vRight * (vCenter.x * fScale)
				+ Vector3(0.0f, vCenter.y * fScale, 0.0f)
				+ vLook  * (vCenter.z * fScale);

			weaponModel.SetPosition(vGunPos - vCenterOffset);
			Quaternion qGun;
			qGun.SetRotationY(fCameraYaw_);		// ヨー(横向き)を先に設定。
			qGun.AddRotationX(-fCameraPitch_);	// ピッチをローカル軸で後乗せ→横向きでもロールしない。上下が逆なら符号反転。
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


		void Player::UpdateLifeState(float fDeltaTime)
		{
			switch (enLifeState_)
			{
			case EnLifeState::Alive:
				/* HPが尽きたらダウンへ移行する(まだ死亡ではない)。*/
				if (IsDead())
				{
					enLifeState_ = EnLifeState::Down;
					fBleedOutTimer_ = kBleedOutTime;
					PublishGameEvent(nsEvent::EnGameEvent::PlayerDowned);
				}
				break;

			case EnLifeState::Down:
				/* 出血で残り時間が減り、尽きたら死亡する(ソロは救助者がいないので通常ここへ至る)。*/
				fBleedOutTimer_ -= fDeltaTime;
				if (fBleedOutTimer_ <= 0.0f)
				{
					fBleedOutTimer_ = 0.0f;
					enLifeState_ = EnLifeState::Dead;
					PublishGameEvent(nsEvent::EnGameEvent::PlayerDead);
				}
				break;

			case EnLifeState::Dead:
				/* 何もしない。*/
				break;
			}
		}


		void Player::Revive()
		{
			/* ダウン中のみ救助可能。既定HPで生存へ戻す(将来の味方/BOTが呼ぶ想定)。*/
			if (enLifeState_ != EnLifeState::Down)
				return;

			stCharacterStatus_.stHp_.iCurrentHP_ = kReviveHP;
			enLifeState_ = EnLifeState::Alive;
			fBleedOutTimer_ = 0.0f;
			PublishGameEvent(nsEvent::EnGameEvent::PlayerRevived);
		}


		void Player::PublishGameEvent(nsEvent::EnGameEvent enType)
		{
			/* バスが取得できていれば発行する。*/
			if (pEventBus_ != nullptr)
				pEventBus_->Publish({ enType });
		}
	}
}
