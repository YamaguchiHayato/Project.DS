#include "stdafx.h"
#include "BomberEnemy.h"
#include "Src/Actor/Character/Enemy/State/EnemyIdleState.h"
#include "Src/System/GamePause.h"
#include "Src/Actor/Character/Enemy/State/EnemyChaseState.h"
#include "Src/Actor/Character/Enemy/State/EnemyAttackState.h"
#include "Src/Actor/Character/Enemy/State/EnemyDeadState.h"
#include "Src/Data/EnemyStatusTable.h"

namespace
{
	const float fAnimInterpolateTime_ = 0.2f; //! アニメ切替の補間時間。
	const Vector3 vPoolParkPosition_ = { 0.0f, -100000.0f, 0.0f }; //! プール待機中の退避先。
}

namespace nsApp
{
	namespace nsActor
	{
#define TYPE_BOMBER CharacterModelType::Bomber


		void BomberEnemy::InitCharacterModel()
		{
			/* モデルの種類をセットする。*/
			stAnimation_.Initialize(TYPE_BOMBER);

			/* 対応するアニメーションを読み込む。*/
			stAnimation_.LoadAnimation();

			/* アニメーションをロードする。*/
			stModel_.LoadCharacterModel(TYPE_BOMBER, stAnimation_.GetAnimatiocClip(), stAnimation_.GetAnimationClips());

			/* モデルの大きさをセットする。*/
			stModel_.SetCharacterScale(Vector3::One * 0.01f);

			/* 初期座標をセット。*/
			stModel_.SetPosition(vPosition_);
			stMovement_.SetGravityEnabled(true);
			stModel_.Update();

			/* 初期のアニメーションをセット。*/
			iPlayingAnimation_ = -1;
			PlayIdle();
		}


		bool BomberEnemy::Start()
		{
			/* カプセルで壁（PhysicsStaticObject）と当たる。*/
			stMovement_.Init(20.0f, 145.0f, vPosition_);

			/* 視線判定の目の高さを設定する。*/
			stSightCheck_.SetEyeHeight(130.0f);

			/* 体力を初期化する。*/
			stStatus_ = nsData::EnemyStatusTable::Get(nsActor::EnEnemyType::Bomber);
			stCharacterStatus_.stHp_.iMaxHP_ = stStatus_.iMaxHp_;
			stCharacterStatus_.stHp_.iCurrentHP_ = stStatus_.iMaxHp_;

			/* 本番モデルとアニメーションをロードする。*/
			InitCharacterModel();

			/* 待機アニメから開始する。*/
			PlayIdle();

			/* 遷移樹を所有者に結び、Bomber 用の枝を組む。*/
			stTransition_.Bind(this);
			stTransition_.BuildCommonTree();

			/* 最初は待機ステートから始める。*/
			pStateMachine_->ChangeState(new EnemyIdleState());
			stTransition_.SetCurrentState(EnEnemyState::Idle);
			stModel_.SetCharacterScale(Vector3::One);


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


		void BomberEnemy::Update()
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

			/* ステートマシーンを更新する。*/
			ICharacter::Update();

			/* 移動計算を実行する。*/
			const EnEnemyState enState = stTransition_.GetCurrentState();
			if (enState == EnEnemyState::Idle || enState == EnEnemyState::Wander || enState == EnEnemyState::Attack)
				vPosition_ = stMovement_.Execute(Vector3::Zero, g_gameTime->GetFrameDeltaTime());

			/* 大きさの倍率を設定する。*/
			if (stTransition_.GetCurrentState() != EnEnemyState::Death)
				stModel_.SetCharacterScale(Vector3::One);

			/* 重力を付与。*/
			stMovement_.SetGravityEnabled(true);

			/* 位置をモデルへ反映する。*/
			ApplyModelTransform();
		}


		void BomberEnemy::Render(RenderContext& rc)
		{
			/* プールで待機中は描画しない。*/
			if (bPoolInactive_)
				return;

			/* ICharacter::stModel_ を描画する。*/
			ICharacter::Render(rc);
		}


		void BomberEnemy::UpdateToTargetVector()
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


		float BomberEnemy::GetDistanceToTarget()
		{
			/* 対象へのベクトルを更新して長さを返す。*/
			UpdateToTargetVector();
			return vToTarget_.Length();
		}


		bool BomberEnemy::IsTargetInAggroRange()
		{
			/* アグロ円以内なら true。*/
			return GetDistanceToTarget() <= fAggroRange_;
		}


		bool BomberEnemy::IsTargetInAttackRange()
		{
			/* 自爆距離以内なら true。*/
			return GetDistanceToTarget() <= fAttackRange_;
		}


