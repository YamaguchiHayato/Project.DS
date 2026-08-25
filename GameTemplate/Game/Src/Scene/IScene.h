#pragma once
#include "stdint.h"

namespace nsApp
{
	namespace nsScene
	{
		/* 循環を防ぐために前方宣言。*/
		class GameFlow;

		/**
		 * @enum EnSceneID
		 * @brief シーンの識別子。
		 */
		enum class EnSceneID : uint8_t
		{
			Title,		//! タイトル。
			Loading,	//! ローディング（予約）。
			InGame,		//! 本編。
			Debug,		//! デバッグ親。
			Result,		//! リザルト（予約）。
			None,		//! 切替なし。
		};

		/**
		 * @enum EnDebugSceneID
		 * @brief デバッグシーンの識別子。
		 */
		enum class EnDebugSceneID : uint8_t
		{
			EnemySolo,		//! 敵単体テスト。
			ShootingRange,	//! 射撃場テスト。
			PlayerSolo,		//! プレイヤー単体テスト。
			SpawnTest,		//! スポーンテスト。
			AITest,			//! AIテスト。
			None,			//! 切替なし。
		};

		/**
		 * @file   IScene.h
		 * @brief  シーンの基底クラス。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/18
		 */
		class IScene : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			IScene() = default;
			virtual ~IScene() = default;


		public:
			/* ライフサイクル。*/
			virtual bool Start() override = 0;
			virtual void Update() override = 0;
			virtual void Render(RenderContext& rc) override {}


		public:
			/**
			 * @brief シーン切替用のポインタを設定する。
			 * @param pGameFlow シーン切替用のポインタ。
			 */
			inline void SetGameFlow(GameFlow* pGameFlow)
			{
				/* シーン切替用のポインタを受け取る。*/
				pGameFlow_ = pGameFlow;
			}

			/**
			 * @brief シーン切替用のポインタを取得する。
			 * @return シーン切替用のポインタ。
			 */
			inline GameFlow* GetGameFlow() const
			{
				/* シーン切替用のポインタを返す。*/
				return pGameFlow_;
			}


		protected:
			GameFlow* pGameFlow_ = nullptr;	 //! シーン切替用のポインタ。
		};
	}
}