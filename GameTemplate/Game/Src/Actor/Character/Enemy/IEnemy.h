#pragma once
#include "Src/Actor/Character/ICharacter.h"
#include "Src/Actor/Character/Enemy/Transition/EnEnemyState.h"
#include "Src/StateMachine/StateMachine.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   IEnemy.h
		 * @brief  敵が State / Transition に見せる共通の口。
		 * @details 具体種の差はここに出さない。メンバデータは持たない。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/26
		 */
		class IEnemy : public ICharacter
		{
		public:
			/* デストラクタ。*/
			virtual ~IEnemy() = default;

			/**
			 * @brief 遷移樹に判定を依頼する。
			 * @return 切り替えたら true。
			 */
			virtual bool TryChangeState() = 0;

			/**
			 * @brief 現在のステート種別を遷移樹へ通知する。
			 * @param enState 入ったステート種別。
			 */
			virtual void NotifyEnemyState(EnEnemyState enState) = 0;

			/**
			 * @brief ステートマシーンを取得する。
			 * @return ステートマシーン（非所有）。
			 */
			virtual nsState::StateMachine<Actor>* GetStateMachine() = 0;

			/**
			 * @brief 対象が死亡しているか。
			 * @return 死亡していれば true。
			 */
			virtual bool IsTargetDead() const = 0;

			/**
			 * @brief アグロ円内か。
			 * @return 円内なら true。
			 */
			virtual bool IsTargetInAggroRange() = 0;

			/**
			 * @brief 攻撃範囲内か。
			 * @return 範囲内なら true。
			 */
			virtual bool IsTargetInAttackRange() = 0;

			/**
			 * @brief 視線が通っているか。
			 * @return 通っていれば true。
			 */
			virtual bool IsTargetVisible() const = 0;

			/**
			 * @brief 待機アニメを再生する。
			 */
			virtual void PlayIdle() = 0;

			/**
			 * @brief 歩きアニメを再生する。
			 */
			virtual void PlayWalk() = 0;

			/**
			 * @brief 対象へ移動する。
			 */
			virtual void MoveToTarget() = 0;

			/**
			 * @brief すぐ攻撃できる状態にする。
			 */
			virtual void ReadyAttack() = 0;

			/**
			 * @brief 対象を攻撃する。
			 */
			virtual void AttackTarget() = 0;

			/**
			 * @brief ノックバック開始待ちか。
			 * @return 待ちなら true。
			 */
			virtual bool IsKnockBackPending() const = 0;

			/**
			 * @brief ノックバックが終了したか。
			 * @return 終了していれば true。
			 */
			virtual bool IsKnockBackFinished() const = 0;

			/**
			 * @brief ノックバック開始時の準備をする。
			 */
			virtual void BeginKnockBack() = 0;

			/**
			 * @brief ノックバック移動を1フレーム進める。
			 */
			virtual void ExecuteKnockBack() = 0;
		};
	}
}