#pragma once

namespace nsK2EngineLow
{
	namespace nsSystem
	{
		namespace nsTSVFile
		{
			template<class ParameterType>
			bool TSVTableLoader::LoadList(const char* pFileName, std::vector<ParameterType>& outList)
			{
				/* TSVファイルを読み込むための一時テーブルを作成する。*/
				TSVTable table;
				if (!table.Load(pFileName))
					return false;

				/* 読み込みに失敗しても既存の一覧を壊さないように、一時の一覧へ作る。*/
				std::vector<ParameterType> loadedList;
				loadedList.reserve(table.GetRowCount());

				/* 1行ずつ型自身の Create に作らせ、作れた行だけ一覧に入れる。*/
				for (int row = 0; row < table.GetRowCount(); ++row)
				{
					ParameterType parameter;
					if (ParameterType::Create(table, row, parameter))
						loadedList.push_back(parameter);
				}

				/* 有効なデータが1件も読み込めなかった場合は失敗にする。*/
				if (loadedList.empty())
					return false;

				/* 読み込みに成功した場合だけ、正式な一覧へ反映する。*/
				outList.swap(loadedList);

				return true;
			}


			template<class KeyType, class ParameterType, class KeyConverter>
			bool TSVTableLoader::LoadTable(const char* pFileName, const char* pKeyColumnName, std::unordered_map<KeyType, ParameterType>& outTable, KeyConverter keyConverter, const char* pSecondaryKeyColumnName)
			{
				/* TSVファイルを読み込むための一時テーブルを作成する。*/
				TSVTable table;
				if (!table.Load(pFileName))
					return false;

				/* 読み込みに失敗しても既存の表を壊さないように、一時の表へ作る。*/
				std::unordered_map<KeyType, ParameterType> loadedTable;

				for (int row = 0; row < table.GetRowCount(); ++row)
				{
					/* まずはメインのキー列、空なら予備のキー列からキー名を取る。*/
					std::string keyName = table.GetString(row, pKeyColumnName);
					if (keyName.empty() && pSecondaryKeyColumnName != nullptr)
						keyName = table.GetString(row, pSecondaryKeyColumnName);

					/* キー名が取れない、または知らないキー名の行は無効行として無視する。*/
					KeyType key{};
					if (keyName.empty() || !keyConverter(keyName, key))
						continue;

					/* 1行分のパラメータは型自身の Create に作らせる。*/
					ParameterType parameter;
					if (!ParameterType::Create(table, row, parameter))
						continue;

					loadedTable[key] = parameter;
				}

				/* 有効なデータが1件も読み込めなかった場合は失敗にする。*/
				if (loadedTable.empty())
					return false;

				/* 読み込みに成功した場合だけ、正式な表へ反映する。*/
				outTable.swap(loadedTable);

				return true;
			}
		}
	}
}