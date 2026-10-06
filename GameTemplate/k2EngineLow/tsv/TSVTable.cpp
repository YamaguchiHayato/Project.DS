#include "k2EngineLowPreCompile.h"
#include "TSVTable.h"
#include <cstdlib>
#include <fstream>

namespace
{
	const char* whitespace = " \t\r\n"; //! 空白を取り除く文字列。
	const char* UTF8BOM = "\xEF\xBB\xBF"; //! UTF-8 BOM。
	const char* Delimiter = "\t"; //! 区切り文字。
	const char CommentMark_ = '#'; //! この文字で始まる行はコメントとして読み飛ばす。
}

namespace nsK2EngineLow
{
	namespace nsSystem
	{
		namespace nsTSVFile
		{
			bool TSVTable::Load(const char* pFileName)
			{
				/* 読込データを初期化する。*/
				Clear();

				/* ファイルネームが空な場合、エラーメッセージを出す。*/
				if (pFileName == nullptr || pFileName[0] == '\0')
				{
					ReportError("TSVファイルのパスが存在しません。");
					return false;
				}

				/* ファイルが壊れている場合、エラーメッセージを出す。*/
				std::ifstream tsvIfstream(pFileName);
				if (!tsvIfstream.is_open())
				{
					ReportError("TSVファイルの読み込みに失敗しました。");
					return false; 
				}
				if (!std::getline(tsvIfstream, line_))
				{
					ReportError("TSVファイルが空です。");
					return false;
				}

				/* UTF-8 BOMを削除する。*/
				if (line_.compare(0, 3, UTF8BOM) == 0)
					line_.erase(0, 3);

				/* 列から列番号を検索できるように登録する。*/
				SplitLine(line_, cells_);
				for (int i = 0; i < static_cast<int>(cells_.size()); ++i)
				{
					/* 列名が空でない場合、列番号を登録する。*/
					if (!cells_[i].empty())
						mapColumnIndex_[cells_[i]] = i;
				}

				/* TSVファイルの行データを読み込む。*/
				while (std::getline(tsvIfstream, line_))
				{
					/* 空白行やコメント行は読み飛ばす。*/
					const size_t first = line_.find_first_not_of(whitespace);
					if (first == std::string::npos || line_[first] == CommentMark_)
						continue;

					/* TSVファイルの1行分のデータをタブ区切りで分割する。*/
					SplitLine(line_, cells_);
					vectorRows_.push_back(cells_);
				}

				/* TSVファイルの読み込みが完了したことを示すフラグを立てる。*/
				isLoaded_ = true;

				/* TSVファイルの読み込みに成功したことを示す。*/
				return true;
			}


			void TSVTable::ReportError(const std::string& message)
			{
				/* エラー内容を記録する。*/
				lastError_ = message;

#ifdef K2_DEBUG
				/* エラー内容をウィンドウに表示する。*/
				NotificationErrorMessage();
#endif // K2_DEBUG

			}


			std::string TSVTable::GetString(int row, const char* pColumnName, const char* defaultValue) const
			{
				/* 指定した行列のセルを探す。*/
				const std::string* pCell = FindCell(row, pColumnName);

				return (pCell != nullptr) ? *pCell : defaultValue;
			}


			float TSVTable::GetFloat(int row, const char* pColumnName, float defaultValue) const
			{
				/* セルが無ければデフォルト値を返す。*/
				const std::string* pCell = FindCell(row, pColumnName);
				if (pCell == nullptr)
					return defaultValue;

				/* 最後まで数として読めなければデフォルト値(打ち間違いを0にしない)。*/
				char* pEnd = nullptr;
				const float value = std::strtof(pCell->c_str(), &pEnd);
				if (*pEnd != '\0')
					return defaultValue;
				return value;
			}


			int TSVTable::GetInt(int row, const char* pColumnName, int defaultValue) const
			{
				/* セルが無ければデフォルト値を返す。*/
				const std::string* pCell = FindCell(row, pColumnName);

				if (pCell == nullptr)
					return defaultValue;

				/* 8 と書いても 8.0 と書いても整数として受け取れるようにする。*/
				char* pEnd = nullptr;
				const double value = std::strtod(pCell->c_str(), &pEnd);

				/* 最後まで数として読めなければデフォルト値(打ち間違いを0にしない)。*/
				if (*pEnd != '\0')
					return defaultValue;
				return static_cast<int>(value);
			}


			bool TSVTable::GetBool(int row, const char* pColumnName, bool defaultValue) const
			{
				/* セルが無ければデフォルト値を返す。*/
				const std::string* pCell = FindCell(row, pColumnName);

				/* セルが無ければデフォルト値を返す。*/
				if (pCell == nullptr)
					return defaultValue;

				/* Excelから書き出すと TRUE/FALSE になるので大文字も受け取る。*/
				if (*pCell == "true" || *pCell == "TRUE" || *pCell == "1")
					return true;
				if (*pCell == "false" || *pCell == "FALSE" || *pCell == "0")
					return false;

				return defaultValue;
			}


			void TSVTable::Clear()
			{
				/* 読み込んだデータとエラー内容を空にする。*/
				vectorRows_.clear();
				mapColumnIndex_.clear();
				lastError_.clear();
				line_.clear();
				cells_.clear();
				isLoaded_ = false;
			}


			const std::string* TSVTable::FindCell(int row, const char* pColumnName) const
			{
				/* 行番号が範囲外なら無し。*/
				if (row < 0 || row >= GetRowCount() || pColumnName == nullptr)
					return nullptr;

				/* 列名が存在しなければ無し。*/
				const auto it = mapColumnIndex_.find(pColumnName);
				if (it == mapColumnIndex_.end())
					return nullptr;

				/* その行に列が足りない、または空セルなら無し(デフォルト値を使わせる)。*/
				const TSVRowData& rowData = vectorRows_[row];
				if (it->second >= static_cast<int>(rowData.size()) || rowData[it->second].empty())
					return nullptr;

				/* セルが見つかったので返す。*/
				return &rowData[it->second];
			}


			void TSVTable::SplitLine(const std::string& line, TSVRowData& outputCell)
			{
				outputCell.clear();
				/* タブ区切りで分割する。タブが連続しても空のセルとして1列に数える。*/
				size_t begin = 0;
				while (true)
				{
					/* 区切り文字の位置を探す。*/
					const size_t end = line.find(Delimiter, begin);
					outputCell.push_back(Trim(line.substr(begin, end - begin)));

					/* 区切り文字が見つからなければ終了する。*/
					if (end == std::string::npos)
						break;

					/* 次のセルの先頭位置を更新する。*/
					begin = end + 1;
				}
			}


			std::string TSVTable::Trim(const std::string& text)
			{
				/* 空白だけで構成されている場合は空文字を返す。*/
				const size_t begin = text.find_first_not_of(whitespace);
				if (begin == std::string::npos)
					return "";

				/* 前後の空白を取り除いた文字列を返す。*/
				const size_t end = text.find_last_not_of(whitespace);
				return text.substr(begin, end - begin + 1);
			}


			void TSVTable::NotificationErrorMessage()
			{
				/* エラー内容をウィンドウに表示する。*/
				const int length = MultiByteToWideChar(CP_UTF8, 0, lastError_.c_str(), -1, nullptr, 0);
				std::wstring wideMessage(length, L'\0');
				MultiByteToWideChar(CP_UTF8, 0, lastError_.c_str(), -1, &wideMessage[0], length);
				MessageBoxW(nullptr, wideMessage.c_str(), L"TSV読み込みエラー", MB_OK | MB_ICONERROR);
			}
		}
	}
}