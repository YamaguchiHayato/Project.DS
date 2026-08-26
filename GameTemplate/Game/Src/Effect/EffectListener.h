#pragma once
#include <unordered_map>
#include "Src/Event/GameEvent.h"
#include "Src/Effect/EffectList.h"

namespace nsApp
{
	namespace nsEffect
	{
		/**
		 * @file   EffectListener.h
		 * @brief  ゲーム内の通知を購読し、その出来事に応じたエフェクトを再生するリスナー(Observer)。
		 *         「何が起きたか」だけを受け取り、何をどう見せるかはこのクラスが決める。
		 *         発行元(Player/Grenadeなど)は演出を知らなくてよい。
		 *         SEを実装するときは、同じ通知を購読する SoundListener を並べて追加する。
		 * @author Izumida Kiryu
		 * @date   2026/08/21
		 */

		/**
		 * @struct EffectPlaySetting
		 * @brief  通知1件に対して、どのエフェクトをどう再生するかの設定。
		 */
		struct EffectPlaySetting
		{
			EnEffectID enID_ = EnEffectID::Hit;	//! 再生するエフェクトの識別子。
			float fScale_ = 1.0f;				//! 表示倍率。
			float fLifeTime_ = 1.0f;			//! 表示時間(秒)。
			bool bUseDirection_ = false;		//! 通知の向きにエフェクトを向けるか。
		};

		class EffectListener : public nsEvent::IGameEventListener
		{
		public:
			/* コンストラクタとデストラクタ。*/
			EffectListener() = default;
			virtual ~EffectListener() = default;


		public:
			/**
			 * @brief 再生に使うエフェクトリストを設定する。
			 * @param pEffectList 再生先のエフェクトリスト。
			 */
			void Initialize(EffectList* pEffectList)
			{
				pEffectList_ = pEffectList;
			}

			/**
			 * @brief 通知を受け取り、対応するエフェクトを再生する。
			 * @param stEvent 受け取った通知。
			 */
			void OnGameEvent(const nsEvent::GameEvent& stEvent) override;


		private:
			EffectList* pEffectList_ = nullptr;	//! 再生先のエフェクトリスト。
		};
	}
}
