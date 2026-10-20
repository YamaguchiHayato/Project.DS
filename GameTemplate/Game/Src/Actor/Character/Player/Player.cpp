#include "stdafx.h"
#include "Player.h"
#include "LocalPlayerController.h"
#include "Weapon.h"
#include "Tracer.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Event/EventBus.h"
#include "Src/System/GamePause.h"
#include "Src/Item/PipeBomb.h"
#include "Src/Item/Pickup.h"
#include "Src/Data/PlayerStatusTable.h"
#include "Src/Data/WeaponStatusTable.h"
#include "Src/Combat/HitBoxSet.h"
#include "Src/System/ModelBounds.h"
#include <algorithm>

#include "Src/Actor/Character/Common/CharacterAnimation.h"
#include "Src/Actor/Character/Player/State/PlayerAttackState.h"
#include "Src/Actor/Character/Player/State/PlayerIdleState.h"

namespace
{
	const float kMuzzleForward = 50.0f; //! 銃口の前方オフセット(照準方向)。
	const float kMuzzleRight = 40.0f; //! トレーサー始点を目線から右へずらす量(線を斜めに見せる)。
	const float kMuzzleDown = -32.0f; //! トレーサー始点を目線から下へずらす量(負で下)。
	const float kEnemyCenterHeightForShove = 85.0f;	//! 突き飛ばしの手応えを出す高さ(敵の体の中心)。
	const int kShoveDamage = 10;			//! 突き飛ばしで与えるダメージ。押し返しが主で、削るのはおまけ。
	const float kCapsuleRadius = 25.0f;		//! 移動用カプセルの半径(壁との押し戻しに使う)。
	const float kCapsuleHeight = 120.0f;	//! 移動用カプセルの高さ。
	const float kHealEffectHeight = 80.0f;	//! 回復の演出を出す高さ(胸のあたり)。

	/* 即効アイテムの名前(player.json の startQuickItem に書く文字列)。*/
	const char* sQuickItemPillsName_ = "Pills";				//! 鎮痛剤。
	const char* sQuickItemAdrenalineName_ = "Adrenaline";	//! アドレナリン。

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
	 * @brief 即効アイテムの名前を種類へ変換する。
	 * @param sName 名前("Pills" / "Adrenaline")。
	 * @return 種類。どれでもなければ None。
	 */
	nsApp::nsActor::EnQuickItem ParseQuickItem(const std::string& sName)
	{
		if (sName == sQuickItemPillsName_)
			return nsApp::nsActor::EnQuickItem::Pills;

		if (sName == sQuickItemAdrenalineName_)
			return nsApp::nsActor::EnQuickItem::Adrenaline;

		return nsApp::nsActor::EnQuickItem::None;
	}
}

namespace nsApp
{
	namespace nsActor
	{
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
			stMovement_.SetGravityEnabled(true);

			/* モデルとアニメーションを読み込む。*/
			systems_.Init(this, stPlayerStatus_, vPosition_);

			/* HPと所持アイテム数をステータス表から設定する。*/
			stCharacterStatus_.stHp_.iCurrentHP_ = stPlayerStatus_.iMaxHP_;
			stCharacterStatus_.stHp_.iMaxHP_ = stPlayerStatus_.iMaxHP_;
			iMedkitCount_ = stPlayerStatus_.iMedkitCount_;
			iPipeBombCount_ = stPlayerStatus_.iPipeBombCount_;
			enQuickItem_ = ParseQuickItem(stPlayerStatus_.sStartQuickItem_);

			/* 目の高さは立ち姿から始める(ダウンすると低くなる)。*/
			fEyeHeight_ = stPlayerStatus_.fEyeHeight_;

			/* イベント発行先(勝敗管理などが購読)を取得する。デバッグシーン等、無い場合は発行しない。*/
			pEventBus_ = FindGO<nsEvent::EventBus>("eventBus");

			/* 初期所持武器を登録する。マウスホイールで順番に切り替わる。*/
			stWeaponInventory_.AddWeapon(nsWeapon::EnWeaponType::Handgun);
			stWeaponInventory_.AddWeapon(nsWeapon::EnWeaponType::AssaultRifle);

			/* ステートマシンを初期化する。*/
			stTransition_.Bind(this);
			stTransition_.BuildTree();
			pStateMachine_->ChangeState(new PlayerIdleState());
			stTransition_.SetCurrentState(EnPlayerState::Idle);
		
			return true;
		}


		void Player::Update()
		{
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
				/* ポーズ中は描画だけする。*/
				systems_.Present();
				return;
			}

			/* 2.視点を回す。倒れていても首は回せる(ダウン中にピストルで狙うため)。*/
			systems_.GetUpdate().UpdateLook(stIntent_);

