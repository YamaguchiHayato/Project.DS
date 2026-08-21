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

	namespace nsEvent    { class EventBus; }		//! 前方宣言。
	namespace nsRule     { class ResolveGameWinner; }	//! 前方宣言。
	namespace nsUI       { class InGameHud; }		//! 前方宣言。
	namespace nsDirector { class EnemyDirector; }	//! 前方宣言。

	namespace nsScene
	{
		/**
		 * @file   InGameScene.h
		 * @brief  本編(ソロ)シーン。プレイヤー・敵・ステージ・進行を束ねる合成ルート。
		 *         肥大化を防ぐため、実際のゲームロジックは各システム(Player/EnemyDirector/
		 *         ResolveGameWinner 等の GameObject)側に持たせ、ここは生成と一人称カメラ追従に徹する。
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
			 * @brief 状況をイベント発行し、勝敗管理(ResolveGameWinner)の決着を見てリザルトへ橋渡しする。
			 * @details センサ(到達/死亡)→EventBus発行→GameRuleが勝敗確定→ここでシーン遷移を予約。
			 */
			void UpdateResultJudge();


		private:
			nsActor::Player*		pPlayer_ = nullptr;			//! プレイヤー。
			ModelRender				stGroundModel_;				//! 地面。
			nsDirector::EnemyDirector*	pEnemyDirector_ = nullptr;	//! 敵の湧き係。CommonEnemy を時間・上限で湧かせる。

			Vector3					vSafeRoomPos_ = { 0.0f, 0.0f, 1500.0f };	//! セーフルーム(ゴール)の中心位置。
			float					fSafeRoomRadius_ = 150.0f;	//! セーフルーム到達判定の半径(水平)。
			ModelRender				stSafeRoomModel_;			//! ゴールの目印。
			bool					bResultRequested_ = false;	//! リザルト遷移を予約済みか(多重予約防止)。

			nsEvent::EventBus*			pEventBus_ = nullptr;		//! イベントバス(発行/購読の仲介)。
			nsRule::ResolveGameWinner*	pGameRule_ = nullptr;		//! 勝敗管理(バスを購読)。
			bool						bReachPublished_ = false;	//! セーフルーム到達を発行済みか。

			nsUI::InGameHud*			pHud_ = nullptr;			//! HUD(HP/弾/目標/クロスヘア/ダウン表示)。
		};
	}
}
