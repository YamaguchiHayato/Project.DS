#pragma once

#include "Src/UI/Option/OptionCanvas.h"

namespace nsApp
{
	namespace nsUI
	{
		/**
		 * @file   KeyConfigUI.h
		 * @brief  キー割り当ての表。操作名と今のキーを並べ、選んで付け替える。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/04: 最終更新日。マイクラ式の表にした。
		 */
		class KeyConfigUI : public IGameObject
		{
		public:
			/* ライフサイクル。 */
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;


		public:
			/**
			 * @brief キー設定を閉じるかどうかを返す。
			 * @return 閉じるなら true。
			 */
			bool IsClose() const
			{
				return close_;
			}


		private:
			/**
			 * @brief 見出しを飛ばして、選べる行へ動かす。
			 * @param direction 上なら -1、下なら 1。
			 */
			void MoveCursor(int direction);

			/**
			 * @brief 選んでいる行が画面の外に出ないように、一番上の行を動かす。
			 */
			void ClampTop();

			/**
			 * @brief キーボードの入力を処理する。
			 */
			void UpdateKeyboard();

			/**
			 * @brief マウスの入力を処理する。
			 */
			void UpdateMouse();

			/**
			 * @brief 押されたキーを、選んでいる行へ入れる。
			 */
			void TryRebind();


		private:
			OptionCanvas canvas_; //! 枠と文字。設定画面と同じ描き方。
			int selected_ = 0; //! 選んでいる行。表の番号。
			int top_ = 0; //! 一番上に見せている行。
			bool waiting_ = false; //! 次の入力を待っているか。
			bool close_ = false; //! 表を閉じるか。
			bool wasW_ = false; //! 前フレームで W が押されていたか。
			bool wasS_ = false; //! 前フレームで S が押されていたか。
			bool wasEnter_ = false; //! 前フレームで Enter が押されていたか。
			bool wasR_ = false; //! 前フレームで R が押されていたか。
			bool wasEscape_ = false; //! 前フレームで Escape が押されていたか。
			bool wasMouseLeft_ = false; //! 前フレームで左クリックされていたか。
		};
	}
}