		Vector3 BomberEnemy::MakeEyePosition(const Vector3& vPos) const
		{
			/* 目の高さの座標を作る。*/
			Vector3 vEye = vPos;
			vEye.y += stSightCheck_.GetEyeHeight();
			return vEye;
		}


		bool BomberEnemy::IsTargetVisible() const
		{
			/* 対象が無ければ視線は通らない。*/
			if (pTarget_ == nullptr)
				return false;

			/* 対象の目の高さまでの視線が通っていれば true。*/
			return stSightCheck_.HasClearSight(MakeEyePosition(vPosition_), MakeEyePosition(pTarget_->GetPosition()));
		}


		bool BomberEnemy::TryChangeState()
		{
			/* 遷移の判断は Transition に任せる。*/
			return stTransition_.TryChangeState();
		}


		void BomberEnemy::NotifyEnemyState(EnEnemyState enState)
		{
			/* 樹の「今の節」を State の Enter と揃える。*/
			stTransition_.SetCurrentState(enState);
		}


		nsState::StateMachine<Actor>* BomberEnemy::GetStateMachine()
		{
			/* Transition など外部から ChangeState できるように公開する。*/
			return pStateMachine_;
		}


		void BomberEnemy::LookAtTarget()
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


		void BomberEnemy::MoveToTarget()
		{
			/* 対象が無ければ動かない。*/
			if (pTarget_ == nullptr)
				return;

			/* 対象の方向を向く。*/
			LookAtTarget();

			/* 自爆距離内ならその場にとどめる。*/
			if (IsTargetInAttackRange())
			{
				vPosition_ = stMovement_.Execute(Vector3::Zero, g_gameTime->GetFrameDeltaTime());
				return;
			}

			/* 目標へ向かう移動計算は CharacterMovement に一任する。*/
			vPosition_ = stMovement_.MoveToward(pTarget_->GetPosition(), fChaseSpeed_, g_gameTime->GetFrameDeltaTime());
		}


		void BomberEnemy::AttackTarget()
		{
			/* 対象が無ければ自爆しない。*/
			if (pTarget_ == nullptr || pTarget_->IsDead())
				return;

			/* 対象の方向を向く。*/
			LookAtTarget();

			/* 攻撃アニメが再生中なら自爆しない。*/
			if (stModel_.IsPlayAnimation())
				return;

			/* インターバルが残っているなら待つ。*/
			if (fAttackTimer_ < fAttackInterval_)
				return;

			/* 爆風を出し、自分も死ぬ。*/
			Explode();
			KillSelfAfterExplode();
			fAttackTimer_ = 0.0f;
		}


		void BomberEnemy::Explode()
		{
			/* すでに爆発済みなら何もしない。*/
			if (bExploded_)
				return;

			bExploded_ = true;

			/* 対象がいなければ爆風ダメージは出さない。*/
			if (pTarget_ == nullptr)
				return;

			/* 爆発半径外ならダメージを出さない。*/
			if (GetDistanceToTarget() > fExplodeRange_)
				return;

			/* 範囲内の対象へダメージを出す。*/
			pTarget_->ApplyDamage(iExplodeDamage_);
		}


		void BomberEnemy::KillSelfAfterExplode()
		{
			/* すでに死亡していれば何もしない。*/
			if (IsDead())
				return;

			/* HP をゼロにして死亡遷移へ渡す。*/
			stCharacterStatus_.stHp_.iCurrentHP_ = 0;
		}


		void BomberEnemy::ApplyModelTransform()
		{
			/* 位置と向きをモデルへ反映する。*/
			stModel_.SetPosition(vPosition_);
			stModel_.SettRotation(qLook_);
			stModel_.Update();
		}


		void BomberEnemy::PlayAnimation(int iAnimationNumber)
		{
			/* 同じアニメなら再生し直さない。*/
			if (iPlayingAnimation_ == iAnimationNumber)
				return;

			/* 指定アニメを再生する。*/
			iPlayingAnimation_ = iAnimationNumber;
			stModel_.PlayAnimation(iAnimationNumber, fAnimInterpolateTime_);
		}


		void BomberEnemy::PlayIdle()
		{
			/* Idle.max を再生する。*/
			PlayAnimation(stAnimation_.GetBasicAnimationIndex(ANIM_LIST::Idle));
		}


		void BomberEnemy::PlayWalk()
		{
			/* Walk.max は Bomber に無い。何もしない。*/
		}


		void BomberEnemy::PlayRun()
		{
			/* Run.max を再生する。*/
			PlayAnimation(stAnimation_.GetBasicAnimationIndex(ANIM_LIST::Run));
		}


