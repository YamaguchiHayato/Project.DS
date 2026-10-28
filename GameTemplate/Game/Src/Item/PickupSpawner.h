#pragma once
#include "Src/Item/PickupSpawnSetting.h"

namespace nsApp
{
	namespace nsItem
	{
		/**
		 * @file   PickupSpawner.h
		 * @brief  配置表(TSV)を読んで、物資をステージへ1回だけ撒く係。
		 * @details 「どこに置くか」「何を置くか」の両方を毎回抽選するので、
		 *          ニューゲームのたびに配置も落ちている銃の種類も変わる。
		 *          敵と違って時間で湧かせる必要が無いので、GameObject にはしていない。
		 *          シーンの開始時に Spawn を1回呼ぶだけで使える。
		 * @author Izumida Kiryu
		 * @date   2026/10/26
		 */
		class PickupSpawner
		{
		public:
			/* コンストラクタとデストラクタ。*/
			PickupSpawner() = default;
			virtual ~PickupSpawner() = default;


		public:
			/**
			 * @brief 配置表を読んで物資を撒く。
			 * @param pFilePath 配置表(TSV)のファイルパス。
			 * @return 1つでも置けたら true。表が読めなければ false(物資なしで続行する)。
			 */
			bool Spawn(const char* pFilePath);


		private:
			/**
			 * @brief 1地点ぶんの抽選をして、当たりなら物資を1つ置く。
			 * @param stSetting その地点の設定。
			 * @return 置いたら true。確率に外れた場合と、名前が分からなかった場合は false。
			 */
			bool PlaceOne(const PickupSpawnSetting& stSetting);

			/**
			 * @brief 設定の中心から、半径のぶんだけランダムにずらした位置を作る。
			 * @param stSetting その地点の設定。
			 * @return 置く位置。
			 */
			Vector3 MakeScatteredPosition(const PickupSpawnSetting& stSetting) const;
		};
	}
}
