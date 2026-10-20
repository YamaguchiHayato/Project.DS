#pragma once
#include "stdint.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @enum EnPlayerFact
		 * @brief プレイヤーの状態を表すフラグ。
		 * @details 条件が追加されるたびに編集を要する。
		 * @author Yamaguchi Hayato
		 * @date 2026/10/16
		 */
		enum EnPlayerFact : uint32_t
		{
			enPlayerFact_None = 0,					//! 条件なし。
			enPlayerFact_Moving = 1u << 0,			//! 移動中。
			enPlayerFact_NotMoving = 1u << 1,		//! 停止中。
			enPlayerFact_Sprinting = 1u << 2,		//! スプリント中。
			enPlayerFact_NotSprinting = 1u << 3,	//! スプリントしていない。
			enPlayerFact_WantWinPause = 1u << 4,	//! リザルト用ポーズへ移りたい（後で使う）。
			enPlayerFact_WantAttack = 1u << 5,		//! 銃戦闘ステートへ移りたい（後で使う）。
			enPlayerFact_NotWantAttack = 1u << 6,	//! 銃戦闘ステートを抜けたい（後で使う）。
		};


		class PlayerSituation
		{
		public:  
			/* コンストラクタとデストラクタ。*/
			PlayerSituation() = default;
			virtual ~PlayerSituation() = default;


		public:
			/**
			 * @brief プレイヤーの状態をクリアする。
			 */
			void Clear();

			/**
			 * @brief プレイヤーの状態をフラグとしてまとめる。
			 * @return プレイヤーの状態を表すフラグのビットマスク。
			 */
			uint32_t MakeFactMask() const;

			/**
			 * @brief 要求された事実をすべて満たしているか判定する。
			 * @param need 樹圭樹が要求する事実マスク。
			 * @return すべて満たしていれば true。
			 */
			bool Matches(uint32_t need) const;


		public:
			/**
			 * @brief 移動中か。
			 * @param moving 移動中なら true。
			 */
			inline void SetMoving(bool moving)
			{
				moving_ = moving;
			}

			/**
			 * @brief スプリント中フラグを設定する。
			 * @param bSprinting スプリント中なら true。
			 */
			inline void SetSprinting(bool bSprinting)
			{
				sprinting_ = bSprinting;
			}

			/**
			 * @brief リザルト用ポーズ要求を設定する。
			 * @param bWantWinPause 要求があれば true。
			 */
			inline void SetWantWinPause(bool bWantWinPause)
			{
				wantWinPause_ = bWantWinPause;
			}

			/**
			 * @brief 銃戦闘ステート要求を設定する。
			 * @param bWantAttack 要求があれば true。
			 */
			inline void SetWantAttack(bool bWantAttack)
			{
				wantAttack_ = bWantAttack;
			}


		public:
			/**
			 * @brief 移動中か。
			 * @return 移動中なら true。
			 */
			inline bool IsMoving() const
			{
				return moving_;
			}

			/**
			 * @brief スプリント中か。
			 * @return スプリント中なら true。
			 */
			inline bool IsSprinting() const
			{
				return sprinting_;
			}

			/**
			 * @brief リザルト用ポーズを要求しているか。
			 * @return 要求していれば true。
			 */
			inline bool IsWantWinPause() const
			{
				return wantWinPause_;
			}

			/**
			 * @brief 銃戦闘ステートを要求しているか。
			 * @return 要求していれば true。
			 */
			inline bool IsWantAttack() const
			{
				return wantAttack_;
			}


		private:
			bool moving_ = false; //! 移動中か判定。
			bool sprinting_ = false; //! スプリント中か判定。 ※。
			bool wantWinPause_ = false; //! リザルト用ポーズへ移りたいか判定。
			bool wantAttack_ = false; //! 銃戦闘ステートへ移りたいか判定。
		};
	}
}