			/* 3.覗き込みと拡散、反動、体力の時間経過、目の高さを進める。*/
			systems_.GetUpdate().UpdateStanceSpread(fDeltaTime);
			systems_.GetUpdate().UpdateRecoil(fDeltaTime);
			systems_.GetUpdate().UpdateHealth(fDeltaTime);
			systems_.GetUpdate().UpdateEyeHeight(fDeltaTime);	

			/* 4.生命状態を更新する(ダウン中は出血が進み、尽きると死亡)。*/
			systems_.GetUpdate().UpdateLifeState(fDeltaTime);

			/* 5.生命状態ごとにできることが違う。*/
			switch (enLifeState_)
			{
			case EnLifeState::Alive:
				/* 生きているので、移動・射撃・アイテム使用などができる。*/
				systems_.GetUpdate().UpdateMove(fDeltaTime, stIntent_);
				systems_.GetUpdate().UpdateViewSway(fDeltaTime, stIntent_);

				/* 移動の結果から揺れを作る。射撃の起点(目の位置)にも効くので武器より先に更新する。*/
				PlayerAttackState::TickWeapon(this, fDeltaTime);
				UpdateItems(fDeltaTime);
				UpdateAction();
				break;

			case EnLifeState::Down:
				/* 倒れているので動けないが、本家と同じくサブ武器(ピストル)でなら撃てる。*/
				bIsMoving_ = false;
				bIsSprinting_ = false;
				bIsCrouching_ = false;
				systems_.GetUpdate().UpdateViewSway(fDeltaTime, stIntent_);
				PlayerAttackState::TickWeapon(this, fDeltaTime);
				break;

			case EnLifeState::Dead:
				/* 何もできない。武器のクールダウン等だけ進め、揺れは収まっていく。*/
				bIsMoving_ = false;
				bIsSprinting_ = false;
				bIsCrouching_ = false;
				stWeaponInventory_.Update(fDeltaTime);
				systems_.GetUpdate().UpdateViewSway(fDeltaTime, stIntent_);
				break;
			}