		void BomberEnemy::PlayAttack()
		{
			/* Roar.max を再生する（Attack エントリ）。*/
			PlayAnimation(stAnimation_.GetBasicAnimationIndex(ANIM_LIST::Attack));
		}


		void BomberEnemy::PlayHit()
		{
			/* Hit.max を再生する。*/
			PlayAnimation(stAnimation_.GetBasicAnimationIndex(ANIM_LIST::Hit));
		}


		void BomberEnemy::PlayDeath()
		{
			Explode(); /* 未爆発のとき用（撃破死など）*/

			fDeathFadeTimer_ = 0.0f;
			fModelAlpha_ = 1.0f;
			bDeathEffectPlayed_ = false;
			stModel_.SetAlpha(1.0f);

			/* 自爆済みなら死亡アニメは飛ばしてフェードだけ。*/
			if (bExploded_)
			{
				iPlayingAnimation_ = -1;
				return;
			}

			const int iDeath = stAnimation_.GetBasicAnimationIndex(ANIM_LIST::Death);
			iPlayingAnimation_ = iDeath;
			stModel_.PlayAnimation(iDeath, fAnimInterpolateTime_); /* 0 より補間ありの方が安全 */
		}


		void BomberEnemy::BeginWander()
		{
			/* 徘徊はしない。*/
		}


		bool BomberEnemy::IsDeathState() const
		{
			/* 遷移樹の現在節が Death かで判定する。*/
			return stTransition_.GetCurrentState() == EnEnemyState::Death;
		}


		const wchar_t* BomberEnemy::GetCurrentStateName() const
		{
			/* 遷移樹の種別から表示名を返す。*/
			switch (stTransition_.GetCurrentState())
			{
			case EnEnemyState::Idle:
				return L"Idle";
			case EnEnemyState::Chase:
				return L"Chase";
			case EnEnemyState::Attack:
				return L"Roar";
			case EnEnemyState::KnockBack:
				return L"Hit";
			case EnEnemyState::Death:
				return L"Deth";
			default:
				return L"Unknown";
			}
		}


		void BomberEnemy::ApplyDamage(int iDamage)
		{
			/* プールへの待機中はダメージを受けない。*/
			if (bPoolInactive_)
				return;

			/* HP を減らす。*/
			IEnemy::ApplyDamage(iDamage);

			/* 死亡していればノックバックしない。*/
			if (IsDead())
				return;

			/* 攻撃者（追跡対象）と反対方向へ下がる準備をする。*/
			if (pTarget_ == nullptr)
				return;

			vAway_ = vPosition_ - pTarget_->GetPosition();
			vAway_.y = 0.0f;
			if (vAway_.Length() <= 0.0001f)
				return;

			vAway_.Normalize();
			vKnockBackSpeed_ = vAway_ * fKnockBackPower_;
			fKnockBackTimer_ = fKnockBackDuration_;
			bKnockBackPending_ = true;
			bKnockBackFinished_ = false;
		}


		bool BomberEnemy::IsKnockBackPending() const
		{
			return bKnockBackPending_;
		}


		bool BomberEnemy::IsKnockBackFinished() const
		{
			return bKnockBackFinished_;
		}


		void BomberEnemy::BeginKnockBack()
		{
			/* 開始待ちを消費し、終了フラグを戻す。*/
			bKnockBackPending_ = false;
			bKnockBackFinished_ = false;

			/* Hit.max を再生する。*/
			PlayHit();

			/* タイマーが無ければ既定時間を入れる。*/
			if (fKnockBackTimer_ <= 0.0f)
				fKnockBackTimer_ = fKnockBackDuration_;
		}


		void BomberEnemy::ExecuteKnockBack()
		{
			/* 終了済みなら何もしない。*/
			if (bKnockBackFinished_)
				return;

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


		void BomberEnemy::ExecuteDeth()
		{
			/* すでに消えているなら何もしない。*/
			if (IsDeathFadeDone())
				return;

			/* 経過時間を更新する。*/
			fDeathFadeTimer_ += g_gameTime->GetFrameDeltaTime();

			/* 1 → 0 へ線形補完する。*/
			fModelAlpha_ = CalcDeathFadeAlpha(fDeathFadeTimer_);
			stModel_.SetAlpha(fModelAlpha_);

			/* フェード完了時の処理。*/
			if (IsDeathFadeDone())
			{
				stModel_.SetAlpha(0.0f);
				Deactivate();
			}
		}


		float BomberEnemy::CalcDeathFadeAlpha(float fTimer) const
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


		void BomberEnemy::Activate(const Vector3& vPos, ICharacter* pTarget)
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
			bExploded_ = false;
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


		void BomberEnemy::Deactivate()
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
			bExploded_ = false;
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