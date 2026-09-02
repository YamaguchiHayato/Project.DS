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

			/**
			 * @brief 直近の試合結果(勝ち=true)を記録する。ResultScene へ渡す受け皿。
			 * @param bWon 勝利したら true。
			 */
			inline void SetMatchWon(bool bWon)
			{
				/* 勝敗を保持する。*/
				bMatchWon_ = bWon;
			}

			/**
			 * @brief 直近の試合結果を取得する。
			 * @return 勝利していたら true。
			 */
			/**
			 * @brief 直近の試合の戦績を記録する。ResultScene へ渡す受け皿。
			 * @param iKillCount 撃破数。
			 * @param fClearTime かかった時間(秒)。
			 */
			inline void SetMatchRecord(int iKillCount, float fClearTime)
			{
				iMatchKillCount_ = iKillCount;
				fMatchClearTime_ = fClearTime;
			}

			//! 直近の試合の撃破数。
			inline int GetMatchKillCount() const { return iMatchKillCount_; }

			//! 直近の試合にかかった時間(秒)。
			inline float GetMatchClearTime() const { return fMatchClearTime_; }

			inline bool IsMatchWon() const
			{
				/* 保持した勝敗を返す。*/
				return bMatchWon_;
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
			bool bMatchWon_ = false; //! 直近の試合結果(勝ち=true)。ResultSceneへ渡す。
			int iMatchKillCount_ = 0; //! 直近の試合の撃破数。
			float fMatchClearTime_ = 0.0f; //! 直近の試合にかかった時間(秒)。
		};
	}
}