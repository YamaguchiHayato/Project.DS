#pragma once
#include <vector>
#include "Src/Event/GameEvent.h"

namespace nsApp
{
	namespace nsEvent
	{
		/**
		 * @file   EventBus.h
		 * @brief  ゲーム内イベントの発行/購読を仲介する(Observer/Mediator)。
		 *         発行者と購読者が互いを直接参照せずに済むようにするハブ。
		 * @author Izumida Kiryu
		 * @date   2026/08/21
		 */
		class EventBus : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			EventBus() = default;
			virtual ~EventBus() = default;


		public:
			/* ライフサイクル(このハブ自体は毎フレーム何もしない)。*/
			bool Start() override { return true; }
			void Update() override {}


		public:
			/**
			 * @brief 購読者を登録する(二重登録は無視)。
			 * @param pListener 登録する購読者。
			 */
			void Subscribe(IGameEventListener* pListener);

			/**
			 * @brief 購読者を解除する。
			 * @param pListener 解除する購読者。
			 */
			void Unsubscribe(IGameEventListener* pListener);

			/**
			 * @brief イベントを全購読者へ即時配信する。
			 * @param stEvent 配信するイベント。
			 */
			void Publish(const GameEvent& stEvent);


		private:
			std::vector<IGameEventListener*> vecListeners_;	//! 購読者一覧。
		};
	}
}
