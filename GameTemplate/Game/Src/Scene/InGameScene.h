#pragma once
#include "Src/Scene/IScene.h"

namespace nsApp
{
	namespace nsActor
	{
		/* 前方宣言。*/
		class Player;
		class CommonEnemy;
	}

	namespace nsScene
	{
		/**
		 * @file   InGameScene.h
		 * @brief  本編(ソロ)シーン。プレイヤー・敵・ステージ・進行を束ねる合成ルート。
		 *         肥大化を防ぐため、実際のゲームロジックは各システム(Player/EnemyDirector/
		 *         GameRuleManager 等の GameObject)側に持たせ、ここは生成と一人称カメラ追従に徹する。
		 * @author Izumida Kiryu
		 * @date   2026/08/20
		 */
		class InGameScene : public IScene
		{
		public:
			/* コンストラクタとデストラクタ。*/
			InGameScene() = default;
			virtual ~InGameScene();


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;


		private:
			/**
			 * @brief カメラのニア・ファーを設定する。
			 */
			void InitCamera();

			/**
			 * @brief 一人称カメラをプレイヤーの目の位置・視線へ追従させる。
			 */
			void UpdateCamera();

			/**
			 * @brief 勝敗を判定し、決着していればリザルトへ遷移予約する。
			 * @details 勝利=セーフルーム到達、敗北=プレイヤー死亡。
			 *          ※次段で GameRuleManager(State)＋EventBus へ引き上げる暫定判定。
			 */
			void UpdateResultJudge();


		private:
			nsActor::Player*		pPlayer_ = nullptr;			//! プレイヤー。
			ModelRender				stGroundModel_;				//! 地面。
			nsActor::CommonEnemy*	pTestEnemy_ = nullptr;		//! ★v1動作確認用の敵1体。EnemyDirector実装時に置き換える。

			Vector3					vSafeRoomPos_ = { 0.0f, 0.0f, 1500.0f };	//! セーフルーム(ゴール)の中心位置。
			float					fSafeRoomRadius_ = 150.0f;	//! セーフルーム到達判定の半径(水平)。
			ModelRender				stSafeRoomModel_;			//! ゴールの目印。
			bool					bResultRequested_ = false;	//! リザルト遷移を予約済みか(多重予約防止)。
		};
	}
}
