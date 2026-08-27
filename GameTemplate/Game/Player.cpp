#include "stdafx.h"
#include "Player.h"
#include "LocalPlayerController.h"
#include "Weapon.h"
#include "Tracer.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Event/EventBus.h"
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

	const float kMuzzleForward = 50.0f;		//! 銃口の前方オフセット(照準方向)。
	const float kMuzzleHeight = 150.0f;		//! 銃口の高さ(目線付近から撃つ)。
	const float kMuzzleRight = 40.0f;		//! トレーサー始点を目線から右へずらす量(線を斜めに見せる)。
	const float kMuzzleDown = -32.0f;		//! トレーサー始点を目線から下へずらす量(負で下)。
	const float kAnimInterpolateTime = 0.2f;	//! アニメーション補間時間(秒)。

	const float kEyeHeight = 160.0f;			//! 目(カメラ)の高さ。DebugPlayerScene側の kEyeHeight_ と合わせる。
	const float kViewModelRight = 22.0f;		//! ビューモデル銃の右オフセット。
	const float kViewModelDown = -26.0f;		//! ビューモデル銃の下オフセット(負で下)。
	const float kTargetGunSize = 45.0f;			//! ビューモデル銃の目標サイズ(一番長い辺をこの長さに自動スケール)。
	const float kWeaponRange = 3000.0f;			//! 射程(ヒットスキャンのレイ・トレーサーの長さ)。
	const float kBleedOutTime = 15.0f;			//! ダウンしてから死亡するまでの出血時間(秒)。
	const int kReviveHP = 30;					//! 救助で復帰したときのHP。
	const float kShoveRange = 180.0f;			//! 突き飛ばしが届く距離。
	const float kShovePush = 120.0f;			//! 突き飛ばしで敵を押し返す距離。
	const float kShoveFrontDot = 0.5f;			//! 正面判定のしきい値(0.5=正面±60度)。
	const float kShoveCooldownTime = 0.7f;		//! 突き飛ばしのクールダウン(秒)。
	const float kPi = 3.14159265f;			//! 円周率。
	const float kAdsTransitionRate = 12.0f;	//! 覗き込みの切り替わる速さ(大きいほど素早く構える)。
	const float kSpreadRecoverRate = 4.0f;	//! 連射で広がった拡散が収まる速さ。
	const float kMaxSpreadShot = 0.06f;		//! 連射で増える拡散角の上限(ラジアン)。
	const float kAdsViewModelRight = 6.0f;	//! 覗き込み時に銃を画面中央へ寄せた後の右オフセット。
	const float kAdsViewModelDown = -12.0f;	//! 覗き込み時の銃の下オフセット。
	const float kReloadLowerAngle = 0.9f;	//! リロード中に銃口を下げる角度(ラジアン)。
	const float kReloadLowerDown = 26.0f;	//! リロード中に銃を下げる距離。
	const float kReloadSpinAngle = 6.2832f;	//! リロード中に銃を回す角度(ラジアン。1周ぶん)。
	const float kSprintLowerAngle = 0.7f;	//! スプリント中に銃口を下げる角度(ラジアン)。走っている間は構えを解く。
	const float kSprintLowerDown = 18.0f;	//! スプリント中に銃を下げる距離。
	const float kSprintLowerRate = 8.0f;	//! 銃を下げる/戻す速さ。
	const float kHeadHeightRate = 0.85f;	//! 体の高さのうち、ここから上を頭とみなす割合。
	const float kHeadShotRate = 2.0f;		//! 頭に当てたときのダメージ倍率。
	const float kAdsRollAngle = 1.5708f;	//! 覗き込み時に銃を倒す角度(ラジアン。約90度)。視界を塞がないよう横倒しにする。
	const float kAdsSensitivityRate = 0.55f;	//! 覗き込み中の視点移動の倍率(拡大しているぶん狙いを合わせやすくする)。
	const float kAdsRecoilRate = 0.6f;		//! 覗き込み中の反動の倍率(構えるほど跳ねが小さくなる)。
	const float kRecoilRecoverRate = 6.0f;	//! 反動が戻る速さ(大きいほど早く元へ戻る)。
	const float kKickBackRecoverRate = 12.0f;	//! 銃のキックバックが戻る速さ。
	const float kMaxPitch = 1.4f;				//! カメラピッチの上下限(rad, ≈±80度)。真上/真下での破綻防止。
	const float kCapsuleRadius = 25.0f;		//! 移動用カプセルの半径(壁との押し戻しに使う)。
	const float kCapsuleHeight = 120.0f;	//! 移動用カプセルの高さ。
	const float kSprintMul = 1.6f;			//! スプリント時の移動速度倍率。
	const float kBobWalkSpeed = 9.0f;		//! 歩行ボブの速さ(歩き)。
	const float kBobWalkAmp = 2.0f;			//! 歩行ボブの振れ幅(歩き)。
	const float kBobSprintSpeed = 13.0f;	//! 歩行ボブの速さ(走り)。
	const float kBobSprintAmp = 4.0f;		//! 歩行ボブの振れ幅(走り)。
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

			/* 移動処理を初期化する。カプセルで壁(PhysicsStaticObject)と当たる。*/
			stMovement_.Init(kCapsuleRadius, kCapsuleHeight, vPosition_);
			stMovement_.SetGravityEnabled(true);

			/* モデルとアニメーションを読み込む。*/
			InitModel();

			/* テスト用のHPを設定する。*/
			stCharacterStatus_.stHp_.iCurrentHP_ = 100;
			stCharacterStatus_.stHp_.iMaxHP_ = 100;

			/* 被弾を見つけるため、開始時のHPを覚えておく。*/
			iPrevHP_ = GetCurrentHP();

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

			/* HPが減っていれば攻撃を受けたとみなして通知する。*/
			const int iNowHP = GetCurrentHP();
			if (iNowHP < iPrevHP_)
				PublishGameEvent(nsEvent::EnGameEvent::PlayerDamaged, vPosition_, Vector3::Zero, iPrevHP_ - iNowHP);

			iPrevHP_ = iNowHP;

			/* 2.覗き込みの度合いと弾の拡散を更新する。*/
			UpdateAds(fDeltaTime);

			/* 3.射撃の反動を時間で元へ戻す。*/
			UpdateRecoil(fDeltaTime);

			/* 4.生命状態を更新する(HP0でダウン→出血タイマー切れで死亡)。*/
			UpdateLifeState(fDeltaTime);

			/* 5.生存しているときだけ移動・武器・アクションを処理する(ダウン/死亡中は行動不能)。*/
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

			/* 6.移動状態をモデル(位置・回転・アニメーション)へ反映する。*/
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
			/* 覗き込み中は拡大されるぶん、視点の動きを鈍くして狙いやすくする。*/
			const float fLookRate = 1.0f - (1.0f - kAdsSensitivityRate) * fAdsRate_;

			fCameraYaw_ += stIntent_.fLookYawDelta_ * fLookRate;

			/* マウスの縦移動量でカメラのピッチ(上下)を更新し、真上/真下付近で止める。*/
			fCameraPitch_ += stIntent_.fLookPitchDelta_ * fLookRate;
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

				/* 止まっていても移動処理は回す(壁との押し戻しを効かせるため)。*/
				vPosition_ = stMovement_.Execute(Vector3::Zero, fDeltaTime);
				return;
			}

			/* カメラ相対の移動方向を作る(W=カメラ奥, S=手前, A/D=左右)。*/
			Vector3 vMoveDir = vCameraForward * vMoveAxis.z + vCameraRight * vMoveAxis.x;
			vMoveDir.y = 0.0f;
			vMoveDir.Normalize();

			/* スプリント(Shift)中は移動速度を上げて進む。*/
			bIsSprinting_ = stIntent_.bSprintPress_;
			float fSpeed = bIsSprinting_ ? (fMoveSpeed_ * kSprintMul) : fMoveSpeed_;

			/* 覗き込み中はゆっくり歩く。*/
			nsWeapon::Weapon* pAdsWeapon = stWeaponInventory_.GetCurrentWeapon();
			if (pAdsWeapon != nullptr && fAdsRate_ > 0.0f)
				fSpeed *= 1.0f + (pAdsWeapon->GetAdsSpeedRate() - 1.0f) * fAdsRate_;

			/* 移動計算と壁との押し戻しは CharacterMovement クラスに一任する。*/
			vPosition_ = stMovement_.MoveOnDirection(vMoveDir, fSpeed, fDeltaTime);

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
			/* 操作した向きに射撃の反動を足したものが実際の視線になる。*/
			const float fYaw = fCameraYaw_ + fRecoilYaw_;
			const float fPitch = fCameraPitch_ + fRecoilPitch_;

			/* ヨー(水平)＋ピッチ(上下)から視線の単位ベクトルを作る。カメラと照準で共通。*/
			const float fCosPitch = cosf(fPitch);
			return Vector3(fCosPitch * sinf(fYaw), sinf(fPitch), fCosPitch * cosf(fYaw));
		}


		void Player::UpdateAds(float fDeltaTime)
		{
			/* 走っている間は構えられない(Apex同様、スプリントを優先する)。*/
			const bool bWantAds = stIntent_.bAdsPress_ && !bIsSprinting_;

			/* 覗き込みの度合いを目標へ滑らかに近づける。*/
			float fRate = fDeltaTime * kAdsTransitionRate;
			if (fRate > 1.0f)
				fRate = 1.0f;

			const float fTarget = bWantAds ? 1.0f : 0.0f;
			fAdsRate_ += (fTarget - fAdsRate_) * fRate;

			/* 走っている間は銃を下げ、やめたら構え直す。*/
			float fLowerRate = fDeltaTime * kSprintLowerRate;
			if (fLowerRate > 1.0f)
				fLowerRate = 1.0f;

			const float fLowerTarget = (bIsSprinting_ && bIsMoving_) ? 1.0f : 0.0f;
			fLowerRate_ += (fLowerTarget - fLowerRate_) * fLowerRate;

			/* 連射で広がった拡散を時間で収める。*/
			float fSpreadRate = fDeltaTime * kSpreadRecoverRate;
			if (fSpreadRate > 1.0f)
				fSpreadRate = 1.0f;

			fSpreadShot_ -= fSpreadShot_ * fSpreadRate;
		}


		float Player::GetAdsZoomRate()
		{
			/* 武器が無ければ画角を変えない。*/
			nsWeapon::Weapon* pWeapon = stWeaponInventory_.GetCurrentWeapon();
			if (pWeapon == nullptr)
				return 1.0f;

			return pWeapon->GetAdsZoomRate();
		}


		float Player::GetCurrentSpread() const
		{
			/* 武器が無ければ拡散も無い。*/
			nsWeapon::Weapon* pWeapon = const_cast<Player*>(this)->stWeaponInventory_.GetCurrentWeapon();
			if (pWeapon == nullptr)
				return 0.0f;

			/* 覗き込みの度合いで腰だめとADSの拡散を混ぜ、連射ぶんを足す。*/
			const float fBase = pWeapon->GetSpreadHip() + (pWeapon->GetSpreadAds() - pWeapon->GetSpreadHip()) * fAdsRate_;
			return fBase + fSpreadShot_;
		}


		Vector3 Player::MakeSpreadDirection(const Vector3& vAimDir) const
		{
			/* 武器が無ければ拡散させない。*/
			nsWeapon::Weapon* pWeapon = const_cast<Player*>(this)->stWeaponInventory_.GetCurrentWeapon();
			if (pWeapon == nullptr)
				return vAimDir;

			/* いまの拡散角を取り出す。*/
			const float fSpread = GetCurrentSpread();
			if (fSpread <= 0.0001f)
				return vAimDir;

			/* 照準を軸に、円錐の内側へランダムにずらす。*/
			const float fAngle = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f * kPi;
			const float fRadius = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * fSpread;

			/* 照準に垂直な2軸を作る。*/
			Vector3 vRight = { cosf(fCameraYaw_), 0.0f, -sinf(fCameraYaw_) };
			Vector3 vUp = vAimDir;
			vUp.Cross(vRight);

			/* 2軸へずらして方向を作り直す。*/
			Vector3 vResult = vAimDir;
			vResult += vRight * (cosf(fAngle) * fRadius);
			vResult += vUp * (sinf(fAngle) * fRadius);
			vResult.Normalize();

			return vResult;
		}


		void Player::UpdateRecoil(float fDeltaTime)
		{
			/* 跳ね上がった視点を徐々に元へ戻す。*/
			float fRecoverRate = fDeltaTime * kRecoilRecoverRate;
			if (fRecoverRate > 1.0f)
				fRecoverRate = 1.0f;

			fRecoilPitch_ -= fRecoilPitch_ * fRecoverRate;
			fRecoilYaw_ -= fRecoilYaw_ * fRecoverRate;

			/* 手前へ下がった銃を徐々に元の位置へ戻す。*/
			float fKickRate = fDeltaTime * kKickBackRecoverRate;
			if (fKickRate > 1.0f)
				fKickRate = 1.0f;

			fWeaponKickBack_ -= fWeaponKickBack_ * fKickRate;
		}


		void Player::ApplyFireRecoil(nsWeapon::Weapon* pWeapon)
		{
			/* 武器が無ければ反動も無い。*/
			if (pWeapon == nullptr)
				return;

			/*
			 * 撃った1発ぶんの跳ね方は武器のリコイルパターンが決めている。
			 * 同じ武器なら毎回同じ順で跳ねるので、形を覚えれば逆へ動かして抑えられる。
			 */
			const nsWeapon::RecoilStep& stStep = pWeapon->GetLastRecoilStep();

			/* 覗き込み中は反動を抑える。*/
			const float fAdsMul = 1.0f - (1.0f - kAdsRecoilRate) * fAdsRate_;

			/* パターンの倍率に武器の反動量を掛けて、視点を跳ね上げる。*/
			fRecoilPitch_ += pWeapon->GetRecoilPitch() * stStep.fPitch_ * fAdsMul;
			fRecoilYaw_ += pWeapon->GetRecoilYaw() * stStep.fYaw_ * fAdsMul;

			/* 銃を手前へ下げる(次のフレームから元へ戻っていく)。*/
			fWeaponKickBack_ = pWeapon->GetKickBack();
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
							/* 発射の反動を加える(視点が跳ね、銃が手前へ下がる)。*/
							ApplyFireRecoil(pWeapon);

							/* 連射するほど弾がばらつくようにする。*/
							fSpreadShot_ += pWeapon->GetSpreadPerShot();
							if (fSpreadShot_ > kMaxSpreadShot)
								fSpreadShot_ = kMaxSpreadShot;

							/* 発射したことを通知する(演出は購読側が担当する)。*/
							PublishGameEvent(nsEvent::EnGameEvent::WeaponFired, vMuzzlePos, vAimDir);

							/* 拡散のぶんだけ照準をばらつかせた、実際の弾道。*/
							const Vector3 vShotDir = MakeSpreadDirection(vAimDir);

							/* レイの終点(最大射程)。命中したらここを命中点に置き換える。*/
							Vector3 vHitPoint = vEyePos + vShotDir * kWeaponRange;

						/*
						 * ヒットスキャン命中判定: レイ(銃口→射程)に対し、敵を球とみなして
						 * 最も手前で交差する1体を探す。コライダー未整備のため自前の レイvs球。
						 * (将来コライダーが付いたら PhysicsWorld::RayTest へ置換してよい)
						 */
						const float kEnemyRadius = 45.0f;		// 敵の水平被弾半径(人型を縦シリンダー近似)。
							const float kEnemyHeight = 175.0f;	// 足元(y=0)から頭までのおおよその高さ。
						nsActor::CommonEnemy* pHitEnemy = nullptr;
						bool bHeadShot = false;					// 頭に当たったか。
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
								const float fForward = vToMid.Dot(vShotDir);
								if (fForward < 0.0f || fForward > fNearest)
									continue;

								/* レイ上の最近点で、水平距離が半径以内 かつ 当たった高さが体の範囲内なら命中。*/
								const Vector3 vClosest = vEyePos + vShotDir * fForward;
								const float fDx = vClosest.x - vFeet.x;
								const float fDz = vClosest.z - vFeet.z;
								const float fHorizDist = sqrtf(fDx * fDx + fDz * fDz);
								if (fHorizDist <= kEnemyRadius &&
									vClosest.y >= vFeet.y - 20.0f &&
									vClosest.y <= vFeet.y + kEnemyHeight + 20.0f)
								{
									fNearest = fForward;
									pHitEnemy = pEnemy;

									/* 体の上のほうに当たっていれば頭とみなす。*/
									bHeadShot = (vClosest.y >= vFeet.y + kEnemyHeight * kHeadHeightRate);
								}
						}

						/* 命中していたら、トレーサーを命中点で止めてダメージを与える。*/
						if (pHitEnemy != nullptr)
						{
							vHitPoint = vEyePos + vShotDir * fNearest;
							/* 頭に当たっていればダメージを増やす。*/
							int iDamage = pWeapon->GetAttackPower();
							if (bHeadShot)
								iDamage = static_cast<int>(iDamage * kHeadShotRate);

							pHitEnemy->ApplyDamage(iDamage);

							/* 命中の演出(当たった位置に出す)。*/
							PublishGameEvent(nsEvent::EnGameEvent::BulletHit, vHitPoint, vShotDir, iDamage, bHeadShot);

							/* 倒したら撃破エフェクト＋撃破イベントを出して退場させる。*/
								if (pHitEnemy->IsDead())
								{
									Vector3 vKillPos = pHitEnemy->GetPosition();
									vKillPos.y += 85.0f;
									PublishGameEvent(nsEvent::EnGameEvent::EnemyKilled, vKillPos);

									DeleteGO(pHitEnemy);
								}
						}

						/* 見せるためのトレーサー(曳光弾)を一瞬だけ表示する。*/
						nsWeapon::Tracer* pTracer = NewGO<nsWeapon::Tracer>(0, "tracer");
						pTracer->Setup(vMuzzlePos, vHitPoint);
					}
				}
			}

			/* 数字キーでの直接持ち替え(ホイールより優先する)。*/
			if (stIntent_.bMainWeaponTrigger_)
				stWeaponInventory_.SwitchToSlot(nsWeapon::EnWeaponSlot::Main);
			else if (stIntent_.bSubWeaponTrigger_)
				stWeaponInventory_.SwitchToSlot(nsWeapon::EnWeaponSlot::Sub);

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

				/*
				 * 敵を外側へ押し返す(のけぞりの簡易版)。
				 * 座標を直接書き換えるだけだと敵の移動処理に上書きされてしまうため、
				 * 移動処理へも反映される SetPosition を使う。
				 */
				pEnemy->SetPosition(pEnemy->GetPosition() + vDir * kShovePush);
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
				PublishGameEvent(nsEvent::EnGameEvent::PlayerHealed, vHealPos);
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
			/* 覗き込むほど、銃を画面中央(照準の位置)へ寄せる。*/
			const float fRightOffset = kViewModelRight + (kAdsViewModelRight - kViewModelRight) * fAdsRate_;
			const float fDownOffset = kViewModelDown + (kAdsViewModelDown - kViewModelDown) * fAdsRate_;
			vGunPos += vRight * fRightOffset;				// 右
			vGunPos.y += fDownOffset;						// 下(負)
			vGunPos.y -= kSprintLowerDown * fLowerRate_;	// 走っている間はさらに下げる

			/*
			 * リロード中は銃を下げて回す。
			 * 弾を入れ替えている最中だと分かるよう、山なりに沈めてから戻す。
			 */
			const float fReloadRate = pWeapon->GetReloadRate();
			const float fReloadDip = sinf(fReloadRate * kPi);	// 0→1→0 と動く沈み具合。
			vGunPos.y -= kReloadLowerDown * fReloadDip;
			vGunPos -= vLook * fWeaponKickBack_;			// 撃った直後は手前へ下がる

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

				/* 覗き込み中は狙いが定まるよう揺れを抑える。*/
				const float fBobScale = fBobAmp * fBobWeight_ * (1.0f - fAdsRate_);
				vGunPos += vRight * (sinf(fBobTimer_) * fBobScale);
				vGunPos.y += sinf(fBobTimer_ * 2.0f) * fBobScale;
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
			qGun.AddRotationZ(kAdsRollAngle * fAdsRate_);	// 覗き込むほど銃を横倒しにし、拡大時に視界を塞がないようにする。
			qGun.AddRotationX(kSprintLowerAngle * fLowerRate_);	// 走っている間は銃口を下げて構えを解く。
			qGun.AddRotationX(kReloadLowerAngle * fReloadDip);	// リロード中は銃口を下げる。
			qGun.AddRotationZ(kReloadSpinAngle * fReloadRate);	// リロード中は銃を1周させる。
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


		void Player::PublishGameEvent(nsEvent::EnGameEvent enType, const Vector3& vPosition, const Vector3& vDirection, int iParam, bool bIsCritical)
		{
			/* バスが取得できていれば発行する。*/
			if (pEventBus_ == nullptr)
				return;

			nsEvent::GameEvent stEvent;
			stEvent.enType_ = enType;
			stEvent.vPosition_ = vPosition;
			stEvent.vDirection_ = vDirection;
			stEvent.iParam_ = iParam;
			stEvent.bIsCritical_ = bIsCritical;
			pEventBus_->Publish(stEvent);
		}
	}
}
