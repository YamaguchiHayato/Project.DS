#include "stdafx.h"
#include "CommonEnemy.h"
#include "Src/Actor/Character/Enemy/State/EnemyIdleState.h"
#include "Src/System/GamePause.h"
#include "Src/Actor/Character/Enemy/State/EnemyChaseState.h"
#include "Src/Actor/Character/Enemy/State/EnemyAttackState.h"
#include "Src/Actor/Character/Enemy/State/EnemyDeadState.h"

namespace
{
	const float fAnimInterpolateTime_ = 0.2f; //! アニメ切替の補間時間。
	const Vector3 vPoolParkPosition_ = { 0.0f, -100000.0f, 0.0f };	//! プール待機中の退避先。どの射程・判定半径も届かない位置。
}

namespace nsApp
{
	namespace nsActor
	{
#define TYPE_COMMON CharacterModelType::Common

		void CommonEnemy::InitCharacterModel()
		{
			/* モデルの種類をセットする。*/
			stAnimation_.Initialize(TYPE_COMMON);

			/* 対応するアニメーションを読み込む。*/
			stAnimation_.LoadAnimation();

			/* アニメーションをロードする。*/
			stModel_.LoadCharacterModel(TYPE_COMMON, stAnimation_.GetAnimatiocClip(), stAnimation_.GetAnimationClips());

			/* モデルの大きさをセットする。*/
			stModel_.SetCharacterScale(Vector3::One* 0.01f);

			/* 初期座標をセット。*/
			stModel_.SetPosition(vPosition_);
			stMovement_.SetGravityEnabled(true); //! 重力を有効化。
			stModel_.Update();

			/* 初期のアニメーションをセット。*/
			iPlayingAnimation_ = -1;
			PlayIdle();			
		}


		bool CommonEnemy::Start()
		{
			/* カプセルで壁（PhysicsStaticObject）と当たる。*/
			stMovement_.Init(20.0f, 145.0f, vPosition_);
			
			/* 視線判定の目の高さを設定する。*/
			stSightCheck_.SetEyeHeight(120.0f);

			/* 体力を初期化する。*/
			/* todo 外部Parameter化。*/
			stCharacterStatus_.stHp_.iMaxHP_ = 30;
			stCharacterStatus_.stHp_.iCurrentHP_ = 30;

			/* 本番モデルとアニメーションをロードする。*/
			InitCharacterModel();

			/* 待機アニメから開始する。*/
			PlayIdle();

			/* 遷移樹を所有者に結び、Common 用の枝を組む。*/
			stTransition_.Bind(this);
			stTransition_.BuildCommonTree();

			/* 最初は待機ステートから始める。*/
			pStateMachine_->ChangeState(new EnemyIdleState());
			stTransition_.SetCurrentState(EnEnemyState::Idle);

			/* プール待機状態にする。*/
			if (bPoolInactive_)
				Deactivate();

			else
			{
				fModelAlpha_ = 1.0f;
				stModel_.SetAlpha(fModelAlpha_);
				ApplyModelTransform();

			}

			return true;
		}


		void CommonEnemy::Update()
		{
			/* プールで待機中は更新しない。*/
			if (bPoolInactive_)
				return;

			/* ポーズ中は動かさない。*/
			if (nsSystem::IsGamePaused())
				return;

			/* 死亡以外は攻撃タイマーを進める。*/
			if (stTransition_.GetCurrentState() != EnEnemyState::Death)
				fAttackTimer_ += g_gameTime->GetFrameDeltaTime();

			/* 待機中は Idle タイマーを進める。*/
			if (stTransition_.GetCurrentState() == EnEnemyState::Idle)
				fIdleTimer_ += g_gameTime->GetFrameDeltaTime();

			/* 徘徊中は Wander タイマーを進める。*/
			if (stTransition_.GetCurrentState() == EnEnemyState::Wander)
				fWanderTimer_ += g_gameTime->GetFrameDeltaTime();

			/* ステートマシーンを更新する（遷移は State 内の TryChangeState）。*/
			ICharacter::Update();

			/* 大きさの倍率を設定する。*/
			if (stTransition_.GetCurrentState() != EnEnemyState::Death)
				stModel_.SetCharacterScale(Vector3::One * 0.01f);

			/* 位置をモデルへ反映する。*/
			ApplyModelTransform();
		}


		void CommonEnemy::Render(RenderContext& rc)
		{
			/* プールで待機中は描画しない。*/
			if(bPoolInactive_)
				return;

			/* ICharacter::stModel_ を描画する。*/
			ICharacter::Render(rc);
		}


