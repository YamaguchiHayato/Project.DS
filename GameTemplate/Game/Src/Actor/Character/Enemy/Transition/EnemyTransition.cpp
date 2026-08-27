#include "stdafx.h"
#include "EnemyTransition.h"
#include "Src/Actor/Character/Enemy/IEnemy.h"
#include "Src/Actor/Character/Enemy/State/EnemyIdleState.h"
#include "Src/Actor/Character/Enemy/State/EnemyChaseState.h"
#include "Src/Actor/Character/Enemy/State/EnemyAttackState.h"
#include "Src/Actor/Character/Enemy/State/EnemyDeadState.h"
#include "Src/Actor/Character/Enemy/State/EnemyKnockBackState.h"

namespace nsApp
{
	namespace nsActor
	{
		namespace
		{
			/**
			 * @brief Idle ステートを生成する。
			 * @return 生成したステート。
			 */
			nsState::IState<Actor>* CreateIdleState()
			{
				return new EnemyIdleState();
			}

			/**
			 * @brief Chase ステートを生成する。
			 * @return 生成したステート。
			 */
			nsState::IState<Actor>* CreateChaseState()
			{
				return new EnemyChaseState();
			}

			/**
			 * @brief Attack ステートを生成する。
			 * @return 生成したステート。
			 */
			nsState::IState<Actor>* CreateAttackState()
			{
				return new EnemyAttackState();
			}

			/**
			 * @brief Death ステートを生成する。
			 * @return 生成したステート。
			 */
			nsState::IState<Actor>* CreateDeathState()
			{
				return new EnemyDeathState();
			}


			/**
			 * @brief KnockBack ステートを生成する。
			 * @return 生成したステート。
			 */
			nsState::IState<Actor>* CreateKnockBackState()
			{
				return new EnemyKnockBackState();
			}
		}


		void EnemyTransition::Bind(IEnemy* pEnemy)
		{
			/* ChangeState に使う所有者を保持する。*/
			pEnemy_ = pEnemy;
		}


		void EnemyTransition::RegisterState(EnEnemyState enState, EnemyStateCreator fnCreate)
		{
			/* 種別を添え字にして生成関数を登録する。*/
			aCreators_[static_cast<size_t>(enState)] = fnCreate;
		}


		void EnemyTransition::BuildCommonTree()
		{
			/* Common 雑魚用の樹を組み直す。同じ From では先に書いた枝を優先する。*/
			aEdges_.clear();

			/* Idle：死亡していれば Death。*/
			aEdges_.push_back({ EnEnemyState::Idle, enEnemyFact_SelfDead, EnEnemyState::Death });

			/* Idle：ノックバック要求があれば KnockBack。*/
			aEdges_.push_back({ EnEnemyState::Idle, enEnemyFact_NeedKnockBack, EnEnemyState::KnockBack });

			/* Idle：アグロ円内かつ視線ありなら Chase。*/
			aEdges_.push_back({ EnEnemyState::Idle, enEnemyFact_InAggro | enEnemyFact_Visible, EnEnemyState::Chase });

			/* Chase：死亡していれば Death。*/
			aEdges_.push_back({ EnEnemyState::Chase, enEnemyFact_SelfDead, EnEnemyState::Death });

			/* Chase：ノックバック要求があれば KnockBack。*/
			aEdges_.push_back({ EnEnemyState::Chase, enEnemyFact_NeedKnockBack, EnEnemyState::KnockBack });

			/* Chase：対象が死亡していれば Idle。*/
			aEdges_.push_back({ EnEnemyState::Chase, enEnemyFact_TargetDead, EnEnemyState::Idle });

			/* Chase：アグロ円外なら Idle（視線は問わない）。*/
			aEdges_.push_back({ EnEnemyState::Chase, enEnemyFact_NotInAggro, EnEnemyState::Idle });

			/* Chase：攻撃範囲内なら Attack。*/
			aEdges_.push_back({ EnEnemyState::Chase, enEnemyFact_InAttack, EnEnemyState::Attack });

			/* Attack：死亡していれば Death。*/
			aEdges_.push_back({ EnEnemyState::Attack, enEnemyFact_SelfDead, EnEnemyState::Death });

			/* Attack：ノックバック要求があれば KnockBack。*/
			aEdges_.push_back({ EnEnemyState::Attack, enEnemyFact_NeedKnockBack, EnEnemyState::KnockBack });

			/* Attack：対象が死亡していれば Idle。*/
			aEdges_.push_back({ EnEnemyState::Attack, enEnemyFact_TargetDead, EnEnemyState::Idle });

			/* Attack：アグロ円外なら Idle。*/
			aEdges_.push_back({ EnEnemyState::Attack, enEnemyFact_NotInAggro, EnEnemyState::Idle });

			/* Attack：攻撃範囲外なら Chase。*/
			aEdges_.push_back({ EnEnemyState::Attack, enEnemyFact_NotInAttack, EnEnemyState::Chase });

			/* KnockBack：死亡していれば Death。*/
			aEdges_.push_back({ EnEnemyState::KnockBack, enEnemyFact_SelfDead, EnEnemyState::Death });

			/* KnockBack：終了かつアグロ円内なら Chase。*/
			aEdges_.push_back({ EnEnemyState::KnockBack, enEnemyFact_KnockBackDone | enEnemyFact_InAggro, EnEnemyState::Chase });

			/* KnockBack：終了かつアグロ円外なら Idle。*/
			aEdges_.push_back({ EnEnemyState::KnockBack, enEnemyFact_KnockBackDone | enEnemyFact_NotInAggro, EnEnemyState::Idle });

			/* 派生の new は構築時に閉じる。実行時の TryChangeState では switch しない。*/
			RegisterState(EnEnemyState::Idle, CreateIdleState);
			RegisterState(EnEnemyState::Chase, CreateChaseState);
			RegisterState(EnEnemyState::Attack, CreateAttackState);
			RegisterState(EnEnemyState::Death, CreateDeathState);
			RegisterState(EnEnemyState::KnockBack, CreateKnockBackState);
		}


