#pragma once
#include "Src/Actor/Character/Player/State/PlayerStateBase.h"
#include "Src/Actor/Character/Player/State/AttackState/AttackFireStrategy.h"
#include "Src/Actor/Character/Player/State/AttackState/AttackReloadStrategy.h"
#include "Src/Actor/Character/Player/State/AttackState/AttackSwitchStrategy.h"
#include "Src/Actor/Character/Player/State/AttackState/AttackShoveStrategy.h"
#include "Src/Actor/Character/Player/State/AttackState/AttackAdsStrategy.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   PlayerAttackState.h
		 * @brief  操作キャラの銃戦闘ステート。
		 * @details 敵の AttackState とは別。将来ここで射撃・ADS などの挙動をまとめる。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/20
		 */
		class PlayerAttackState : public PlayerStateBase
		{
		public:
			/**
			 * @brief 銃戦闘ステートの更新処理。Player::Update() から呼ばれる。
			 * @param pPlayer 武器を持つクラス。
			 * @param deltaTime　前フレームからの経過時間(秒)。
			 */
			static void TickWeapon(Player* pPlayer, float deltaTime);


		protected:
			/**
			 * @brief このステートの種別を返す。
			 * @return Attack。
			 */
			EnPlayerState GetStateKind() const override;

			/**
			 * @brief 銃戦闘開始時の差分処理。
			 */
			void OnEnter() override;

			/**
			 * @brief 銃戦闘更新時の差分処理。
			 */
			void OnUpdate() override;


		private:
			static AttackFireStrategy fireStrategy_;     //! 発射戦略。
			static AttackReloadStrategy reloadStrategy_; //! リロード戦略。	
			static AttackSwitchStrategy switchStrategy_; //! 武器切り替え戦略。
			static AttackShoveStrategy shoveStrategy_;   //! 突き飛ばし戦略。
			static AttackAdsStrategy adsStrategy_; //! 覗き込み戦略。
		};
	}
}