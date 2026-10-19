#include "stdafx.h"
#include "PlayerUpdate.h"
#include "Src/Actor/Character/Player/Player.h"
#include "Src/Actor/Character/Player/PlayerIntent.h"
#include <algorithm>

namespace
{
	const float ADS_SENSITIVITY_RATE = 0.55f; //! 覗き込み中の視点移動の倍率(拡大しているぶん狙いを合わせやすくする)。
	const float MAX_PITCH = 1.4f; //! カメラピッチの上下限(rad, ≈±80度)。真上/真下での破綻防止。
	const float ADS_TRANSITION_RATE = 12.0f; //! 覗き込みの切り替わる速さ(大きいほど素早く構える)。
	const float SPREAD_RECOVER_RATE = 4.0f; //! 連射で広がった拡散が収まる速さ。
	const float SPRINT_LOWER_RATE = 8.0f; //! 銃を下げる/戻す速さ。
	const float RECOIL_RECOVER_RATE = 6.0f;		//! 反動が戻る速さ(大きいほど早く元へ戻る)。
	const float KICK_BACK_RECOVER_RATE = 12.0f;	//! 銃のキックバックが戻る速さ。
}


namespace nsApp
{
	namespace nsActor
	{
		void PlayerUpdate::UpdateLook(const PlayerIntent& stIntent)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* マウスの横移動量でカメラのヨー(左右)を更新する。*/
			pPlayer_->AddCameraYaw(stIntent.fLookYawDelta_ * (1.0f - (1.0f - ADS_SENSITIVITY_RATE) * pPlayer_->GetAdsRate()));

			/* マウスの縦移動量でカメラのピッチ(上下)を更新し、真上/真下付近で止める。*/
			pPlayer_->AddCameraPitch(stIntent.fLookPitchDelta_ * (1.0f - (1.0f - ADS_SENSITIVITY_RATE) * pPlayer_->GetAdsRate()));

			/* 真上の限界を超えたら止める。*/
			if (pPlayer_->GetCameraPitch() > MAX_PITCH)
				pPlayer_->SetCameraPitch(MAX_PITCH);
			/* 真下の限界を超えたら止める。*/
			else if (pPlayer_->GetCameraPitch() < -MAX_PITCH)
				pPlayer_->SetCameraPitch(-MAX_PITCH);
		}


