#include "stdafx.h"
#include "EventBus.h"
#include <algorithm>

namespace nsApp
{
	namespace nsEvent
	{
		void EventBus::Subscribe(IGameEventListener* pListener)
		{
			/* 無効・重複は登録しない。*/
			if (pListener == nullptr)
				return;
			if (std::find(vecListeners_.begin(), vecListeners_.end(), pListener) != vecListeners_.end())
				return;

			/* 購読者を追加する。*/
			vecListeners_.push_back(pListener);
		}


		void EventBus::Unsubscribe(IGameEventListener* pListener)
		{
			/* 一致する購読者を取り除く。*/
			vecListeners_.erase(
				std::remove(vecListeners_.begin(), vecListeners_.end(), pListener),
				vecListeners_.end());
		}


		void EventBus::Publish(const GameEvent& stEvent)
		{
			/* 現在の購読者全員へ配信する。*/
			for (IGameEventListener* pListener : vecListeners_)
			{
				if (pListener != nullptr)
					pListener->OnGameEvent(stEvent);
			}
		}
	}
}
