#pragma once
#include "stdint.h"

namespace nsApp
{
	namespace nsEvent
	{
		/**
		 * @file   GameEvent.h
		 * @brief  ゲーム内イベントの定義と、購読者インターフェース(Observer)。
		 *         発行者(武器・敵・トリガ等)と購読者(ルール・HUD等)を疎結合にするための土台。
		 * @author Izumida Kiryu
		 * @date   2026/08/21
		 */

		/**
		 * @enum  EnGameEvent
		 * @brief ゲーム進行に関わる出来事の種類。
		 */
		enum class EnGameEvent : uint8_t
		{
			PlayerReachedSafeRoom,	//! プレイヤーがセーフルームへ到達した。
			PlayerDead,				//! プレイヤーが死亡した。
			PlayerDowned,			//! プレイヤーがダウンした(予約)。
			PlayerRevived,			//! プレイヤーが救助された(予約)。
			EnemyKilled,			//! 敵を撃破した(予約)。
		};

		/**
		 * @struct GameEvent
		 * @brief  発行される1件のイベント。必要に応じてパラメータを増やす。
		 */
		struct GameEvent
		{
			EnGameEvent	enType_;		//! イベント種別。
			int			iParam_ = 0;	//! 付随パラメータ(用途は種別依存。予約)。
		};

		/**
		 * @class IGameEventListener
		 * @brief イベント購読者のインターフェース(Observer)。
		 */
		class IGameEventListener
		{
		public:
			/* デストラクタ。*/
			virtual ~IGameEventListener() = default;

			/**
			 * @brief イベントを受け取る。
			 * @param stEvent 受け取ったイベント。
			 */
			virtual void OnGameEvent(const GameEvent& stEvent) = 0;
		};
	}
}
