#pragma once

namespace nsApp
{
	namespace nsScene
	{
		/**
		 * @file   InGameBuildHelper.h
		 * @brief  ゲーム中シーンの初期構築(プレイヤー・敵・ステージ配置など)を
		 *         補助するクラス。InGameSceneの肥大化を防ぐために処理を切り出す。
		 *         現状は最小の枠のみ。今後ここに配置処理をまとめていく。
		 * @author Izumida Kiryu
		 * @date   2026/08/19
		 */
		class InGameBuildHelper
		{
		public:
			/* コンストラクタとデストラクタ。*/
			InGameBuildHelper() = default;
			~InGameBuildHelper() = default;


		public:
			/**
			 * @brief シーンの初期構築を行う。
			 */
			void Build();
		};
	}
}
