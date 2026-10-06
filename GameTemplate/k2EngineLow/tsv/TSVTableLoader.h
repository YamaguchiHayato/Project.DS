#pragma once
#include "TSVTable.h"

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
			 * @file   TSVTableLoader.h
			 * @brief  TSVファイルから、Create を持つ型の一覧・表を作るヘルパークラス。
			 * @details 読み込む型には次の静的関数を持たせる(Createパターン)。
			 *          static bool Create(const TSVTable& table, int row, 型& outParameter);
			 *          1行から自分自身を作り、使えない行ならfalseを返す。
			 * @author Yamaguchi Hayato
			 * @date   2026/10/06: 新規作成日。
			 */
			class TSVTableLoader
			{
			public:
				/**
				 * @brief TSVファイルの全行から、パラメータの一覧を作る。
				 *        同じ種類が何行も並ぶ表(配置表など)に使う。
				 * @tparam ParameterType Create を持つ型。
				 * @param pFileName TSVファイルのパス。
				 * @param outList 読み込み結果を格納する一覧。失敗した場合は元のまま。
				 * @return 1件以上読み込めたらtrue。
				 */
				template<class ParameterType>
				static bool LoadList(const char* pFileName, std::vector<ParameterType>& outList);

				/**
				 * @brief TSVファイルの全行から、キーごとのパラメータ表を作る。
				 *        キーが重複しない表(敵種ごとのステータスなど)に使う。同じキーは後の行で上書きする。
				 * @tparam KeyType キーの型。
				 * @tparam ParameterType Create を持つ型。
				 * @tparam KeyConverter キー名をキー型へ変換する関数。bool(const std::string&, KeyType&) の形。
				 * @param pFileName TSVファイルのパス。
				 * @param pKeyColumnName キーとして使用する列名。
				 * @param outTable 読み込み結果を格納する表。失敗した場合は元のまま。
				 * @param keyConverter キー名をキー型へ変換する関数。知らない名前ならfalseを返す。
				 * @param pSecondaryKeyColumnName キー列が空のときに見る予備の列名。不要ならnullptr。
				 * @return 1件以上読み込めたらtrue。
				 */
				template<class KeyType, class ParameterType, class KeyConverter>
				static bool LoadTable(const char* pFileName, const char* pKeyColumnName, std::unordered_map<KeyType, ParameterType>& outTable, KeyConverter keyConverter, const char* pSecondaryKeyColumnName = nullptr
				);
			};
		}
	}
}

#include "TSVTableLoader.inl"