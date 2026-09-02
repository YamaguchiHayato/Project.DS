#include "stdafx.h"
#include "Player.h"
#include "LocalPlayerController.h"
#include "Weapon.h"
#include "Tracer.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Event/EventBus.h"
#include "Src/System/GamePause.h"
#include "Src/Item/Grenade.h"
#include "Src/Item/Pickup.h"
#include "Src/Data/PlayerStatusTable.h"
#include "Src/Combat/HitBoxSet.h"

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
	const float kMuzzleRight = 40.0f;		//! トレーサー始点を目線から右へずらす量(線を斜めに見せる)。
	const float kMuzzleDown = -32.0f;		//! トレーサー始点を目線から下へずらす量(負で下)。
	const float kAnimInterpolateTime = 0.2f;	//! アニメーション補間時間(秒)。

	const float kViewModelRight = 22.0f;		//! ビューモデル銃の右オフセット。
	const float kViewModelDown = -26.0f;		//! ビューモデル銃の下オフセット(負で下)。
	const float kTargetGunSize = 45.0f;			//! ビューモデル銃の目標サイズ(一番長い辺をこの長さに自動スケール)。
	const float kHandHeightRate = 0.55f;	//! 右手のボーンが見つからないときに銃を置く高さ(目線の高さに対する割合)。
	const float kPi = 3.14159265f;			//! 円周率。
	const float kAdsTransitionRate = 12.0f;	//! 覗き込みの切り替わる速さ(大きいほど素早く構える)。
	const float kSpreadRecoverRate = 4.0f;	//! 連射で広がった拡散が収まる速さ。
	const float kAdsViewModelRight = 6.0f;	//! 覗き込み時に銃を画面中央へ寄せた後の右オフセット。
	const float kAdsViewModelDown = -12.0f;	//! 覗き込み時の銃の下オフセット。

	/* リロード動作の区切り。0=リロード開始、1=完了間際。*/
	const float kReloadPullEnd = 0.28f;		//! ここまでで銃を下げ切る(マガジンを抜く)。
	const float kReloadInsertStart = 0.34f;	//! ここからマガジンを挿し込む。
	const float kReloadInsertEnd = 0.52f;	//! ここで挿し込み終わる。
	const float kReloadRaiseStart = 0.58f;	//! ここから構え直す。
	const float kReloadRaiseEnd = 0.96f;	//! ここで構え終わる。
	const float kReloadSettleStart = 0.90f;	//! ここから戻りの行き過ぎが出る。
	const float kEnemyCenterHeightForShove = 85.0f;	//! 突き飛ばしの手応えを出す高さ(敵の体の中心)。
	const int kShoveDamage = 10;			//! 突き飛ばしで与えるダメージ。押し返しが主で、削るのはおまけ。
	const int kPickupAmmoAmount = 60;		//! 弾薬をひとつ拾ったときに補給される予備弾数。
	const int kFlashLightIndex = 0;			//! 手持ちライトに使うスポットライトの番号。
	const float kFlashLightRange = 900.0f;	//! ライトが届く距離。
	const float kFlashLightAngle = 0.5f;	//! ライトの広がり(ラジアン)。
	const float kSprintLowerAngle = 0.7f;	//! スプリント中に銃口を下げる角度(ラジアン)。走っている間は構えを解く。
	const float kSprintLowerDown = 18.0f;	//! スプリント中に銃を下げる距離。
	const float kSprintLowerRate = 8.0f;	//! 銃を下げる/戻す速さ。
	const float kAdsRollAngle = 1.5708f;	//! 覗き込み時に銃を倒す角度(ラジアン。約90度)。視界を塞がないよう横倒しにする。
	const float kAdsSensitivityRate = 0.55f;	//! 覗き込み中の視点移動の倍率(拡大しているぶん狙いを合わせやすくする)。
	const float kAdsRecoilRate = 0.6f;		//! 覗き込み中の反動の倍率(構えるほど跳ねが小さくなる)。
	const float kRecoilRecoverRate = 6.0f;	//! 反動が戻る速さ(大きいほど早く元へ戻る)。
	const float kKickBackRecoverRate = 12.0f;	//! 銃のキックバックが戻る速さ。
	const float kMaxPitch = 1.4f;				//! カメラピッチの上下限(rad, ≈±80度)。真上/真下での破綻防止。
	const float kCapsuleRadius = 25.0f;		//! 移動用カプセルの半径(壁との押し戻しに使う)。
	const float kCapsuleHeight = 120.0f;	//! 移動用カプセルの高さ。




	/**
	 * @brief 値を上限と下限の内側に収める。
	 * @param fValue 収める値。
	 * @param fLimit 上限(下限は-fLimit)。
	 * @return 収めた値。
	 */
	float ClampAbs(float fValue, float fLimit)
	{
		if (fValue > fLimit)
			return fLimit;

		if (fValue < -fLimit)
			return -fLimit;

		return fValue;
	}


	/**
	 * @brief 0から1へなめらかに変化する値を作る(始めと終わりがゆるやかになる)。
	 * @param fStart 変化が始まる位置。
	 * @param fEnd   変化が終わる位置。
	 * @param fValue いまの位置。
	 * @return 0〜1の値。
	 */
	float SmoothStep(float fStart, float fEnd, float fValue)
	{
		/* 幅が無ければ、終点を越えたかどうかだけで決める。*/
		if (fEnd <= fStart)
			return (fValue >= fEnd) ? 1.0f : 0.0f;

		const float fRate = (fValue - fStart) / (fEnd - fStart);
		if (fRate <= 0.0f)
			return 0.0f;

		if (fRate >= 1.0f)
			return 1.0f;

		/* 3次式で両端をなめらかにする。*/
		return fRate * fRate * (3.0f - 2.0f * fRate);
	}


	/**
	 * @brief 0→1→0 と山なりに変化する値を作る(短い動きの1往復に使う)。
	 * @param fStart 動きが始まる位置。
	 * @param fEnd   動きが終わる位置。
	 * @param fValue いまの位置。
	 * @return 0〜1の値。範囲の外では0。
	 */
	float PulseCurve(float fStart, float fEnd, float fValue)
	{
		if (fValue <= fStart || fValue >= fEnd || fEnd <= fStart)
			return 0.0f;

		return sinf(kPi * (fValue - fStart) / (fEnd - fStart));
	}
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
			/* 調整用のステータスを読み込む(Assets/data/player.json。無ければ既定値)。*/
			stPlayerStatus_ = nsData::PlayerStatusTable::Get();

			/* 操作意図の供給源を作る(ローカル実機。将来ここをネット受信用に差し替えられる)。*/
			pController_ = new LocalPlayerController();

			/* 移動処理を初期化する。カプセルで壁(PhysicsStaticObject)と当たる。*/
			stMovement_.Init(kCapsuleRadius, kCapsuleHeight, vPosition_);
			/*
			 * 重力は床のあるステージが入ってから有効にする。
			 * 床が無い状態で有効にすると、接地できず落ち続けてしまう。
			 */
			stMovement_.SetGravityEnabled(false);

			/* モデルとアニメーションを読み込む。*/
			InitModel();

			/* HPと所持アイテム数をステータス表から設定する。*/
			stCharacterStatus_.stHp_.iCurrentHP_ = stPlayerStatus_.iMaxHP_;
			stCharacterStatus_.stHp_.iMaxHP_ = stPlayerStatus_.iMaxHP_;
			iMedkitCount_ = stPlayerStatus_.iMedkitCount_;
			iGrenadeCount_ = stPlayerStatus_.iGrenadeCount_;

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

				/* 移動の結果から揺れを作る。射撃の起点(目の位置)にも効くので武器より先に更新する。*/
				UpdateViewSway(fDeltaTime);

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
				bIsSprinting_ = false;

				/* 動けないので揺れは収まっていく。*/
				UpdateViewSway(fDeltaTime);
			}

			/* 6.移動状態をモデル(位置・回転・アニメーション)へ反映する。*/
			UpdateModel();
		}


		void Player::Render(RenderContext& rc)
		{
			/*
			 * 自分の視点(一人称)のときは体モデルを描かない。カメラが頭の位置にあり視界を塞ぐため。
			 * 他人から見た姿(三人称)では、銃を持った体がそのまま見える。
			 */
			if (enViewMode_ == EnViewMode::ThirdPerson)
				stModelRender_.Draw(rc);

			/* 現在装備中の武器モデルを描画する(置き場所は見せ方で変わる)。*/
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

			/*
			 * 体の大きさはステータス表で決める。
			 * 一人称では体を描かないので今まで見えていなかったが、三人称では他プレイヤーに見える。
			 * 敵や地面と大きさが揃わない場合は player.json の bodyModelScale で合わせる。
			 */
			const float fBodyScale = stPlayerStatus_.fBodyModelScale_;
			stModelRender_.SetScale(Vector3(fBodyScale, fBodyScale, fBodyScale));

			/* 実際の表示サイズを測っておく(三人称カメラの寄り引きに使う)。*/
			CalcBodyModelSize();
			stModelRender_.Update();

			/*
			 * 三人称で銃を持たせるボーンを探しておく(毎フレーム名前で探さないため)。
			 * 名前は player.json の handBoneName。見つからなければ -1 になり、銃は腰のあたりへ置かれる。
			 */
			if (stModelRender_.m_skeleton.IsInited())
			{
				wchar_t wcBoneName[128] = {};
				mbstowcs(wcBoneName, stPlayerStatus_.sHandBoneName_.c_str(), _countof(wcBoneName) - 1);
				iRightHandBoneId_ = stModelRender_.m_skeleton.FindBoneID(wcBoneName);
			}

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
			float fSpeed = bIsSprinting_ ? (stPlayerStatus_.fMoveSpeed_ * stPlayerStatus_.fSprintRate_) : stPlayerStatus_.fMoveSpeed_;

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


		void Player::UpdateViewSway(float fDeltaTime)
		{
			/* 揺れの調整値(player.jsonのviewShake)。*/
			const nsData::ViewShakeStatus& stShake = stPlayerStatus_.stViewShake_;

			/* 覗き込んでいる間は狙いが定まるよう、揺れ全体を抑える。*/
			const float fShakeRate = 1.0f - fAdsRate_ * stShake.fAdsSuppressRate_;

			/*
			 * 視点を振った量を「銃の遅れ」に変える。
			 * 振った向きと逆へずれてから中央へ戻るので、腕が視点に引きずられているように見える。
			 */
			fSwayRight_ -= stIntent_.fLookYawDelta_ * stShake.fSwayGain_ * fShakeRate;
			fSwayUp_ -= stIntent_.fLookPitchDelta_ * stShake.fSwayGain_ * fShakeRate;

			/* 離れ過ぎないよう上限を掛ける。*/
			fSwayRight_ = ClampAbs(fSwayRight_, stShake.fSwayMaxOffset_);
			fSwayUp_ = ClampAbs(fSwayUp_, stShake.fSwayMaxOffset_);

			/* 腕が追いついたぶんだけ中央へ戻す。*/
			float fSwayRecover = fDeltaTime * stShake.fSwayRecoverRate_;
			if (fSwayRecover > 1.0f)
				fSwayRecover = 1.0f;

			fSwayRight_ -= fSwayRight_ * fSwayRecover;
			fSwayUp_ -= fSwayUp_ * fSwayRecover;

			/* 歩行ボブの重み。止まった瞬間にピタッと消えないよう補間する。*/
			float fWeightLerp = fDeltaTime * stShake.fBobWeightRate_;
			if (fWeightLerp > 1.0f)
				fWeightLerp = 1.0f;

			const float fWeightTarget = bIsMoving_ ? 1.0f : 0.0f;
			fBobWeight_ += (fWeightTarget - fBobWeight_) * fWeightLerp;

			/* 歩いている間だけ位相を進める(止まると波が止まり、重みで消えていく)。*/
			if (bIsMoving_)
				fBobTimer_ += fDeltaTime * (bIsSprinting_ ? stShake.fBobSprintSpeed_ : stShake.fBobWalkSpeed_);

			/*
			 * カメラ自体を歩きに合わせて上下させる。
			 * 銃だけを揺らすと「銃が揺れている」ようにしか見えないので、体のほうを動かす。
			 * 1歩で2回沈むので、上下は位相の2倍で回す。
			 */
			const float fBobShake = fBobWeight_ * fShakeRate;
			const float fBobHeight = bIsSprinting_ ? stShake.fViewBobHeightSprint_ : stShake.fViewBobHeightWalk_;
			fViewBobHeight_ = sinf(fBobTimer_ * 2.0f) * fBobHeight * fBobShake;

			/* 左右の踏み込みに合わせてカメラをわずかに傾ける。*/
			const float fBobRollAngle = bIsSprinting_ ? stShake.fViewBobRollSprint_ : stShake.fViewBobRollWalk_;
			const float fBobRoll = sinf(fBobTimer_) * fBobRollAngle * fBobShake;

			/* 横移動に合わせた傾き(ストレイフロール)。入力を鈍らせてから使う。*/
			float fStrafeLerp = fDeltaTime * stShake.fStrafeFollowRate_;
			if (fStrafeLerp > 1.0f)
				fStrafeLerp = 1.0f;

			const float fStrafeTarget = bIsMoving_ ? stIntent_.vMoveAxis_.x : 0.0f;
			fStrafeAxis_ += (fStrafeTarget - fStrafeAxis_) * fStrafeLerp;

			fViewRoll_ = fBobRoll - fStrafeAxis_ * stShake.fStrafeRollAngle_ * fShakeRate;
		}


		void Player::CalcReloadMotion(float fReloadRate, float& fOutLower, float& fOutInsert, float& fOutSettle) const
		{
			/*
			 * 銃を下げている度合い。
			 * 前半で下げ切って(マガジンを抜く)、後半で構え直すぶんを引く。
			 */
			fOutLower = SmoothStep(0.0f, kReloadPullEnd, fReloadRate)
				- SmoothStep(kReloadRaiseStart, kReloadRaiseEnd, fReloadRate);

			/* マガジンを挿し込む短い突き上げ。0→1→0 と1往復する。*/
			fOutInsert = PulseCurve(kReloadInsertStart, kReloadInsertEnd, fReloadRate);

			/* 構え直した勢いで少し行き過ぎてから収まる。*/
			fOutSettle = PulseCurve(kReloadSettleStart, 1.0f, fReloadRate);
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


		CommonEnemy* Player::FindHitEnemy(const Vector3& vRayStart, const Vector3& vRayDirection, nsCombat::HitResult& stOutResult)
		{
			stOutResult = nsCombat::HitResult();

			/* 敵の部位別当たり判定(形とダメージ倍率)を取り出す。*/
			const nsCombat::HitBoxSet& stHitBoxSet = nsCombat::HitBoxSet::GetShared(CharacterModelType::Infected);

			CommonEnemy* pHitEnemy = nullptr;

			/* 当たった敵が見つかるたびに射程を縮め、より手前の敵だけを残す。*/
			float fNearest = stPlayerStatus_.fWeaponRange_;

			for (CommonEnemy* pEnemy : FindGOs<CommonEnemy>("commonEnemy"))
			{
				if (pEnemy == nullptr)
					continue;

				/* 敵1体ぶんの部位別判定。足元の座標を基準に頭・胴・脚を並べて判定する。*/
				nsCombat::HitResult stResult;
				if (!stHitBoxSet.RayTest(vRayStart, vRayDirection, fNearest, pEnemy->GetPosition(), stResult))
					continue;

				fNearest = stResult.fDistance_;
				pHitEnemy = pEnemy;
				stOutResult = stResult;
			}

			return pHitEnemy;
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
					Vector3 vEyePos = GetEyePosition();

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
						if (fSpreadShot_ > pWeapon->GetMaxSpreadShot())
							fSpreadShot_ = pWeapon->GetMaxSpreadShot();

						/* 発射したことを通知する(演出は購読側が担当する)。*/
						PublishGameEvent(nsEvent::EnGameEvent::WeaponFired, vMuzzlePos, vAimDir);

						/* 拡散のぶんだけ照準をばらつかせた、実際の弾道。*/
						const Vector3 vShotDir = MakeSpreadDirection(vAimDir);

						/* レイの終点(最大射程)。命中したらここを命中点に置き換える。*/
						Vector3 vHitPoint = vEyePos + vShotDir * stPlayerStatus_.fWeaponRange_;

						/* ヒットスキャン命中判定: 目線から弾の向きへレイを飛ばし、当たった敵と部位を求める。*/
						nsCombat::HitResult stHitResult;
						nsActor::CommonEnemy* pHitEnemy = FindHitEnemy(vEyePos, vShotDir, stHitResult);

						/* 命中していたら、トレーサーを命中点で止めてダメージを与える。*/
						if (pHitEnemy != nullptr)
						{
							vHitPoint = stHitResult.vHitPoint_;

							/* 当たった部位の倍率でダメージを増減する(頭なら大ダメージ、脚なら効きが悪い)。*/
							int iDamage = static_cast<int>(pWeapon->GetAttackPower() * stHitResult.fDamageRate_);

							/* 倍率が小さくても、当てたのに0ダメージにはしない。*/
							if (iDamage < 1)
								iDamage = 1;

							/* 頭に当たったかはUIと演出で使う。*/
							const bool bHeadShot = (stHitResult.enPart_ == nsCombat::EnHitPart::Head);

							pHitEnemy->ApplyDamage(iDamage);

							/* 命中の演出(当たった位置に出す)。*/
							PublishGameEvent(nsEvent::EnGameEvent::BulletHit, vHitPoint, vShotDir, iDamage, bHeadShot);

							/* 倒したら撃破エフェクト＋撃破イベントを出して退場させる。*/
							if (pHitEnemy->IsDead())
							{
								/* 撃破の閃光は胸のあたり(身長の半分)に出す。*/
								Vector3 vKillPos = pHitEnemy->GetPosition();
								vKillPos.y += nsCombat::HitBoxSet::GetShared(CharacterModelType::Infected).GetHeight() * 0.5f;
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
			/* インタラクト: 足元に落ちている物資を拾う。*/
			if (stIntent_.bUseTrigger_)
				PickUpItem();

			/* ライトのON/OFF切り替え。*/
			if (stIntent_.bLightTrigger_)
				bIsLightOn_ = !bIsLightOn_;

			/*
			 * 手持ちのライトを目線の位置から視線の先へ向ける。
			 * 消えているときは届く距離を0にして、光が出ないようにする。
			 */
			g_renderingEngine->SetSpotLight(kFlashLightIndex, GetEyePosition(), { 1.0f, 0.95f, 0.85f }, bIsLightOn_ ? kFlashLightRange : 0.0f, GetLookDirection(), kFlashLightAngle);

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
			fShoveCooldown_ = stPlayerStatus_.fShoveCooldownTime_;

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
				if (fDist <= 0.0001f || fDist > stPlayerStatus_.fShoveRange_)
					continue;
				Vector3 vDir = vToEnemy;
				vDir.Normalize();
				if (vDir.Dot(vAimDir) < stPlayerStatus_.fShoveFrontDot_)
					continue;

				/*
				 * 敵を外側へ押し返す(のけぞりの簡易版)。
				 * 座標を直接書き換えるだけだと敵の移動処理に上書きされてしまうため、
				 * 移動処理へも反映される SetPosition を使う。
				 */
				pEnemy->SetPosition(pEnemy->GetPosition() + vDir * stPlayerStatus_.fShovePush_);

				/* 押し返すだけでなく、わずかに削る。*/
				pEnemy->ApplyDamage(kShoveDamage);

				/* 当たった手応えを通知する(位置は敵の体の中心あたり)。*/
				Vector3 vShoveHitPos = pEnemy->GetPosition();
				vShoveHitPos.y += kEnemyCenterHeightForShove;
				PublishGameEvent(nsEvent::EnGameEvent::BulletHit, vShoveHitPos, vDir, kShoveDamage);

				/* 倒したら退場させる。*/
				if (pEnemy->IsDead())
				{
					PublishGameEvent(nsEvent::EnGameEvent::EnemyKilled, pEnemy->GetPosition());
					DeleteGO(pEnemy);
				}
			}

			/* TODO: 突き飛ばしのSE/モーション、特殊感染者への効果差など。*/
			OutputDebugStringA("[Player] Shove!\n");
		}


		void Player::PickUpItem()
		{
			/* 拾える距離にある物資を探す。*/
			for (nsItem::Pickup* pPickup : FindGOs<nsItem::Pickup>("pickup"))
			{
				if (pPickup == nullptr || !pPickup->IsInRange(vPosition_))
					continue;

				/* 種類に応じて補給する。満たされていて拾えない場合は次を探す。*/
				bool bPickedUp = false;
				switch (pPickup->GetType())
				{
				case nsItem::EnPickupType::Ammo:
					/* 所持している武器の予備弾を補給する。*/
					bPickedUp = stWeaponInventory_.AddReserveAmmoToAll(kPickupAmmoAmount);
					break;

				case nsItem::EnPickupType::Medkit:
					iMedkitCount_++;
					bPickedUp = true;
					break;

				case nsItem::EnPickupType::Grenade:
					iGrenadeCount_++;
					bPickedUp = true;
					break;

				default:
					break;
				}

				/* 拾えなければ物資はその場に残す。*/
				if (!bPickedUp)
					continue;

				/* 拾ったことを通知して、その物資を消す。*/
				PublishGameEvent(nsEvent::EnGameEvent::ItemPickedUp, pPickup->GetPosition());
				DeleteGO(pPickup);
				return;
			}
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
				Vector3 vThrowPos = GetEyePosition();
				vThrowPos += vLook * kMuzzleForward;
				nsItem::Grenade* pGrenade = NewGO<nsItem::Grenade>(0, "grenade");
				pGrenade->Setup(vThrowPos, vLook);
			}
		}


		void Player::UpdateModel()
		{
			/* 移動状態に応じてアニメーションを切り替える。*/
			PlayAnimation(bIsMoving_ ? EnPlayerAnimation::Walk : EnPlayerAnimation::Idle);

			/*
			 * 他人から見た姿のときは、体を「狙っている向き」へ向ける。
			 * どこを狙っているかが味方から見て分かるようにするため(L4D2のサバイバーと同じ)。
			 * 一人称では体を描かないので、進行方向を向いたままでよい。
			 * TODO: 横歩き・後ろ歩きのアニメーションが無いので、真横へ進むと足が滑って見える。
			 *       アニメーションが増えたら移動方向で出し分ける。
			 */
			Quaternion qBody = qRotation_;
			if (enViewMode_ == EnViewMode::ThirdPerson)
				qBody.SetRotationY(fCameraYaw_);

			/* 位置と回転をモデルへ反映して更新する。*/
			stModelRender_.SetPosition(vPosition_);
			stModelRender_.SetRotation(qBody);
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

			/* 見せ方で銃の置き場所が変わる。自分の視点なら画面手前、他人から見た姿なら右手のボーン。*/
			if (enViewMode_ == EnViewMode::ThirdPerson)
				UpdateHandWeaponModel(pWeapon, iType);
			else
				UpdateViewWeaponModel(pWeapon, iType);
		}


		void Player::UpdateViewWeaponModel(nsWeapon::Weapon* pWeapon, int iType)
		{
			/* FPSのビューモデルとして、カメラ(目)基準で画面手前(前・右・下)に銃を置く。*/
			const Vector3 vLook = GetLookDirection();								// 前(ヨー＋ピッチ。上下を向くと銃も追従)
			const Vector3 vRight = { cosf(fCameraYaw_), 0.0f, -sinf(fCameraYaw_) };	// 右(水平・単位)

			/* 揺れとリロード演出の調整値(player.jsonのviewShake / reload)。*/
			const nsData::ViewShakeStatus& stShake = stPlayerStatus_.stViewShake_;
			const nsData::ReloadMotionStatus& stReload = stPlayerStatus_.stReloadMotion_;

			/* 最終スケール = 自動サイズ合わせ × 武器データの微調整倍率(1.0基準)。*/
			const float fScale = aWeaponModelAutoScale_[iType] * pWeapon->GetModelScale();

			/* 銃の「中心」を置きたい位置(目の前・右・下)。目の位置は歩きの上下動を含む。*/
			Vector3 vGunPos = GetEyePosition();
			vGunPos += vLook * pWeapon->GetViewModelForward();	// 前(武器ごと)

			/* 覗き込むほど、銃を画面中央(照準の位置)へ寄せる。*/
			const float fRightOffset = kViewModelRight + (kAdsViewModelRight - kViewModelRight) * fAdsRate_;
			const float fDownOffset = kViewModelDown + (kAdsViewModelDown - kViewModelDown) * fAdsRate_;
			vGunPos += vRight * fRightOffset;				// 右
			vGunPos.y += fDownOffset;						// 下(負)
			vGunPos.y -= kSprintLowerDown * fLowerRate_;	// 走っている間はさらに下げる

			/*
			 * リロードは「マガジンを抜く→挿す→構え直す」の3段階で見せる。
			 * 銃を手前・下へ引いて傾け(抜く)、下で一度突き上げ(挿す)、
			 * 元へ戻して最後に少し行き過ぎる(構え直す)。
			 */
			const float fReloadRate = pWeapon->GetReloadRate();
			float fReloadLower = 0.0f;
			float fReloadInsert = 0.0f;
			float fReloadSettle = 0.0f;
			CalcReloadMotion(fReloadRate, fReloadLower, fReloadInsert, fReloadSettle);

			vGunPos.y -= stReload.fLowerDown_ * fReloadLower;	// 抜く動作で沈める
			vGunPos -= vLook * (stReload.fPullBack_ * fReloadLower);	// 手前へ引き寄せる
			vGunPos -= vRight * (stReload.fPullRight_ * fReloadLower);	// 画面中央へ寄せる
			vGunPos.y += stReload.fInsertUp_ * fReloadInsert;	// 挿し込みの突き上げ
			vGunPos.y += stReload.fSettleUp_ * fReloadSettle;	// 構え直しの行き過ぎ

			vGunPos -= vLook * fWeaponKickBack_;			// 撃った直後は手前へ下がる

			/* 視点を振ったときの遅れ。腕が視点に引きずられているように見せる。*/
			vGunPos += vRight * fSwayRight_;
			vGunPos.y += fSwayUp_;

			/*
			 * 歩行ボブ(銃ぶん)。カメラ自体の上下動は GetEyePosition に含まれているので、
			 * ここで足すのは「体の動きに対して銃がさらに遅れて振れる」ぶん。
			 * 位相をずらしてあるので、カメラと同時に動いて画面上で消えることはない。
			 * 横は1歩に1往復、縦は1歩に2往復＝8の字を描く。
			 */
			{
				const float fBobPhase = fBobTimer_ - stShake.fBobPhaseLag_;
				const float fBobAmp = bIsSprinting_ ? stShake.fBobSprintAmp_ : stShake.fBobWalkAmp_;

				/* 覗き込み中は銃の揺れを完全に止める(カメラ側の揺れは adsSuppressRate ぶんだけ残る)。*/
				const float fBobScale = fBobAmp * fBobWeight_ * (1.0f - fAdsRate_);
				vGunPos += vRight * (sinf(fBobPhase) * fBobScale);
				vGunPos.y += sinf(fBobPhase * 2.0f) * fBobScale * stShake.fBobWeaponUpRate_;
			}

			/* 原点ズレの補正に使う基準の回転(視点の向きだけ)。*/
			Quaternion qBase;
			qBase.SetRotationY(fCameraYaw_);
			qBase.AddRotationX(-fCameraPitch_);

			/* 実際に銃へ与える回転。ここへ演出ぶんを重ねていく。*/
			Quaternion qGun = qBase;
			qGun.AddRotationZ(kAdsRollAngle * fAdsRate_);	// 覗き込むほど銃を横倒しにし、拡大時に視界を塞がないようにする。
			qGun.AddRotationX(kSprintLowerAngle * fLowerRate_);	// 走っている間は銃口を下げて構えを解く。
			qGun.AddRotationX(stReload.fLowerAngle_ * fReloadLower);	// マガジンを抜くあいだは銃口を下げる。
			qGun.AddRotationZ(stReload.fRollAngle_ * fReloadLower);	// 差込口が見えるよう銃を傾ける。
			qGun.AddRotationX(-stReload.fInsertAngle_ * fReloadInsert);	// 挿し込む瞬間だけ銃口が持ち上がる。
			qGun.AddRotationZ(fSwayRight_ * stShake.fSwayRollRate_);		// 視点を振った遅れのぶん傾ける。

			PlaceWeaponModel(iType, vGunPos, qBase, qGun, fScale);
		}


		void Player::UpdateHandWeaponModel(nsWeapon::Weapon* pWeapon, int iType)
		{
			/* 銃は視線の向きに構える。体の向きではなく視線を使うので、狙っている方向がそのまま出る。*/
			const Vector3 vLook = GetLookDirection();
			const Vector3 vRight = { cosf(fCameraYaw_), 0.0f, -sinf(fCameraYaw_) };

			/*
			 * 銃を置く基準は体の右手のボーン。
			 * ボーンのワールド行列は ModelRender::Update が計算しているので、
			 * 体のモデルを更新したあとに呼ぶこと(UpdateModel がその順で呼んでいる)。
			 */
			Vector3 vHandPos = vPosition_;
			vHandPos.y += stPlayerStatus_.fEyeHeight_ * kHandHeightRate;

			if (iRightHandBoneId_ >= 0 && stModelRender_.m_skeleton.IsInited())
			{
				/* ワールド行列の平行移動成分が、その骨のワールド座標。*/
				const Matrix& mHand = stModelRender_.m_skeleton.GetBone(iRightHandBoneId_)->GetWorldMatrix();
				vHandPos.Set(mHand.m[3][0], mHand.m[3][1], mHand.m[3][2]);
			}

			/* 銃ごとにグリップの位置が違うので、武器データのぶんだけずらして手に収める。*/
			vHandPos += vLook * pWeapon->GetHandForward();
			vHandPos += vRight * pWeapon->GetHandRight();
			vHandPos.y += pWeapon->GetHandUp();

			/*
			 * 手に持たせる銃は「世界の中での実寸」で決める。
			 * ビューモデルは画面に大きく映すため誇張しているので、そちらの倍率は使わない。
			 * 体を大きくすると手のボーンの位置も一緒に動くが、銃の大きさは動かないので
			 * ここで体の大きさに見合う長さを指定しておく必要がある。
			 */
			const float fLongestEdge = aWeaponModelLongestEdge_[iType];
			const float fScale = (fLongestEdge > 0.0001f) ? (pWeapon->GetHandLength() / fLongestEdge) : 1.0f;

			/* 銃口が視線を向くように回す(ビューモデルと同じく、ヨーを設定してからピッチを後乗せする)。*/
			Quaternion qGun;
			qGun.SetRotationY(fCameraYaw_);
			qGun.AddRotationX(-fCameraPitch_);

			/* 手に持たせるときは演出の回転を重ねないので、補正の基準も同じ回転でよい。*/
			PlaceWeaponModel(iType, vHandPos, qGun, qGun, fScale);
		}


		void Player::PlaceWeaponModel(int iType, const Vector3& vCenterPos, const Quaternion& qBase, const Quaternion& qRotation, float fScale)
		{
			/*
			 * モデル原点のズレを打ち消す。
			 * 補正には視点の向き(ヨー＋ピッチ)だけを使い、演出でつけた回転は含めない。
			 * 演出の回転まで含めると、リロードのように大きく回したときに銃の位置まで動いてしまう。
			 */
			Vector3 vCenterOffset = aWeaponModelCenter_[iType] * fScale;
			qBase.Apply(vCenterOffset);

			/* 不具合を調べられるよう、実際に置いた位置を控えておく。*/
			vWeaponViewPos_ = vCenterPos - vCenterOffset;

			ModelRender& weaponModel = aWeaponModels_[iType];
			weaponModel.SetPosition(vWeaponViewPos_);
			weaponModel.SetRotation(qRotation);
			weaponModel.SetScale(Vector3(fScale, fScale, fScale));
			weaponModel.Update();
		}


		void Player::CalcBodyModelSize()
		{
			/* 全メッシュの全頂点を走査してローカルAABB(最小・最大)を求める。*/
			Vector3 vMin = { 1e30f, 1e30f, 1e30f };
			Vector3 vMax = { -1e30f, -1e30f, -1e30f };

			stModelRender_.GetModel().GetTkmFile().QueryMeshParts(
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

			/* 頂点が取れなければ測れない。*/
			if (vMax.x < vMin.x)
			{
				fBodyModelSize_ = 0.0f;
				return;
			}

			/* 一番長い辺を大きさとする(モデルの上向き軸に依存しないようにするため)。*/
			const Vector3 vSize = vMax - vMin;
			float fLongest = vSize.x;
			if (vSize.y > fLongest) fLongest = vSize.y;
			if (vSize.z > fLongest) fLongest = vSize.z;

			fBodyModelSize_ = fLongest * stPlayerStatus_.fBodyModelScale_;

			/* デバッグ: 想定の身長(目線の高さ等)と実物の大きさが合っているか確認する。*/
			DebugPrintW(L"[Player] body AABB min(%.1f,%.1f,%.1f) max(%.1f,%.1f,%.1f) size(%.1f,%.1f,%.1f) -> 表示サイズ %.1f (目線の高さ %.1f)\n",
				vMin.x, vMin.y, vMin.z, vMax.x, vMax.y, vMax.z,
				vSize.x, vSize.y, vSize.z, fBodyModelSize_, stPlayerStatus_.fEyeHeight_);
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
				aWeaponModelLongestEdge_[iType] = 0.0f;
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
			aWeaponModelLongestEdge_[iType] = fLongest;

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
					fBleedOutTimer_ = stPlayerStatus_.fBleedOutTime_;
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

			stCharacterStatus_.stHp_.iCurrentHP_ = stPlayerStatus_.iReviveHP_;
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
