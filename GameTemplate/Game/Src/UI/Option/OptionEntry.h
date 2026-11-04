#pragma once
#include "GraphicsSetting.h"

namespace nsApp
{
	namespace nsUI
	{
		/**
		 * @brief 設定画面のタブ。OPTION_TAB_LABELS と同じ順にすること。
		 */
		enum class OptionTab : unsigned char
		{
			Camera,		//! カメラ。
			Control,	//! 操作。
			Video,		//! グラフィックス。
			Sound,		//! サウンド。
			Count
		};

		/**
		 * @brief 行の値の出し方。
		 */
		enum class OptionRowType : unsigned char
		{
			Gauge,		//! 数字とゲージ。
			Choice,		//! 選択肢の文字。
			Command		//! 決定で別の画面を開く。
		};

		/**
		 * @brief Command の行で開くもの。
		 */
		enum class OptionCommand : unsigned char
		{
			KeyConfig,	//! キー設定。
			None
		};

		/**
		 * @file   OptionEntry.h
		 * @brief  設定画面の1行ぶんの定義。行の追加は OPTION_ENTRIES に1行足すだけにする。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/04: 新規作成日。
		 */
		struct OptionEntry
		{
			OptionTab tab; //! 並ぶタブ。
			OptionRowType type; //! 値の出し方。
			nsData::SettingCategory category; //! 値のカテゴリ。Command では使わない。
			unsigned char item; //! カテゴリ内の項目番号。Command では OptionCommand。
			float displayScale; //! Gauge の数字の倍率。値×これを四捨五入して出す。
			const wchar_t* label; //! 表示名。
			const wchar_t* description; //! 選んでいるときに下に出す説明。
			const wchar_t* const* choices; //! Choice の表示名。番号順。Choice 以外は nullptr。
		};


		extern const OptionEntry OPTION_ENTRIES[]; //! 全行の表。
		extern const int OPTION_ENTRY_COUNT; //! 全行の数。
		extern const wchar_t* const OPTION_TAB_LABELS[]; //! タブの表示名。
	}
}