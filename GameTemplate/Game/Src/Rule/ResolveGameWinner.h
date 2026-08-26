#pragma once
#include "Src/Event/GameEvent.h"

namespace nsApp
{
	namespace nsEvent
	{
		/* 前方宣言。*/
		class EventBus;
	}

	namespace nsRule
	{
		/**
		 * @file   ResolveGameWinner.h
		 * @brief  ソロの勝敗を一元管理する(State)。EventBus を購読し、到達で勝利/死亡で敗北へ遷移する。
		 *         シーン遷移自体は行わず「勝敗の状態」だけを持つ(遷移は InGameScene が橋渡し)。
		 * @author Izumida Kiryu
		 * @date   2026/08/21
		 */

		/**
		 * @enum  EnGamePhase
		 * @brief 試合の進行状態。
		 */
		enum class EnGamePhase : uint8_t
		{
			Playing,	//! 進行中。
			Won,		//! 勝利確定。
			Lost,		//! 敗北確定。
		};

		class ResolveGameWinner : public IGameObject, public nsEvent::IGameEventListener
		{
		public:
			/* コンストラクタとデストラクタ。*/
			ResolveGameWinner() = default;
			virtual ~ResolveGameWinner();


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override {}


		public:
			/**
			 * @brief イベントを受けて勝敗を更新する(Observer)。
			 * @param stEvent 受け取ったイベント。
			 */
			void OnGameEvent(const nsEvent::GameEvent& stEvent) override;

			/**
			 * @brief 勝敗が決着したか。
			 * @return 進行中でなければ true。
			 */
			inline bool IsOver() const { return enPhase_ != EnGamePhase::Playing; }

			/**
			 * @brief 勝利で決着したか。
			 * @return 勝利なら true。
			 */
			inline bool IsWin() const { return enPhase_ == EnGamePhase::Won; }

			/**
			 * @brief 現在の進行状態を取得する。
			 * @return 進行状態。
			 */
			inline EnGamePhase GetPhase() const { return enPhase_; }


		private:
			EnGamePhase enPhase_ = EnGamePhase::Playing;	//! 現在の進行状態。
			nsEvent::EventBus* pEventBus_ = nullptr;			//! 購読先のイベントバス。
		};
	}
}
