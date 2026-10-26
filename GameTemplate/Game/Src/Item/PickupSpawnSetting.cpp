#include "stdafx.h"
#include "Src/Item/PickupSpawnSetting.h"

namespace
{
	const char* sCandidatesColumn_ = "candidates";	//! 置く候補の列名。
	const char* sPosXColumn_ = "x";					//! 中心のX座標の列名。
	const char* sPosYColumn_ = "y";					//! 中心のY座標の列名。
	const char* sPosZColumn_ = "z";					//! 中心のZ座標の列名。
	const char* sRadiusColumn_ = "radius";			//! ばらつかせる半径の列名。
	const char* sChanceColumn_ = "chance";			//! 出る確率の列名。
	const char cCandidateDelimiter_ = ',';			//! 候補どうしの区切り文字。

	/**
	 * @brief 前後の空白を取り除く。表を読みやすく書けるよう、候補の間に空白を入れても通るようにする。
	 * @param sText 対象の文字列。
	 * @return 取り除いた文字列。
	 */
	std::string Trim(const std::string& sText)
	{
		const char* sWhitespace = " \t\r\n";

		const size_t iFirst = sText.find_first_not_of(sWhitespace);
		if (iFirst == std::string::npos)
			return "";

		const size_t iLast = sText.find_last_not_of(sWhitespace);
		return sText.substr(iFirst, iLast - iFirst + 1);
	}
}

namespace nsApp
{
	namespace nsItem
	{
		bool PickupSpawnSetting::Create(const nsK2EngineLow::nsSystem::nsTSVFile::TSVTable& table, int row, PickupSpawnSetting& outSetting)
		{
			/* 候補をカンマで切り分ける。空欄の候補は数に入れない。*/
			const std::string sCandidates = table.GetString(row, sCandidatesColumn_);
			size_t iBegin = 0;
			while (iBegin <= sCandidates.size())
			{
				size_t iEnd = sCandidates.find(cCandidateDelimiter_, iBegin);
				if (iEnd == std::string::npos)
					iEnd = sCandidates.size();

				const std::string sName = Trim(sCandidates.substr(iBegin, iEnd - iBegin));
				if (!sName.empty())
					outSetting.vecCandidates_.push_back(sName);

				iBegin = iEnd + 1;
			}

			/* 置くものが1つも書かれていない行は使わない。*/
			if (outSetting.vecCandidates_.empty())
				return false;

			/* 中心を読む。空欄は0。*/
			outSetting.vCenter_.Set
			(
				table.GetFloat(row, sPosXColumn_),
				table.GetFloat(row, sPosYColumn_),
				table.GetFloat(row, sPosZColumn_)
			);

			/* ばらつきと確率を読む。空欄なら「その場所ちょうどに必ず置く」。*/
			outSetting.fRadius_ = table.GetFloat(row, sRadiusColumn_, 0.0f);
			outSetting.fChance_ = table.GetFloat(row, sChanceColumn_, 1.0f);

			/* 半径が負の行は打ち間違いとみなして使わない。*/
			return outSetting.fRadius_ >= 0.0f;
		}
	}
}