		void CommonEnemy::UpdateToTargetVector()
		{
			/* 対象が無ければゼロベクトルにする。*/
			if (pTarget_ == nullptr)
			{
				vToTarget_ = Vector3::Zero;
				return;
			}

			/* 高さは無視した対象へのベクトルを作る。*/
			vToTarget_ = pTarget_->GetPosition() - vPosition_;
			vToTarget_.y = 0.0f;
		}


		float CommonEnemy::GetDistanceToTarget()
		{
			/* 対象へのベクトルを更新して長さを返す。*/
			UpdateToTargetVector();
			return vToTarget_.Length();
		}


		bool CommonEnemy::IsTargetInAggroRange()
		{
			/* アグロ円以内なら true。*/
			return GetDistanceToTarget() <= fAggroRange_;
		}


		bool CommonEnemy::IsTargetInAttackRange()
		{
			/* 攻撃距離以内ならtrue。*/
			return GetDistanceToTarget() <= fAttackRange_;
		}


		Vector3 CommonEnemy::MakeEyePosition(const Vector3& vPos) const
		{
			/* 目の高さの座標を作る。*/
			Vector3 vEye = vPos;

			/* 目の高さを加算する。*/
			vEye.y += stSightCheck_.GetEyeHeight();

			/* 目の高さの座標を返す。*/
			return vEye;
		}


		bool CommonEnemy::IsTargetVisible() const
		{
			/* 対象が無ければ視線は通らない。*/
			if (pTarget_ == nullptr)
				return false;

			/* 対象の目の高さまでの視線が通っていればtrue。*/
			return stSightCheck_.HasClearSight(MakeEyePosition(vPosition_), MakeEyePosition(pTarget_->GetPosition()));
		}


		bool CommonEnemy::TryChangeState()
		{
			/* 遷移の判断は Transition に任せる。*/
			return stTransition_.TryChangeState();
		}


		void CommonEnemy::NotifyEnemyState(EnEnemyState enState)
		{
			/* 樹の「今の節」を State の Enter と揃える。*/
			stTransition_.SetCurrentState(enState);
		}


		nsState::StateMachine<Actor>* CommonEnemy::GetStateMachine()
		{
			/* Transition など外部から ChangeState できるように公開する。*/
			return pStateMachine_;
		}


		void CommonEnemy::LookAtTarget()
		{
			/* アグロ円外なら向きを変えない。*/
			if (!IsTargetInAggroRange())
				return;

			/* 対象へのベクトルを更新する。*/
			UpdateToTargetVector();

			/* ベクトルが無ければ向きを変えない。*/
			if (vToTarget_.Length() <= 0.0f)
				return;

			/* 対象の方向を向く。*/
			qLook_.SetRotationY(atan2f(vToTarget_.x, vToTarget_.z));
			stModel_.SettRotation(qLook_);
		}


		void CommonEnemy::MoveToTarget()
		{
			/* 対象が無ければ動かない。*/
			if (pTarget_ == nullptr)
				return;

			/* 対象の方向を向く。*/
			LookAtTarget();

			/* 攻撃距離内ならその場にとどめる。*/
			if (IsTargetInAttackRange())
			{
				/* 攻撃距離内では移動速度をゼロにして、CharacterMovement に計算させる。*/
				vPosition_ = stMovement_.Execute(Vector3::Zero, g_gameTime->GetFrameDeltaTime());
				return;
			}

			/* 目標へ向かう移動計算はCharacterMovementクラスに一任する。*/
			vPosition_ = stMovement_.MoveToward(pTarget_->GetPosition(), fChaseSpeed_, g_gameTime->GetFrameDeltaTime());
		}


		void CommonEnemy::AttackTarget()
		{
			/* 対象が無ければ攻撃しない。*/
			if (pTarget_ == nullptr)
				return;

			/* 対象が死亡していれば攻撃しない。*/
			if (pTarget_->IsDead())
				return;

			/* 対象の方向を向く。*/
			LookAtTarget();

			/* 攻撃間隔が残っているなら撃たない。*/
			if (fAttackTimer_ < fAttackInterval_)
				return;

			/* ダメージを与えてタイマーを戻す。*/
			pTarget_->ApplyDamage(iAttackPower_);
			fAttackTimer_ = 0.0f;
		}


		void CommonEnemy::ApplyModelTransform()
		{
			/* 位置と向きをモデルへ反映する。*/
			stModel_.SetPosition(vPosition_);
			stModel_.SettRotation(qLook_);
			stModel_.Update();
		}


