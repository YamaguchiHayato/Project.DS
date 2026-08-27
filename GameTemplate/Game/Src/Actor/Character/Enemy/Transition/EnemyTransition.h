#pragma once

#include "Src/Actor/Actor.h"
#include "Src/Actor/Character/Common/CharacterTransition.h"
#include "Src/Actor/Character/Enemy/Transition/EnEnemyState.h"
#include "Src/Actor/Character/Enemy/Transition/EnemySituation.h"
#include "Src/Actor/Character/Enemy/Transition/EnemyTransitionEdge.h"
#include "Src/StateMachine/IState.h"
#include <vector>

namespace nsApp
{
	namespace nsActor
	{
		/* 前方宣言。ヘッダー同士の循環参照を避ける。*/
		class IEnemy;

		/**
		 * @file   EnemyTransition.h
		 * @brief  ゾンビ全体のステート遷移を管轄するクラス。
		 * @details 現在ステートを節とし、登録された枝を上から順に見て次ステートを決める。
		 *          State 実体の生成は構築時に登録した関数テーブルで行い、実行時に switch しない。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/26
		 */
		class EnemyTransition : public CharacterTransition
		{
		public:
			/**
			 * @brief ステート実体を生成する関数の型。
			 */
			using EnemyStateCreator = nsState::IState<Actor>* (*)();


		public:
			/* コンストラクタとデストラクタ。*/
			EnemyTransition() = default;
			virtual ~EnemyTransition() = default;


		public:
			/**
			 * @brief 所有者となる敵を結びつける。
			 * @param pEnemy 対象の敵（非所有）。
			 */
			void Bind(IEnemy* pEnemy);

			/**
			 * @brief Common 雑魚用の遷移樹と生成関数を構築する。
			 */
			void BuildCommonTree();

			/**
			 * @brief 現在のステート種別を記録する。
			 * @param enState 現在のステート種別。
			 * @details 各 State の Enter から呼び、樹の「今の節」を同期する。
			 */
			void SetCurrentState(EnEnemyState enState);

			/**
			 * @brief 現在のステート種別を取得する。
			 * @return 現在のステート種別。
			 */
			inline EnEnemyState GetCurrentState() const
			{
				return enCurrentState_;
			}

			/**
			 * @brief 樹に沿って判定し、必要ならステートを切り替える。
			 * @return 切り替えたら true。
			 */
			bool TryChangeState() override;


		private:
			/**
			 * @brief ステート種別と生成関数を対応づける。
			 * @param enState 登録する種別。
			 * @param fnCreate 実体を new する関数。
			 */
			void RegisterState(EnEnemyState enState, EnemyStateCreator fnCreate);

			/**
			 * @brief 所有者から1フレーム分の事実を集める。
			 * @return 集めた Situation。
			 */
			EnemySituation MakeSituation() const;
			

		private:
			IEnemy* pEnemy_ = nullptr; //! 所有者（非所有）。
			EnEnemyState enCurrentState_ = EnEnemyState::Idle; //! 現在のステート種別。
			std::vector<EnemyTransitionEdge> aEdges_; //! 遷移樹の枝一覧。
			EnemyStateCreator aCreators_[5] = {}; //! 種別ごとの生成関数。
		};
	}
}