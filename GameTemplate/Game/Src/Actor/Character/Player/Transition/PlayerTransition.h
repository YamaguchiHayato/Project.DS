#pragma once

#include "Src/Actor/Actor.h"
#include "Src/Actor/Character/Common/CharacterTransition.h"
#include "Src/Actor/Character/Player/Transition/EnPlayerState.h"
#include "Src/Actor/Character/Player/Transition/PlayerSituation.h"
#include "Src/Actor/Character/Player/Transition/PlayerTransitionEdge.h"
#include "Src/StateMachine/IState.h"
#include <vector>

namespace nsApp
{
	namespace nsActor
	{
		class Player;

		/**
		 * @file   PlayerTransition.h
		 * @brief  操作キャラのステート遷移を管轄するクラス。
		 * @details 現在ステートを節とし、登録された枝を上から順に見て次ステートを決める。
		 *          State 実体の生成は構築時に登録した関数テーブルで行う。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16: 新規作成日。
		 */
		class PlayerTransition : public CharacterTransition
		{
		public:
			/* ステート実体を生成する関数の型。*/
			using PlayerStateCreator = nsState::IState<Actor>* (*)();


		public:
			/* コンストラクタとデストラクタ。*/
			PlayerTransition() = default;
			virtual ~PlayerTransition() = default;


		public:
			/**
			 * @brief 所有者となるプレイヤーを結びつける。
			 * @param pPlayer 対象のプレイヤー（非所有）。
			 */
			void Bind(Player* pPlayer);

			/**
			 * @brief 移動／ポーズ／銃戦闘用の遷移樹と生成関数を構築する。
			 */
			void BuildTree();

			/**
			 * @brief 現在のステート種別を記録する。
			 * @param enState 現在のステート種別。
			 * @details 各 State の Enter から呼び、樹の「今の節」を同期する。
			 */
			void SetCurrentState(EnPlayerState enState);

			/**
			 * @brief 現在のステート種別を取得する。
			 * @return 現在のステート種別。
			 */
			inline EnPlayerState GetCurrentState() const
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
			void RegisterState(EnPlayerState enState, PlayerStateCreator fnCreate);

			/**
			 * @brief 所有者から1フレーム分の事実を集める。
			 * @return 集めた Situation。
			 */
			PlayerSituation MakeSituation() const;


		private:
			Player* pPlayer_ = nullptr;	//! 所有者（非所有）。
			EnPlayerState enCurrentState_ = EnPlayerState::Idle; //! 現在のステート種別。
			std::vector<PlayerTransitionEdge> edges_; //! 遷移樹の枝一覧。
			PlayerStateCreator creators_[5] = {}; //! 種別ごとの生成関数（EnPlayerState の個数）。
		};
	}
}