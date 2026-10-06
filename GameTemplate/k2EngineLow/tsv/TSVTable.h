#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace nsK2EngineLow
{
	namespace nsSystem
	{
		namespace nsTSVFile
		{
			/**
			 * @file   TSVTable.h
			 * @brief  TSVファイルを読み込む汎用テーブルクラス。
			 * @author Yamaguchi Hayato
			 * @date   2026/10/06: 新規作成日。
			 */
			class TSVTable : public Noncopyable
			{
			public:
				/**
				 * @brief TSVファイルの1行分のデータ。
				 * @param pFileName TSVファイルのパス。
				 * @return 読込に成功したらtrue、失敗したらfalse。
				 */
				bool Load(const char* pFileName);

				/**
				 * @brief TSVファイルの読み込みが完了しているかを返す。
				 * @return 読み込み済みならtrue、未読み込みならfalse。
				 */
				inline bool IsLoaded() const
				{
					return isLoaded_;
				}

				/**
				 * @brief TSVファイルの行数を取得する。
				 * @return TSVファイルの行数。
				 */
				inline int GetRowCount() const
				{
					return static_cast<int>(vectorRows_.size());
				}

				/**
				 * @brief TSVファイルの列数があるか確認する。
				 * @return TSVファイルの列数。
				 */
				inline bool HasColumn(const char* pColumnName) const
				{
					return mapColumnIndex_.find(pColumnName) != mapColumnIndex_.end();
				}

				/**
				 * @brief TSVファイル内の std::string 型の値を取得する。
				 * @param row 行。
				 * @param pColumnName 列。
				 * @param defaultValue デフォルト値。
				 * @return 取得した std::string 値。
				 */
				std::string GetString(int row, const char* pColumnName, const char* defaultValue = "") const;

				/**
				 * @brief TSVファイル内の float 型の値を取得する。
				 * @param row 行。
				 * @param pColumnName 列。
				 * @param defaultValue デフォルト値。
				 * @return 取得した float 値。
				 */
				float GetFloat(int row, const char* pColumnName, float defaultValue = 0.0f) const;

				/**
				 * @brief TSVファイル内の int 型の値を取得する。
				 * @param row 行。
				 * @param pColumnName 列。
				 * @param defaultValue デフォルト値。
				 * @return 取得した int 値。
				 */
				int GetInt(int row, const char* pColumnName, int defaultValue = 0) const;

				/**
				 * @brief TSVファイル内の bool 型の値を取得する。
				 * @param row 行。
				 * @param pColumnName 列。
				 * @param defaultValue デフォルト値。
				 * @return 取得した bool 値。
				 */
				bool GetBool(int row, const char* pColumnName, bool defaultValue = false) const;

				/**
				 * @brief TSVファイル内の std::string 型の値を取得する。
				 * @return 取得した std::string 値。
				 */
				inline const std::string GetErrorMessage() const
				{
					return lastError_;
				}


			private:
				typedef std::vector<std::string> TSVRowData; //! TSVファイルの1行分のデータ。

				/**
				 * @brief TSVファイルの行データを解放する。
				 */
				void Clear();

				/**
				 * @brief 指定した行列のセルを探す。
				 * @param row 行。
				 * @param pColumnName 列。 
				 * @return 見つかったセル。 無いなら nullptr を返す。
				 */
				const std::string* FindCell(int row, const char* pColumnName) const;

				/**
				 * @brief 1行をタブ区切りで分割する。空のセルも1列として残す。
				 * @param sLine 対象の文字列。
				 * @param outCells 分割結果。
				 */
				static void SplitLine(const std::string& line, TSVRowData& outputCell);
				
				/**
				 * @brief 文字列の前後の空白を取り除く。
				 * @param text 対象の文字列。
				 * @param text 対象の文字列。
				 * @return 空白を取り除いた文字列。
				 */
				static std::string Trim(const std::string& text);

				/**
				 * @brief エラー内容を記録し、ウィンドウを出して知らせる。
			      * @param message エラー内容。
				 */
				void ReportError(const std::string& message);

				/**
				 * @brief エラーメッセージの処理をまとめておく関数。
				 */
				void NotificationErrorMessage();


			private:	
				std::vector<TSVRowData> vectorRows_; //! TSVファイルの行データを管理する。
				std::unordered_map<std::string, int> mapColumnIndex_; //! 列名から列番号を取得。
				std::string lastError_; //! エラー内容を表示する用の文字列。
				bool isLoaded_ = false; //! TSVファイルの読み込みが完了しているかを管理するフラグ。;
				std::string line_; //! TSVファイルの1行分のデータ。
				TSVRowData cells_; //! TSVファイルの1行分のデータをタブ区切りで分割した結果を格納する。
			};
		}
	}
}

