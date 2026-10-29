#include "stdafx.h"
#include "VideoScreen.h"
#include "../../../System/Video/Video/Src/VideoPlayer.h"
#include "../../../System/Video/Video/Src/PCMDecoder.h"
#include "../../../System/Video/Video/Src/PCMPlayer.h"


namespace nsApp
{
	namespace nsUI
	{
		bool VideoScreen::Start()
		{
			/* 再生したい動画のパスをセットする。*/
			if (!player_.Open(g_graphicsEngine->GetD3DDevice(), /*L"Assets/video/解説動画.mp4"*/))
				return false;

			init_.m_textures[0] = player_.GetTexture();
			init_.m_fxFilePath = "Assets/shader/sprite.fx";
			init_.m_width = player_.GetWidth();
			init_.m_height = player_.GetHeight();
			init_.m_alphaBlendMode = AlphaBlendMode_None;
			sprite_.Init(init_);
			drawer_.pSprite_ = &sprite_;
			sprite_.Update(Vector3::Zero, Quaternion::Identity, Vector3::One);

			/* 音声の再生。*/
			if (pcmDecoder_.Open(L"Assets/video/解説動画.mp4"))
			{
				/* PCMの情報を読み込む。*/
				pcmPlayer_.Play(pcmDecoder_,g_soundEngine->CreatePCMVoice(pcmDecoder_.GetChannelSize(),pcmDecoder_.GetSampleRate(),pcmDecoder_.GetBitsPerSample()));
				pcmDecoder_.Close();
			}

			return true;
		}


		void VideoScreen::Update()
		{
			/* 終端まで来ていなければ、次のフレームをテクスチャへ載せる。*/
			if (player_.IsEnd())
				return;

			player_.Update(g_graphicsEngine->GetCommandList(), 1.0f/60.0f);
			sprite_.Update(Vector3::Zero, Quaternion::Identity, Vector3::One);
		}


		void VideoScreen::Render(RenderContext& rc)
		{
			/* スプライトを描く。*/
			g_renderingEngine->AddRenderObject(&drawer_);
		}
	}
}