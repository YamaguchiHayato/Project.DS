#pragma once
#include "../../../System/Video/Video/Src/VideoPlayer.h"
#include "../../../System/Video/Video/Src/PCMDecoder.h"
#include "../../../System/Video/Video/Src/PCMPlayer.h"


namespace nsApp
{
	namespace nsUI
	{
		/**
		 * @file VideoScreen.h
		 * @brief VideoPlayer のテクスチャをスプライトで描く。
		 * @details デコードとテクスチャ更新は VideoPlayer が行う。
		 * @author Yamaguchi Hayato
		 * @date 2026/10/22: 新規作成日。
		 */
		class VideoScreen : public IGameObject
		{
		public:
			/* ライフサイクル。*/
			bool Start()override;
			void Update()override;
			void Render(RenderContext& rc)override;


		public:
			/**
			 * @brief 動画の再生を終了する。
			 * @return 
			 */
			bool IsEnd() const
			{
				return player_.IsEnd();
			}


		private:
			nsVideo::VideoPlayer player_; //! 動画の再生。
			Sprite sprite_; //! 映像を描く板。
			SpriteInitData init_ = {}; //! 初期化データ。


			/**
			 * @file Drawer.h
			 * @brief 描画用のラッパークラス。
			 * @details IRender を継承して、VideoScreen のスプライトを描画する。
			 * @author Yamaguchi Hayato
			 * @date 2026/10/23: 新規作成日。
			 */
			class Drawer : public IRender
			{
			public:
				Sprite* pSprite_ = nullptr; //! 描画するスプライト。


			public:
				/**
				 * @brief 2D描画。
				 * @param rc レンダリングコンテキスト。
				 */
				void OnRender2D(RenderContext& rc) override
				{
					if (pSprite_ != nullptr)
						pSprite_->Draw(rc);
				}
			};

			Drawer drawer_;
			nsVideo::PCMDecoder pcmDecoder_; //! 音声の読み取り。
			nsVideo::PCMPlayer pcmPlayer_; //! 音声の再生。
		};
	}
}