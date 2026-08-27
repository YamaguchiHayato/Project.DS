#include "stdafx.h"
#include "CommonEnemy.h"
#include "Src/Actor/Character/Enemy/State/EnemyIdleState.h"
#include "Src/System/GamePause.h"
#include "Src/Actor/Character/Enemy/State/EnemyChaseState.h"
#include "Src/Actor/Character/Enemy/State/EnemyAttackState.h"
#include "Src/Actor/Character/Enemy/State/EnemyDeadState.h"

namespace
{
	const char* sUnityChanModelPath_ = "Assets/modelData/unityChan.tkm";	//! Unityちゃんのモデルパス。
}

namespace nsApp
{
	namespace nsActor
	{
		bool CommonEnemy::Start()
		{
			/* カプセルで壁（PhysicsStaticObject）と当たる。*/
			stMovement_.Init(20.0f, 70.0f, vPosition_);

			/* 視線判定の目の高さを設定する。*/
			stSightCheck_.SetEyeHeight(120.0f);

			/* 体力を初期化する。*/
			/* todo 外部Parameter化。*/
			stCharacterStatus_.stHp_.iMaxHP_ = 30;
			stCharacterStatus_.stHp_.iCurrentHP_ = 30;

			/* 仮モデルをロードする。*/
			stModelRender_.Init(sUnityChanModelPath_, nullptr, 0, enModelUpAxisZ);
			stModelRender_.SetPosition(vPosition_);

			/* 初期位置をモデルへ反映する。*/
			ApplyModelTransform();

			/* 遷移樹を所有者に結び、Common 用の枝を組む。*/
			stTransition_.Bind(this);
			stTransition_.BuildCommonTree();

			/* 最初は待機ステートから始める。*/
			pStateMachine_->ChangeState(new EnemyIdleState());
			stTransition_.SetCurrentState(EnEnemyState::Idle);

			return true;
		}


		void CommonEnemy::Update()
		{
			/* ポーズ中は動かさない。*/
			if (nsSystem::IsGamePaused())
				return;

			/* 死亡以外は攻撃タイマーを進める。*/
			if (stTransition_.GetCurrentState() != EnEnemyState::Death)
				fAttackTimer_ += g_gameTime->GetFrameDeltaTime();

			/* ステートマシーンを更新する（遷移は State 内の TryChangeState）。*/
			ICharacter::Update();

			/* 位置をモデルへ反映する。*/
			ApplyModelTransform();
		}


		void CommonEnemy::Render(RenderContext& rc)
		{
			/* 仮モデルを描画する。*/
			stModelRender_.Draw(rc);
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
			stModelRender_.SetRotation(qLook_);
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
			vPosition_ = stMovement_.MoveToward(pTarget_->GetPosition(),fChaseSpeed_,g_gameTime->GetFrameDeltaTime());
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
			/* 位置をモデルへ反映する。*/
			stModelRender_.SetPosition(vPosition_);
			stModelRender_.Update();
		}


		void CommonEnemy::PlayAnimation(int iAnimationNumber)
		{
			/* 同じアニメなら再生し直さない。*/
			if (iPlayingAnimation_ == iAnimationNumber)
				return;

			/* 指定アニメを再生する。*/
			iPlayingAnimation_ = iAnimationNumber;
			stModelRender_.PlayAnimation(iAnimationNumber, 0.2f);
		}


		void CommonEnemy::PlayIdle()
		{
			/* 待機を再生する。*/
			PlayAnimation(0);
		}


		void CommonEnemy::PlayWalk()
		{
			/* 歩きを再生する。*/
			PlayAnimation(1);
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
			vKnockBackSpeed_ = vAway_* fKnockBackPower_;
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
	}
}