		void CommonEnemy::PlayAnimation(int iAnimationNumber)
		{
			/* 同じアニメなら再生し直さない。*/
			if (iPlayingAnimation_ == iAnimationNumber)
				return;

			/* 指定アニメを再生する。*/
			iPlayingAnimation_ = iAnimationNumber;
			stModel_.PlayAnimation(iAnimationNumber, fAnimInterpolateTime_);
		}


		void CommonEnemy::PlayAnimationList(ANIM_LIST state)
		{

		}


		void CommonEnemy::PlayIdle()
		{
			/* 待機を再生する。*/
			PlayAnimation(stAnimation_.GetBasicAnimationIndex(ANIM_LIST::Idle));
		}


		void CommonEnemy::PlayWalk()
		{
			/* 歩きを再生する。*/
			PlayAnimation(stAnimation_.GetBasicAnimationIndex(ANIM_LIST::Walk));
		}


		void CommonEnemy::PlayRun()
		{
			PlayAnimation(stAnimation_.GetBasicAnimationIndex(ANIM_LIST::Run));
		}


		void CommonEnemy::PlayAttack()
		{
			/* 攻撃を再生する。*/
			PlayAnimation(stAnimation_.GetBasicAnimationIndex(ANIM_LIST::Attack));
		}


		void CommonEnemy::PlayDeath()
		{
			fDeathFadeTimer_ = 0.0f;
			fModelAlpha_ = 1.0f;
			bDeathEffectPlayed_ = false;
			stModel_.SetAlpha(1.0f);

			/* 死亡時の演出用のアニメーションを再生する。*/
			const int iRun = stAnimation_.GetBasicAnimationIndex(ANIM_LIST::Run);
			iPlayingAnimation_ = iRun;
			stModel_.PlayAnimation(iRun, 0.0f); // 補完はなし。
		}


		bool CommonEnemy::IsDeathState() const
		{
			/* 遷移樹の現在節が Death かで判定する。*/
			return stTransition_.GetCurrentState() == EnEnemyState::Death;
		}


		const wchar_t* CommonEnemy::GetCurrentStateName() const
		{
			/* 遷移樹の種別から表示名を返す。*/
			switch (stTransition_.GetCurrentState())
			{
			case EnEnemyState::Idle:
				return L"Idle";
			case EnEnemyState::Wander:
				return L"Wander";
			case EnEnemyState::Chase:
				return L"Chase";
			case EnEnemyState::Attack:
				return L"Attack";
			case EnEnemyState::Death:
				return L"Death";
			}
			return L"Unknown";
		}


		void CommonEnemy::ApplyDamage(int iDamage)
		{
			/* プールへの待機中はダメージを受けない。*/
			if (bPoolInactive_)
				return;

			/* HPを減らす。*/
			IEnemy::ApplyDamage(iDamage);

			/* 死亡していればノックバックしない。*/
			if (IsDead())
				return;

			/* 攻撃者（追跡対象）と反対方向へ下がる準備をする。*/
			if (pTarget_ == nullptr)
				return;

			/* 対象の方向を向く。*/
			vAway_ = vPosition_ - pTarget_->GetPosition();
			vAway_.y = 0.0f;
			if (vAway_.Length() <= 0.0001f)
				return;

			/* 反対方向ベクトルを正規化して速度に変換する。*/
			vAway_.Normalize();
			vKnockBackSpeed_ = vAway_ * fKnockBackPower_;
			fKnockBackTimer_ = fKnockBackDuration_;
			bKnockBackPending_ = true;
			bKnockBackFinished_ = false;
		}


		bool CommonEnemy::IsKnockBackPending() const
		{
			/* ノックバック開始待ちか。*/
			return bKnockBackPending_;
		}


		bool CommonEnemy::IsKnockBackFinished() const
		{
			/* ノックバックが終了したか。*/
			return bKnockBackFinished_;
		}


		void CommonEnemy::BeginKnockBack()
		{
			/* 開始待ちを消費し、終了フラグを戻す。*/
			bKnockBackPending_ = false;
			bKnockBackFinished_ = false;

			/* タイマーが無ければ既定時間を入れる。*/
			if (fKnockBackTimer_ <= 0.0f)
				fKnockBackTimer_ = fKnockBackDuration_;
		}


