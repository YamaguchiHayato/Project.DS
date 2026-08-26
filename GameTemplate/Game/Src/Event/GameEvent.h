#pragma once
#include "stdint.h"

namespace nsApp
{
	namespace nsEvent
	{
		/**
		 * @file   GameEvent.h
		 * @brief  ゲーム内で「何が起きたか」を伝える通知の型・種別の定義。
		 *         ここで定義するのは出来事そのものだけで、演出(エフェクト/SE/BGM)の指示は含めない。
		 *         演出は購読側(リスナー)が通知を見て自分で再生する。
		 * @author Izumida Kiryu
		 * @date   2026/08/21
		 */

		/**
		 * @enum  EnGameEvent
		 * @brief ゲーム内で起きた出来事の種類。
		 */
		enum class EnGameEvent : uint8_t
		{
			/* 進行・ルールに関わる出来事。*/
			PlayerReachedSafeRoom,	//! プレイヤーがセーフルームへ到達した。
			PlayerDead,				//! プレイヤーが死亡した。
			PlayerDowned,			//! プレイヤーがダウンした。
			PlayerRevived,			//! プレイヤーが救助された。
			EnemyKilled,			//! 敵を撃破した。

			/* 戦闘中の出来事。*/
			WeaponFired,			//! 武器を発射した。
			BulletHit,				//! 弾が命中した。
			GrenadeExploded,		//! グレネードが爆発した。
			PlayerHealed,			//! プレイヤーが回復した。
		};

		/**
		 * @struct GameEvent
		 * @brief  発行される1件の通知。出来事の種別と、それに付随するデータを持つ。
		 *         付随データは種別によって使う項目が異なる(使わない項目は初期値のまま)。
		 */
		struct GameEvent
		{
			EnGameEvent enType_;						//! 出来事の種別。
			Vector3 vPosition_ = Vector3::Zero;			//! 出来事が起きた位置。
			Vector3 vDirection_ = Vector3::Zero;		//! 出来事の向き(発射方向・命中方向など)。
			int iParam_ = 0;							//! 付随する数値(用途は種別依存)。
		};

		/**
		 * @class IGameEventListener
		 * @brief 通知を購読する側のインターフェース(Observer)。
		 */
		class IGameEventListener
		{
		public:
			/* デストラクタ。*/
			virtual ~IGameEventListener() = default;

			/**
			 * @brief 通知を受け取る。
			 * @param stEvent 受け取った通知。
			 */
			virtual void OnGameEvent(const GameEvent& stEvent) = 0;
		};
	}
}
