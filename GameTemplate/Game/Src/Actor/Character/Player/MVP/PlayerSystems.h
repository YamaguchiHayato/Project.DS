#pragma once
#include "PlayerView.h"
#include "PlayerCalc.h"
#include "PlayerUpdate.h"
#include "PlayerCombat.h"
#include "PlayerItem.h"

namespace nsApp
{
	namespace nsActor
	{
		class Player;
		struct PlayerIntent;

		/**
		 * @file   PlayerSystems.h
		 * @brief  操作キャラの部位（見た目／計算／更新／戦闘／アイテム）を統べる注入口。
		 * @details Player は本クラスを1つだけ持ち、Init / Tick / Present を委譲する。
		 *          各部位の呼び順もここに閉じる。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16
		 */
		class PlayerSystems
		{
		public:
			/*コンストラクタとデストラクタ。 */
			PlayerSystems() = default;
			virtual ~PlayerSystems() = default;


		public:
			/**
			 * @brief 各部位を初期化する。
			 * @param pPlayer 操作キャラ（非所有）。
			 * @param status プレイヤーステータス（スケール・手ボーン名）。
			 * @param position 初期座標。
			 */
			void Init(Player* pPlayer, const nsData::PlayerStatus& status, const Vector3& position);

			/**
			 * @brief 1フレームぶんの常時・戦闘・アイテム更新を進める。
			 * @param deltaTime 経過時間(秒)。
			 * @param intent このフレームの操作意図。
			 */
			void Tick(float deltaTime, const PlayerIntent& intent);

			/**
			 * @brief 見た目を反映する。
			 */
			void Present();

			/**
			 * @brief 描画する。
			 * @param rc レンダリングコンテキスト。
			 */
			void Render(RenderContext& rc);

			/**
			 * @brief 見た目部位への参照。
			 * @details PlayAnimation など、State から Player 経由で触るときに使う。
			 */
			inline PlayerView& GetView()
			{
				return view_;
			}

			/**
			 * @brief 見た目部位への参照。
			 * @details PlayAnimation など、State から Player 経由で触るときに使う。
			 */
			inline const PlayerView& GetView() const
			{
				return view_; 
			}

			/**
			 * @brief 戦闘部位への参照。
			 * @details 後で AttackState から使う。
			 */
			inline PlayerCombat& GetCombat() 
			{
				return combat_; 
			}

			/**
			 * @brief アイテム部位への参照。
			 * @details 後で AttackState から使う。
			 */
			inline PlayerItem& GetItem() 
			{
				return item_; 
			}

			/**
			 * @brief 計算部位への参照。
			 * @return 計算結果。
			 */
			inline PlayerCalc& GetCalc()
			{
				return calc_; 
			}

			/**
			 * @brief 常時更新部位への参照。
			 * @return 更新結果。
			 */
			inline PlayerUpdate& GetUpdate() 
			{
				return update_; 
			}


		private:
			/**
			 * @brief 各部位の更新を進める。
			 */
			void Updaters(float deltaTime);

			/**
			 * @brief 各部位の常時更新を進める。
			 * @param deltaTime 経過時間(秒)。
			 * @param intent このフレームの操作意図。
			 */
			void AlwaysUpdates(float deltaTime, const PlayerIntent& intent);

			/**
			 * @brief 各部位の初期化を進める。
			 */
			void Initializers(Player* pPlayer, const nsData::PlayerStatus& status, const Vector3& position);

			/**
			 * @brief 各部位の見た目を反映する。
			 */
			void Presenters();


		private:
			PlayerView view_;		//! 見た目。
			PlayerCalc calc_;		//! 計算。
			PlayerUpdate update_;	//! 常時更新。
			PlayerCombat combat_;	//! Attack 系実処理。
			PlayerItem item_;		//! アイテム。
		};
	}
}