#pragma once

#include "OptionCanvas.h"
#include "OptionEntry.h"
#include "OptionFooter.h"
#include "OptionModel.h"
#include "OptionRowList.h"
#include "OptionTabBar.h"

namespace nsApp
{
	namespace nsUI
	{
		class KeyConfigUI;

		/**
		 * @file   OptionUI.h
		 * @brief  設定画面の司令塔。入力を受け、部品に描画を任せる。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/04: 最終更新日。ユミア風のタブと行に組み替えた。
		 */
		class OptionUI : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。 */
			OptionUI() = default;
			virtual ~OptionUI();


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;


		public:
			/**
			 * @brief 設定画面を閉じるかどうかを返す。
			 * @return 閉じるなら true。
			 */
			bool IsClose() const
			{
				return close_;
			}


		private:
			static constexpr int MAX_TAB_ROWS = 32; //! 1つのタブに並べられる行の上限。


		private:
			/**
			 * @brief 今のタブの行を表から集める。
			 */
			void FillRows();

			/**
			 * @brief カーソルとスクロール位置を、行の数に収める。
			 */
			void ClampCursor();

			/**
			 * @brief タブを切り替える。カーソルは先頭へ戻す。
			 * @param tab 切り替え先。
			 */
			void ChangeTab(OptionTab tab);

			/**
			 * @brief 今の行の値を1刻み動かす。
			 * @param direction 右なら 1、左なら -1。
			 */
			void StepCurrent(int direction);

			/**
			 * @brief キー設定画面を開く。
			 */
			void OpenKeyConfig();

			/**
			 * @brief キー設定が開いているあいだの後始末。
			 * @return キー設定が開いていれば true。その場合は入力しない。
			 */
			bool UpdateKeyConfig();

			/**
			 * @brief キーボードの入力を処理する。
			 */
			void UpdateKeyboard();

			/**
			 * @brief マウスの入力を処理する。
			 */
			void UpdateMouse();


		private:
			OptionCanvas canvas_; //! 枠と文字。
			OptionTabBar tabBar_; //! 上のタブ。
			OptionRowList rowList_; //! 項目の行。
			OptionFooter footer_; //! 説明と操作ガイド。
			OptionModel model_; //! 値の読み書き。
			const OptionEntry* rows_[MAX_TAB_ROWS] = {}; //! 今のタブの行。
			KeyConfigUI* pKeyConfig_ = nullptr; //! キー設定。開いていなければ nullptr。
			OptionTab tab_ = OptionTab::Camera; //! 今のタブ。
			int rowCount_ = 0; //! 今のタブの行数。
			int cursor_ = 0; //! 選んでいる行。
			int top_ = 0; //! 一番上に見せている行。
			bool close_ = false; //! 設定画面を閉じるか。
			bool wasW_ = false; //! 前フレームで W が押されていたか。
			bool wasS_ = false; //! 前フレームで S が押されていたか。
			bool wasA_ = false; //! 前フレームで A が押されていたか。
			bool wasD_ = false; //! 前フレームで D が押されていたか。
			bool wasQ_ = false; //! 前フレームで Q が押されていたか。
			bool wasE_ = false; //! 前フレームで E が押されていたか。
			bool wasR_ = false; //! 前フレームで R が押されていたか。
			bool wasEnter_ = false; //! 前フレームで Enter が押されていたか。
			bool wasEscape_ = false; //! 前フレームで Escape が押されていたか。
		};
	}
}