			/* ICharacter(Actor)の共通更新処理(ステートマシン更新等)。*/
			ICharacter::Update();
			systems_.Present();
		}


		void Player::Render(RenderContext& rc)
		{
			/* モデルを描画する。*/
			systems_.Render(rc);
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


		float Player::GetAdsZoomRate()
		{
			/* 武器が無ければ画角を変えない。*/
			nsWeapon::Weapon* pWeapon = stWeaponInventory_.GetCurrentWeapon();
			if (pWeapon == nullptr)
				return 1.0f;

			return pWeapon->GetAdsZoomRate();
		}


		void Player::DealBulletDamage(CommonEnemy* pEnemy, int iDamage, const Vector3& vHitPoint, const Vector3& vShotDir, bool bHeadShot)
		{
			/* 倍率が小さくても、当てたのに0ダメージにはしない。*/
			if (iDamage < 1)
				iDamage = 1;

			pEnemy->ApplyDamage(iDamage);

			/* 命中の演出(当たった位置に出す)。*/
			PublishGameEvent(nsEvent::EnGameEvent::BulletHit, vHitPoint, vShotDir, iDamage, bHeadShot);

			/* 倒したら撃破エフェクト＋撃破イベントを出して退場させる。*/
			if (!pEnemy->IsDead())
				return;

			/* 撃破の閃光は胸のあたり(身長の半分)に出す。*/
			Vector3 vKillPos = pEnemy->GetPosition();
			vKillPos.y += nsCombat::HitBoxSet::GetShared(CharacterModelType::Common).GetHeight() * 0.5f;
			PublishGameEvent(nsEvent::EnGameEvent::EnemyKilled, vKillPos);

			DeleteGO(pEnemy);
		}


		void Player::EnterDown()
		{
			/*
			 * 恒久HPは1に留める。0にすると敵が「倒した」と見なして攻撃をやめてしまい、
			 * 倒れているところを殴られ続ける本家の緊張感が出ないため。
			 */
			stCharacterStatus_.stHp_.iCurrentHP_ = 1;
			fTempHP_ = 0.0f;

			enLifeState_ = EnLifeState::Down;
			fBleedOutTimer_ = stPlayerStatus_.fBleedOutTime_;

			/* 途中だった回復は無かったことになる。*/
			bIsHealing_ = false;
			fMedkitProgress_ = 0.0f;
			bIsMoving_ = false;
			bIsSprinting_ = false;

			/* 倒れているあいだはサブ武器(ピストル)しか撃てない。*/
			stWeaponInventory_.SwitchToSlot(nsWeapon::EnWeaponSlot::Sub);

			PublishGameEvent(nsEvent::EnGameEvent::PlayerDowned);
		}


		void Player::EnterDead()
		{
			stCharacterStatus_.stHp_.iCurrentHP_ = 0;
			fTempHP_ = 0.0f;
			fBleedOutTimer_ = 0.0f;
			bIsHealing_ = false;
			fMedkitProgress_ = 0.0f;
			enLifeState_ = EnLifeState::Dead;

			PublishGameEvent(nsEvent::EnGameEvent::PlayerDead);
		}


		void Player::Revive()
		{
			/* ダウン中のみ救助可能(将来の味方/BOTが呼ぶ想定)。*/
			if (enLifeState_ != EnLifeState::Down)
				return;

			/*
			 * 本家と同じく、恒久HPはわずかに戻るだけで、残りは時間で減る一時体力で補う。
			 * メディキットを使わずに復帰した回数を数え、上限に達すると白黒(次のダウンで死亡)になる。
			 */
			stCharacterStatus_.stHp_.iCurrentHP_ = stPlayerStatus_.iReviveHP_;
			fTempHP_ = 0.0f;
			AddTempHP(stPlayerStatus_.stHealthRule_.iReviveTempHP_);
			iReviveCount_++;

			enLifeState_ = EnLifeState::Alive;
			fBleedOutTimer_ = 0.0f;
			PublishGameEvent(nsEvent::EnGameEvent::PlayerRevived);
		}


		void Player::DebugRestockItems()
		{
			iMedkitCount_++;
			iPipeBombCount_++;

			/* 即効アイテムは1つしか持てないので、空いていれば鎮痛剤を入れる。*/
			if (enQuickItem_ == EnQuickItem::None)
				enQuickItem_ = EnQuickItem::Pills;
		}


		void Player::ApplyDamage(int iDamage)
		{
			/* 死んでいれば何も起きない。0以下のダメージも無視する。*/
			if (enLifeState_ == EnLifeState::Dead || iDamage <= 0)
				return;

			/* 攻撃を受けたことを通知する(画面の赤い幕など)。一時体力で受け止めた場合も痛みは伝える。*/
			PublishGameEvent(nsEvent::EnGameEvent::PlayerDamaged, vPosition_, Vector3::Zero, iDamage);

			/* 回復の途中で殴られると手が止まり、やり直しになる。*/
			bIsHealing_ = false;
			fMedkitProgress_ = 0.0f;

			/* ダウン中は体力ではなく出血時間が削られる。尽きたときの死亡は UpdateLifeState が拾う。*/
			if (enLifeState_ == EnLifeState::Down)
			{
				fBleedOutTimer_ -= static_cast<float>(iDamage) * stPlayerStatus_.stHealthRule_.fDownDamageTimeRate_;
				return;
			}

			/* 一時体力から先に削る。*/
			float fRemain = static_cast<float>(iDamage);
			if (fTempHP_ > 0.0f)
			{
				const float fAbsorb = (fTempHP_ < fRemain) ? fTempHP_ : fRemain;
				fTempHP_ -= fAbsorb;
				fRemain -= fAbsorb;
			}

			/* 残りを恒久HPから引く。端数は切り上げて、受け止め切れなかったぶんは必ず届くようにする。*/
			const int iPermanentDamage = static_cast<int>(ceilf(fRemain));
			if (iPermanentDamage <= 0)
				return;

			ICharacter::ApplyDamage(iPermanentDamage);

			/* 恒久HPが尽きたらダウン。白黒なら救助の余地が無いのでそのまま死亡する。*/
			if (IsDead())
			{
				if (IsBlackAndWhite())
					EnterDead();
				else
					EnterDown();
			}
		}


		void Player::AddTempHP(int iAmount)
		{
			fTempHP_ += static_cast<float>(iAmount);

			/* 合計が最大HPを超えないよう切り詰める。*/
			const float fMaxTemp = static_cast<float>(GetMaxHP() - GetCurrentHP());
			if (fTempHP_ > fMaxTemp)
				fTempHP_ = fMaxTemp;

			if (fTempHP_ < 0.0f)
				fTempHP_ = 0.0f;
		}


		float Player::GetMoveSpeedRate() const
		{
			const nsData::HealthRuleStatus& stRule = stPlayerStatus_.stHealthRule_;

			/* アドレナリンが効いていれば痛みを忘れて速く動ける。*/
			if (IsAdrenalineActive())
				return stRule.fAdrenalineSpeedRate_;

			/* 合計HPが少ないほど足が遅くなる(瀕死→負傷歩行→通常)。*/
			const int iTotalHP = GetTotalHP();
			if (iTotalHP <= stRule.iCriticalHP_)
				return stRule.fCriticalSpeedRate_;

			if (iTotalHP < stRule.iLimpHP_)
				return stRule.fLimpSpeedRate_;

			return 1.0f;
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