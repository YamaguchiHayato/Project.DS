#pragma once
#include "SettingRange.h"
#include <algorithm>
#include <ostream>
#include <string>

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @file SettingGroup.h
		 * @brief 設定カテゴリの共通部分。値は配列で持ち、範囲は表を見る。
		 * @details 各カテゴリは項目の列挙と表を渡すだけで、読み書きと範囲チェックはここで済む。
		 * @tparam ITEM 項目の列挙。最後に Count を置くこと。
		 * @author Yamaguchi Hayato
		 * @date 2026/11/04: 新規作成日。
		 */
		template<typename ITEM>
		class SettingGroup
		{
		public:
			static constexpr int COUNT = static_cast<int>(ITEM::Count); //! 項目の数。


		public:
			/**
			 * @brief 全項目を初期値に戻す。
			 */
			void FillDefault()
			{
				for (int index = 0; index < COUNT; index++)
					values_[index] = table_[index].defaultValue;
			}

			/**
			 * @brief 値を返す。
			 * @param item 項目。
			 * @return 値。
			 */
			inline float Get(ITEM item) const
			{
				return values_[ToIndex(item)];
			}

			/**
			 * @brief 選択肢の番号を返す。通常/反転、ON/OFF などに使う。
			 * @param item 項目。
			 * @return 番号。
			 */
			inline int GetIndex(ITEM item) const
			{
				return static_cast<int>(values_[ToIndex(item)] + 0.5f);
			}

			/**
			 * @brief ON/OFF の項目が ON かを返す。
			 * @param item 項目。
			 * @return ON なら true。
			 */
			inline bool IsON(ITEM item) const
			{
				return GetIndex(item) != 0;
			}

			/**
			 * @brief 値を範囲内に収めて入れる。
			 * @param item 項目。
			 * @param value 入れたい値。
			 */
			inline void Set(ITEM item, float value)
			{
				const int index = ToIndex(item);
				values_[index] = std::clamp(value, table_[index].min, table_[index].max);
			}

			/**
			 * @brief 項目の範囲と初期値を返す。画面側でゲージや刻みに使う。
			 * @param item 項目。
			 * @return 範囲。
			 */
			inline const SettingRange& GetRange(ITEM item) const
			{
				return table_[ToIndex(item)];
			}

			/**
			 * @brief TSV の1行を、名前が一致すれば取り込む。
			 * @param key 項目名。
			 * @param value 値。
			 * @return このカテゴリの項目なら true。
			 */
			bool Read(const std::string& key, float value)
			{
				/* 項目が一致しているかを探索。*/
				for (int index = 0; index < COUNT; index++)
				{
					/* 項目名が一致しなければ次へ。*/
					if (key != table_[index].key_)
						continue;

					/* 範囲内に収めて入れる。*/
					values_[index] = std::clamp(value, table_[index].min, table_[index].max);
					return true;
				}
				return false;
			}

			/**
			 * @brief 全項目を TSV の行として書き出す。
			 * @param stream 書き出し先。
			 */
			inline void Write(std::ostream& stream) const
			{
				for (int index = 0; index < COUNT; index++)
					stream << table_[index].key_ << '\t' << values_[index] << '\n';
			}


		protected:
			/**
			 * @brief 表を受け取り、初期値で埋める。
			 * @param table 項目の表。列挙と同じ順・同じ数であること。
			 */
			explicit SettingGroup(const SettingRange* table) : table_(table)
			{
				FillDefault();
			}


		private:
			/**
			 * @brief 列挙を配列の番号に変える。
			 */
			static constexpr int ToIndex(ITEM item)
			{
				return static_cast<int>(item);
			}


		private:
			const SettingRange* table_ = nullptr; //! 範囲と初期値の表。
			float values_[COUNT] = {}; //! 今の値。
		};
	}
}