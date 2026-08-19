#pragma once
#include "Src/Scene/IScene.h"

namespace nsApp
{
	namespace nsScene
	{
		/**
		 * @file   InGameScene.h
		 * @brief ゲーム中シーン。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/18
		 * @details 過去タイトルからクラスを引用予定。
		 */
		class InGameScene : public IScene
		{
		public:
			/* コンストラクタとデストラクタ。*/
			InGameScene() = default;
			virtual ~InGameScene() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override {}
		};
	}
}