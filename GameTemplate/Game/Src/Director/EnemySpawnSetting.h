#pragma once
#include "Src/Actor/Character/Enemy/EnEnemyType.h"

namespace nsApp
{
	namespace nsDirector
	{
		/**
		 * @file   EnemySpawnSetting.h
		 * @brief  スポナー1つ分の設定。配置表(TSV)の1行に当たる。
		 * @details CreatePatternを活用。
		 *          マイクラのスポナー寄せ。プレイヤーが作動距離内にいる間、間隔ごとに円内XZへ無限に湧く。
		 *          形(Point/Circle/Ring)と合計上限(total)は持たない。Yは使わず常に0。
		 *          付近の敵が maxNearby 以上ならその回は湧かせない(ゲームが敵で埋まらないようにするブレーキ)。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/06: 新規作成日。
		 * @date   2026/11/05: 無限湧き向けに項目を作り直し。shape / total / y を廃止。
		 */
		struct EnemySpawnSetting
		{
			nsActor::EnEnemyType enEnemyType = nsActor::EnEnemyType::None; //! 湧かせるゾンビの種類。必ず指定する。
			Vector3 vCenter = Vector3::Zero; //! スポナーの中心(cm)。Yは常に0(床合わせはスポナー側のレイで行う)。
			float fRadius = 0.0f; //! 湧き円の半径(cm)。0なら中心そのもの。XZだけランダムにずらす。
			float fInterval = 2.0f; //! 湧く間隔(秒)。作動中はこの間隔で試し続ける。0なら毎フレーム試す。
			int iAmount = 1; //! 1回の試行で湧かせる数。
			float fActivateRange = 800.0f; //! 作動距離(cm)。プレイヤーが中心からこの距離以内のときだけ湧く。XZで測る。
			int iMaxNearby = 8; //! 中心付近にいる敵の上限。この数以上いるとその回は湧かせない。0以下は制限なし。

			/**
			 * @brief TSVの1行からスポナーの設定を作る。
			 * @param table 読み込んだ表。
			 * @param row 行番号。
			 * @param outSetting 作った設定。
			 * @return 使える行ならtrue。種類名が不明、数が0以下、半径・間隔・作動距離が負ならfalse。
			 */
			static bool Create(const nsK2EngineLow::nsSystem::nsTSVFile::TSVTable& table, int row, EnemySpawnSetting& outSetting);
		};
	}
}