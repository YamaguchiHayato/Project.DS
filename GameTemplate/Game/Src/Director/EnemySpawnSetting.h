#pragma once
#include "Src/Actor/Character/Enemy/EnEnemyType.h"

namespace nsApp
{
	namespace nsDirector
	{
		/**
		 * @enum  EnSpawnShape
		 * @brief スポナーの湧き方の形。
		 */
		enum class EnSpawnShape
		{
			Point,	//! 中心の1点に湧く。
			Circle,	//! 半径の円の中にランダムに湧く。
			Ring,	//! 円周上に等間隔で湧く(放射状)。

			/* @infomation: 生成方式を足したい場合はここに追加してください。*/
		};

		/**
		 * @file   EnemySpawnSetting.h
		 * @brief  スポナー1つ分の設定。配置表(TSV)の1行に当たる。
		 * @details CreatePatternを活用。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/06: 新規作成日。
		 */
		struct EnemySpawnSetting
		{
			nsActor::EnEnemyType enEnemyType_ = nsActor::EnEnemyType::None; //! 湧かせるゾンビの種類。必ず指定する。
			Vector3 vCenter_ = Vector3::Zero; //! スポナーの中心(cm)。
			EnSpawnShape enShape_ = EnSpawnShape::Point; //! 湧き方の形。
			float fRadius_ = 0.0f; //! 半径(cm)。Point では使わない。
			float fInterval_ = 0.0f; //! 湧く間隔(秒)。0なら最初に1回だけ湧く。
			int iAmount_ = 1; //! 1回に湧かせる数。
			int iTotal_ = 0; //! 合計で湧かせる数の上限。0なら無制限。

			/**
			 * @brief TSVの1行からスポナーの設定を作る。
			 * @param table 読み込んだ表。
			 * @param row 行番号。
			 * @param outSetting 作った設定。
			 * @return 使える行ならtrue。種類名・形が不明、数が0以下、半径が負ならfalse。
			 */
			static bool Create(const nsK2EngineLow::nsSystem::nsTSVFile::TSVTable& table, int row, EnemySpawnSetting& outSetting);
		};
	}
}