		void CommonEnemy::ExecuteKnockBack()
		{
			/* 終了済みなら何もしない。*/
			if (bKnockBackFinished_)
				return;

			/* 開始待ちなら何もしない。*/
			const float fDeltaTime = g_gameTime->GetFrameDeltaTime();

			/* 後ずさりを進める。*/
			vPosition_ = stMovement_.Execute(vKnockBackSpeed_, fDeltaTime);
			fKnockBackTimer_ -= fDeltaTime;

			/* 時間が切れたら終了事実を立てる。*/
			if (fKnockBackTimer_ <= 0.0f)
			{
				fKnockBackTimer_ = 0.0f;
				vKnockBackSpeed_ = Vector3::Zero;
				bKnockBackFinished_ = true;
			}
		}


		void CommonEnemy::ExecuteDeth()
		{
			/* すでに消えて居るなら何もしない。*/
			if (IsDeathFadeDone())
				return;
			
			/* 経過時間を更新する。*/
			fDeathFadeTimer_ += g_gameTime->GetFrameDeltaTime();

			/* 1 → 0 へ線形補完する。*/
			fModelAlpha_ = CalcDeathFadeAlpha(fDeathFadeTimer_);
			stModel_.SetAlpha(fModelAlpha_);

			/* フェード完了時(死亡演出)の処理。*/
			if (IsDeathFadeDone())
			{
				/* 透明度を設定する。*/
				stModel_.SetAlpha(0.0f);

				/* プール待機へ戻す。*/
				Deactivate();
			}
		}


		void CommonEnemy::BeginWander()
		{
			/* 徘徊時間をカウント。*/
			fWanderTimer_ = 0.0f;

			/* 歩く方向をランダムに決める。*/
			const float fAngle = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 6.2831853f;
			vWanderDir_.x = sinf(fAngle);
			vWanderDir_.y = 0.0f;
			vWanderDir_.z = cosf(fAngle);

			/* 進行方向を向かせる。*/
			qLook_.SetRotation(Vector3::AxisY, fAngle);
		}


		float CommonEnemy::CalcDeathFadeAlpha(float fTimer) const
		{
			/* 前半は不透明のまま。*/
			if (fTimer < fDeathHoldDuration_)
				return 1.0f;

			/* 後半の演出の長さを計算する。*/
			const auto fFadeLen = fDeathFadeDuration_ - fDeathHoldDuration_;
			if (fFadeLen <= 0.0f)
				return 0.0f;

			auto fTime = (fTimer - fDeathHoldDuration_) / fFadeLen;
			fTime = std::clamp<float>(fTime, 0.0f, 1.0f);

			/* 後半になればなるほど速く消す。*/
			return 1.0f - (fTime * fTime);
		}


		void CommonEnemy::Activate(const Vector3& vPos, ICharacter* pTarget)
		{
			/* 戦場で動かす。*/
			bPoolInactive_ = false;

			/* 呼び出し側から渡された位置・標的を使う。*/
			SetPosition(vPos);
			SetTarget(pTarget);

			/* HP を満タンに戻す。*/
			stCharacterStatus_.stHp_.iCurrentHP_ = stCharacterStatus_.stHp_.iMaxHP_;

			/* 表示と死亡演出用の値を復帰させる。*/
			fModelAlpha_ = 1.0f;
			fDeathFadeTimer_ = 0.0f;
			bDeathEffectPlayed_ = false;
			stModel_.SetAlpha(1.0f);
			stModel_.SetCharacterScale(Vector3::One * 0.01f);

			/* 待機アニメ・待機ステートから始める。*/
			iPlayingAnimation_ = -1;
			PlayIdle();
			pStateMachine_->ChangeState(new EnemyIdleState());
			stTransition_.SetCurrentState(EnEnemyState::Idle);

			/* 位置と向きをモデルへ反映する。*/
			ApplyModelTransform();
		}


		void CommonEnemy::Deactivate()
		{
			/* プール待機へ戻す。*/
			bPoolInactive_ = true;

			/* マップ外へ退散させる。*/
			SetPosition(vPoolParkPosition_);
			ApplyModelTransform();

			/* 演出用の値とフラグを初期化する。*/
			fModelAlpha_ = 0.0f;
			fDeathFadeTimer_ = 0.0f;
			bDeathEffectPlayed_ = false;
			stModel_.SetAlpha(0.0f);

			/* ノックバック値を残さない。*/
			bKnockBackPending_ = false;
			bKnockBackFinished_ = false;

			/* 再生成用のステートを設定する。*/
			iPlayingAnimation_ = -1;
			pStateMachine_->ChangeState(new EnemyIdleState());
			stTransition_.SetCurrentState(EnEnemyState::Idle);
		}
	}
}