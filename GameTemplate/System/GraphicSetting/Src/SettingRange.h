#pragma once

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @file SettingRange.h
		 * @brief 設定1項目ぶんの名前・範囲・刻み・初期値。
		 * @details 各カテゴリの .cpp に表として並べる。選択肢の項目は 0,1,2… の番号として持つ。
		 * @author Yamaguchi Hayato
		 * @date 2026/11/04: 新規作成日。
		 */
		struct SettingRange
		{
			const char* key_; //! TSV に書く名前。全カテゴリで重複させないこと。
			float min; //! 最小値。
			float max; //! 最大値。
			float step; //! 1回の操作で動く量。
			float defaultValue; //! 初期値。
		};
	}
}