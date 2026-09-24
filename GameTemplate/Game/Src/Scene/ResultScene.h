#pragma once
#include "Src/Scene/IScene.h"

namespace nsApp
{
	namespace nsScene
	{
		/**
		 * @file   ResultScene.h
		 * @brief  リザルトシーン。GameFlow が保持する勝敗を表示し、入力でタイトルへ戻す。
		 * @author Izumida Kiryu
		 * @date   2026/08/20
		 */
		class ResultScene : public IScene
		{
		public:
			/* コンストラクタとデストラクタ。*/
			ResultScene() = default;
			virtual ~ResultScene() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;


		private:
			FontRender stResultFont_;			//! 勝敗の見出し。
			FontRender stGuideFont_;			//! 操作ガイド。
			FontRender stRecordFont_;			//! 戦績(撃破数とかかった時間)。
			wchar_t wcRecord_[64] = L"";		//! 戦績の文字列バッファ。
			float		fInputGuardTimer_ = 0.0f;	//! 直前入力での即抜けを防ぐ受付猶予タイマー。
		};
	}
}