		void EnemyTransition::SetCurrentState(EnEnemyState enState)
		{
			/* 各 State の Enter から呼ばれ、樹の「今の節」を同期する。*/
			enCurrentState_ = enState;
		}


		bool EnemyTransition::TryChangeState()
		{
			/* 所有者が無ければ遷移できない。*/
			if (pEnemy_ == nullptr)
				return false;

			/* 事実を集め、今の節から出る枝を上から順に見る。*/
			const EnemySituation stSituation = MakeSituation();

			for (const EnemyTransitionEdge& stEdge : aEdges_)
			{
				/* 出発節が今のステートと違う枝はスキップする。*/
				if (stEdge.enFrom_ != enCurrentState_)
					continue;

				/* 必要な事実が揃っていなければ次の枝へ。*/
				if (!stSituation.Matches(stEdge.uNeedFacts_))
					continue;

				/* 行き先が今と同じなら切り替えない。*/
				if (stEdge.enTo_ == enCurrentState_)
					return false;

				/* 登録テーブルから生成関数を取り出す。*/
				EnemyStateCreator fnCreate = aCreators_[static_cast<size_t>(stEdge.enTo_)];
				if (fnCreate == nullptr)
					return false;

				/* 節を更新してからステート実体を切り替える。*/
				enCurrentState_ = stEdge.enTo_;
				pEnemy_->GetStateMachine()->ChangeState(fnCreate());
				return true;
			}

			/* 条件に合う枝が無ければ現状維持。*/
			return false;
		}


		EnemySituation EnemyTransition::MakeSituation() const
		{
			/* 所有者から事実だけを読む。遷移ルールはここに書かない。*/
			EnemySituation stSituation;
			stSituation.Clear();
			stSituation.SetSelfDead(pEnemy_->IsDead());
			stSituation.SetTargetDead(pEnemy_->IsTargetDead());
			stSituation.SetInAggro(pEnemy_->IsTargetInAggroRange());
			stSituation.SetVisible(pEnemy_->IsTargetVisible());
			stSituation.SetInAttack(pEnemy_->IsTargetInAttackRange());
			stSituation.SetNeedKnockBack(pEnemy_->IsKnockBackPending());
			stSituation.SetKnockBackDone(pEnemy_->IsKnockBackFinished());
			return stSituation;
		}
	}
}