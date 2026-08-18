#pragma once
/**
 * @file   GameFlow.h
 * @brief  シーンの生成と切り替えを管理するクラス。
 * @author Yamaguchi Hayato
 * @date   2026/08/18
 */

#include <functional>
#include <unordered_map>
#include "Src/Scene/IScene.h"

namespace nsApp
{
	namespace nsScene
	{
		class GameFlow : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			GameFlow() = default;
			virtual ~GameFlow() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;


		public:
			/**
			 * @brief シーンを切り替える。
			 * @param enSceneID 切り替え先のシーンID。
			 */
			inline void ChangeScene(EnSceneID enSceneID)
			{
				/* 次フレームで反映する切替先を予約。*/
				enReserveSceneID_ = enSceneID;
				enReserveDebugSceneID_ = EnDebugSceneID::None;
			}

			/**
			 * @brief シーンを切り替える。
			 * @param enSceneID 切り替え先のシーンID。
			 * @param enDebugSceneID 切り替え先のDebugシーンID。
			 */
			inline void ChangeScene(EnSceneID enSceneID, EnDebugSceneID enDebugSceneID)
			{
				/* Debug配下の切替先を予約。*/
				enReserveSceneID_ = enSceneID;
				enReserveDebugSceneID_ = enDebugSceneID;
			}


		private:
			/**
			 * @brief シーン生成関数の型。
			 * @param pGameFlow シーン切替用のポインタ。
			 * @return 生成されたシーンのポインタ。
			 */
			using SceneCreateFunction = std::function<IScene* (GameFlow*)>;

			/**
			 * @brief シーン生成関数を登録する。
			 */
			void RegisterSceneFactory();

			/**
			 * @brief 予約されたシーン切替を反映する。
			 */
			void ApplyReservedSceneChange();

			/**
			 * @brief シーン切替用のキーを生成する。
			 * @param enSceneID SceneID。
			 * @param enDebugSceneID DebugシーンID。
			 * @return 生成されたキー。
			 */
			uint32_t MakeSceneKey(EnSceneID enSceneID, EnDebugSceneID enDebugSceneID) const;


		private:
			std::unordered_map<uint32_t, SceneCreateFunction> mapSceneFactory_;	//! シーン生成関数テーブル。
			IScene* pCurrentScene_ = nullptr; //! 現在のシーン。
			EnSceneID enReserveSceneID_ = EnSceneID::None; //! 予約された通常シーンID。
			EnDebugSceneID enReserveDebugSceneID_ = EnDebugSceneID::None; //! 予約されたDebugシーンID。
		};
	}
}