		void PlayerUpdate::UpdateAds(float fDeltaTime, const PlayerIntent& stIntent)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 覗き込みの度合いを目標へ寄せる。*/
			ApproachAds(fDeltaTime, stIntent);
			ApproachLower(fDeltaTime);
			RecoverSpread(fDeltaTime);
		}


		void PlayerUpdate::UpdateStanceSpread(float fDeltaTime)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* いまの姿勢と移動から目標の倍率を決める。しゃがみ＞走り＞歩き＞立ち止まりの優先で見る。*/
			workStanceTarget_ = 1.0f;

			/* しゃがみ＞走り＞歩き＞立ち止まりの優先で倍率を決める。*/
			if (pPlayer_->IsCrouching())
				workStanceTarget_ = pPlayer_->GetPlayerStatus().stAccuracy_.fCrouchSpreadRate_;

			/* 走り＞歩き＞立ち止まりの優先で倍率を決める。*/
			else if (pPlayer_->IsMoving() && pPlayer_->IsSprinting())
				workStanceTarget_ = pPlayer_->GetPlayerStatus().stAccuracy_.fSprintSpreadRate_;

			/* 歩き＞立ち止まりの優先で倍率を決める。*/
			else if (pPlayer_->IsMoving())
				workStanceTarget_ = pPlayer_->GetPlayerStatus().stAccuracy_.fMoveSpreadRate_;

			/* 目標へ滑らかに寄せる(クロスヘアがカクッと跳ねないように)。*/
			workBlendRate_ = fDeltaTime * pPlayer_->GetPlayerStatus().stAccuracy_.fSpreadFollowRate_;
			if (workBlendRate_ > 1.0f)
				workBlendRate_ = 1.0f;

			/* 姿勢拡散の目標倍率へ滑らかに寄せる。*/
			pPlayer_->AddStanceSpreadRate((workStanceTarget_ - pPlayer_->GetStanceSpreadRate()) * workBlendRate_);
		}


		void PlayerUpdate::UpdateRecoil(float fDeltaTime)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 跳ね上がった視点を徐々に元へ戻す。*/
			workBlendRate_ = fDeltaTime * RECOIL_RECOVER_RATE;
			if (workBlendRate_ > 1.0f)
				workBlendRate_ = 1.0f;

			/* 反動で上がった視点を徐々に元へ戻す。*/
			pPlayer_->AddRecoilPitch(-pPlayer_->GetRecoilPitch() * workBlendRate_);
			pPlayer_->AddRecoilYaw(-pPlayer_->GetRecoilYaw() * workBlendRate_);

			/* 手前へ下がった銃を徐々に元の位置へ戻す。*/
			workBlendRate_ = fDeltaTime * KICK_BACK_RECOVER_RATE;
			if (workBlendRate_ > 1.0f)
				workBlendRate_ = 1.0f;

			/* キックバックを徐々に元の位置へ戻す。*/
			pPlayer_->AddWeaponKickBack(-pPlayer_->GetWeaponKickBack() * workBlendRate_);
		}


		void PlayerUpdate::UpdateViewSway(float fDeltaTime, const PlayerIntent& stIntent)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 覗き込んでいる間は狙いが定まるよう、揺れ全体を抑える。*/
			workShakeRate_ = 1.0f - pPlayer_->GetAdsRate() * pPlayer_->GetPlayerStatus().stViewShake_.fAdsSuppressRate_;

			/* 銃の揺れを進める。*/
			UpdateWeaponSway(fDeltaTime, stIntent);
			UpdateBobWeight(fDeltaTime);
			UpdateViewBob();
			UpdateStrafeRoll(fDeltaTime, stIntent);
		}


		void PlayerUpdate::UpdateWeaponSway(float fDeltaTime, const PlayerIntent& stIntent)
		{
			/*
			 * 視点を振った量を「銃の遅れ」に変える。
			 * 振った向きと逆へずれてから中央へ戻るので、腕が視点に引きずられているように見える。
			 */
			pPlayer_->AddSwayRight(-stIntent.fLookYawDelta_ * pPlayer_->GetPlayerStatus().stViewShake_.fSwayGain_ * workShakeRate_);
			pPlayer_->AddSwayUp(-stIntent.fLookPitchDelta_ * pPlayer_->GetPlayerStatus().stViewShake_.fSwayGain_ * workShakeRate_);

			/* 離れ過ぎないよう上限を掛ける。*/
			pPlayer_->SetSwayRight(std::clamp(pPlayer_->GetSwayRight(), -pPlayer_->GetPlayerStatus().stViewShake_.fSwayMaxOffset_, pPlayer_->GetPlayerStatus().stViewShake_.fSwayMaxOffset_));
			pPlayer_->SetSwayUp(std::clamp(pPlayer_->GetSwayUp(), -pPlayer_->GetPlayerStatus().stViewShake_.fSwayMaxOffset_, pPlayer_->GetPlayerStatus().stViewShake_.fSwayMaxOffset_));

			/* 腕が追いついたぶんだけ中央へ戻す。*/
			workBlendRate_ = std::clamp(fDeltaTime * pPlayer_->GetPlayerStatus().stViewShake_.fSwayRecoverRate_, 0.0f, 1.0f);
			pPlayer_->AddSwayRight(-pPlayer_->GetSwayRight() * workBlendRate_);
			pPlayer_->AddSwayUp(-pPlayer_->GetSwayUp() * workBlendRate_);
		}


		void PlayerUpdate::UpdateBobWeight(float fDeltaTime)
		{
			/* 歩行ボブの重み。止まった瞬間にピタッと消えないよう補間する。*/
			workBlendRate_ = std::clamp(fDeltaTime * pPlayer_->GetPlayerStatus().stViewShake_.fBobWeightRate_, 0.0f, 1.0f);
			pPlayer_->AddBobWeight(((pPlayer_->IsMoving() ? 1.0f : 0.0f) - pPlayer_->GetBobWeight()) * workBlendRate_);

			/* 歩いている間だけ位相を進める(止まると波が止まり、重みで消えていく)。*/
			if (pPlayer_->IsMoving())
				pPlayer_->AddBobTimer(fDeltaTime * (pPlayer_->IsSprinting() ? pPlayer_->GetPlayerStatus().stViewShake_.fBobSprintSpeed_ : pPlayer_->GetPlayerStatus().stViewShake_.fBobWalkSpeed_));
		}


		void PlayerUpdate::UpdateViewBob()
		{
			/*
			 * カメラ自体を歩きに合わせて上下させる。
			 * 銃だけを揺らすと「銃が揺れている」ようにしか見えないので、体のほうを動かす。
			 * 1歩で2回沈むので、上下は位相の2倍で回す。
			 */
			workBobShake_ = pPlayer_->GetBobWeight() * workShakeRate_;
			pPlayer_->SetViewBobHeight(sinf(pPlayer_->GetBobTimer() * 2.0f) * (pPlayer_->IsSprinting() ? pPlayer_->GetPlayerStatus().stViewShake_.fViewBobHeightSprint_ : pPlayer_->GetPlayerStatus().stViewShake_.fViewBobHeightWalk_) * workBobShake_);

			/* 左右の踏み込みに合わせてカメラをわずかに傾ける。*/
			workBobRoll_ = sinf(pPlayer_->GetBobTimer()) * (pPlayer_->IsSprinting() ? pPlayer_->GetPlayerStatus().stViewShake_.fViewBobRollSprint_ : pPlayer_->GetPlayerStatus().stViewShake_.fViewBobRollWalk_) * workBobShake_;
		}


		void PlayerUpdate::UpdateStrafeRoll(float fDeltaTime, const PlayerIntent& stIntent)
		{
			/* 横移動に合わせた傾き(ストレイフロール)。入力を鈍らせてから使う。*/
			workBlendRate_ = std::clamp(fDeltaTime * pPlayer_->GetPlayerStatus().stViewShake_.fStrafeFollowRate_, 0.0f, 1.0f);
			pPlayer_->AddStrafeAxis(((pPlayer_->IsMoving() ? stIntent.vMoveAxis_.x : 0.0f) - pPlayer_->GetStrafeAxis()) * workBlendRate_);
			pPlayer_->SetViewRoll(workBobRoll_ - pPlayer_->GetStrafeAxis() * pPlayer_->GetPlayerStatus().stViewShake_.fStrafeRollAngle_ * workShakeRate_);
		}


		void PlayerUpdate::UpdateHealth(float fDeltaTime)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 一時体力は時間で減っていく(鎮痛剤の効き目が切れていく)。*/
			if (pPlayer_->GetTempHP() > 0.0f)
				pPlayer_->SetTempHP((std::max)(0.0f, pPlayer_->GetTempHP() - pPlayer_->GetPlayerStatus().stHealthRule_.fTempHPDecayRate_ * fDeltaTime));

			/* アドレナリンの効果時間。*/
			if (pPlayer_->GetAdrenalineTimer() > 0.0f)
				pPlayer_->SetAdrenalineTimer((std::max)(0.0f, pPlayer_->GetAdrenalineTimer() - fDeltaTime));
		}


		void PlayerUpdate::UpdateEyeHeight(float fDeltaTime)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 倒れている(死んでいる)ときは低く、しゃがんでいれば中くらい、立っていれば通常の高さ。*/
			workStanceTarget_ = pPlayer_->GetPlayerStatus().stHealthRule_.fDownEyeHeight_;
			if (pPlayer_->GetLifeState() == EnLifeState::Alive)
				workStanceTarget_ = pPlayer_->IsCrouching() ? pPlayer_->GetPlayerStatus().fCrouchEyeHeight_ : pPlayer_->GetPlayerStatus().fEyeHeight_;

			/* 目標へ滑らかに寄せる。倒れ込む・起き上がる動きに見える。*/
			workBlendRate_ = std::clamp(fDeltaTime * pPlayer_->GetPlayerStatus().stHealthRule_.fEyeHeightFollowRate_, 0.0f, 1.0f);
			pPlayer_->AddEyeHeight((workStanceTarget_ - pPlayer_->GetEyeHeight()) * workBlendRate_);
		}


		void PlayerUpdate::UpdateLifeState(float fDeltaTime)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			switch (pPlayer_->GetLifeState())
			{
			case EnLifeState::Alive:
				/* 通常は ApplyDamage の中でダウンへ移るが、別の経路でHPが0になった場合もここで拾う。*/
				if (pPlayer_->IsDead())
				{
					if (pPlayer_->IsBlackAndWhite())
						pPlayer_->EnterDead();
					else
						pPlayer_->EnterDown();
				}
				break;

			case EnLifeState::Down:
				/* 出血で残り時間が減り、尽きたら死亡する(ソロは救助者がいないので通常ここへ至る)。*/
				pPlayer_->AddBleedOutTimer(-fDeltaTime);
				if (pPlayer_->GetBleedOutRemain() <= 0.0f)
					pPlayer_->EnterDead();
				break;

			case EnLifeState::Dead:
				/* 何もしない。*/
				break;
			}
		}


		void PlayerUpdate::UpdateMove(float fDeltaTime, const PlayerIntent& stIntent)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* カメラの旋回角から、地面基準の前方・右方向ベクトルを作る。*/
			workCameraForward_ = { sinf(pPlayer_->GetCameraYaw()), 0.0f, cosf(pPlayer_->GetCameraYaw()) };
			workCameraRight_ = { workCameraForward_.z, 0.0f, -workCameraForward_.x };

			/* しゃがみ(Ctrl)。止まっていても姿勢は変わるので、移動入力より先に見る。*/
			pPlayer_->SetCrouching(stIntent.bCrouchPress_);

			/* 移動入力が無ければ座標も向きも変えない(向きは維持してアイドルへ)。*/
			if (stIntent.vMoveAxis_.x == 0.0f && stIntent.vMoveAxis_.z == 0.0f)
			{
				pPlayer_->SetMoving(false);
				pPlayer_->SetSprinting(false);

				/* 止まっていても移動処理は回す(壁との押し戻しを効かせるため)。*/
				pPlayer_->StandStillMove(fDeltaTime);
				return;
			}

			/* カメラ相対の移動方向を作る(W=カメラ奥, S=手前, A/D=左右)。*/
			workMoveDir_ = workCameraForward_ * stIntent.vMoveAxis_.z + workCameraRight_ * stIntent.vMoveAxis_.x;
			workMoveDir_.y = 0.0f;
			workMoveDir_.Normalize();

			/* スプリント(Shift)中は移動速度を上げて進む。しゃがんだままでは走れない。*/
			pPlayer_->SetSprinting(stIntent.bSprintPress_ && !pPlayer_->IsCrouching());
			workMoveSpeed_ = pPlayer_->IsSprinting() ? (pPlayer_->GetPlayerStatus().fMoveSpeed_ * pPlayer_->GetPlayerStatus().fSprintRate_) : pPlayer_->GetPlayerStatus().fMoveSpeed_;

			/* しゃがみ歩きはゆっくり。*/
			if (pPlayer_->IsCrouching())
				workMoveSpeed_ *= pPlayer_->GetPlayerStatus().fCrouchSpeedRate_;

			/* 体力が少ないと足を引きずり、アドレナリンが効いていれば速くなる。*/
			workMoveSpeed_ *= pPlayer_->GetMoveSpeedRate();

			/* 覗き込み中はゆっくり歩く。*/
			if (pPlayer_->GetWeaponInventory().GetCurrentWeapon() != nullptr && pPlayer_->GetAdsRate() > 0.0f)
				workMoveSpeed_ *= 1.0f + (pPlayer_->GetWeaponInventory().GetCurrentWeapon()->GetAdsSpeedRate() - 1.0f) * pPlayer_->GetAdsRate();

			/* 移動計算と壁との押し戻しは CharacterMovement クラスに一任する。*/
			pPlayer_->MoveOnDirection(workMoveDir_, workMoveSpeed_, fDeltaTime);

			/*
			 * 原神風:常に進行方向を向いて前歩きする(Sで下がっても振り向くので後ろ歩きにならない)。
			 * TODO: 今は瞬時に向きが変わる。滑らかにするなら現在の向きから目標の向きへ補間する。
			 */
			pPlayer_->FaceMoveDirection(workMoveDir_);
			pPlayer_->SetMoving(true);
		}


		void PlayerUpdate::ApproachAds(float fDeltaTime, const PlayerIntent& stIntent)
		{
			/* 走っている間は構えられない(Apex同様、スプリントを優先する)。*/
			/* 覗き込みの度合いを目標へ滑らかに近づける。*/
			workBlendRate_ = fDeltaTime * ADS_TRANSITION_RATE;

			/* 1.0fを超えないようにする。*/
			if (workBlendRate_ > 1.0f)
				workBlendRate_ = 1.0f;

			/* 覗き込みボタンが押されていて、かつスプリントしていない場合は1.0f、そうでなければ0.0fを目標値とする。*/
			pPlayer_->AddAdsRate((((stIntent.bAdsPress_ && !pPlayer_->IsSprinting()) ? 1.0f : 0.0f) - pPlayer_->GetAdsRate()) * workBlendRate_);
		}


		void PlayerUpdate::ApproachLower(float fDeltaTime)
		{
			/* 走っている間は銃を下げ、やめたら構え直す。*/
			workBlendRate_ = fDeltaTime * SPRINT_LOWER_RATE;
			if (workBlendRate_ > 1.0f)
				workBlendRate_ = 1.0f;

			/* 走っている間と、メディキットを使っている間は銃を下げる(両手が塞がっている)。*/
			pPlayer_->AddLowerRate(((((pPlayer_->IsSprinting() && pPlayer_->IsMoving()) || pPlayer_->IsHealing()) ? 1.0f : 0.0f) - pPlayer_->GetLowerRate()) * workBlendRate_);
		}


		void PlayerUpdate::RecoverSpread(float fDeltaTime)
		{
			/* 連射で広がった拡散を時間で収める。*/
			workBlendRate_ = fDeltaTime * SPREAD_RECOVER_RATE;
			if (workBlendRate_ > 1.0f)
				workBlendRate_ = 1.0f;

			/* 拡散が0.0fより大きい場合は、時間で収める。*/
			pPlayer_->AddSpreadShot(-pPlayer_->GetSpreadShot() * workBlendRate_);
		}
	}
}