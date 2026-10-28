#pragma once
#include <string>
#include <vector>

namespace nsApp
{
	namespace nsItem
	{
		/**
		 * @file   PickupSpawnSetting.h
		 * @brief  物資が出るかもしれない1地点ぶんの設定。配置表(TSV)の1行に当たる。
		 * @details 候補を複数書けるようにしてあり、どれを置くかはニューゲームのたびに抽選する。
		 *          「何を置くか」をデータ側で決められるので、ステージごとに出る銃を変えられる。
		 * @author Izumida Kiryu
		 * @date   2026/10/26
		 */
		struct PickupSpawnSetting
		{
			std::vector<std::string> vecCandidates_;	//! 置く候補の名前。この中から1つ抽選する。
			Vector3 vCenter_ = Vector3::Zero;			//! 置く場所の中心。
			float fRadius_ = 0.0f;						//! この半径の円の中へランダムにずらす。0ならその場所ちょうど。
			float fChance_ = 1.0f;						//! その地点に物資が出る確率(0〜1)。

			/**
			 * @brief TSVの1行から1地点ぶんの設定を作る。
			 * @param table      読み込んだ表。
			 * @param row        行番号。
			 * @param outSetting 作った設定。
			 * @return 使える行なら true。候補が1つも書かれていない行、半径が負の行は false。
			 */
			static bool Create(const nsK2EngineLow::nsSystem::nsTSVFile::TSVTable& table, int row, PickupSpawnSetting& outSetting);
		};
	}
}
