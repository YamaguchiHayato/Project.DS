#include "pch.h"
#include "VideoPlayer.h"


namespace nsApp
{
	namespace nsVideo
	{
		bool VideoPlayer::Open(ID3D12Device* pDevice, const wchar_t* filePath)
		{
			/* ファイルを開き、幅と高さが取れてからテクスチャを作る。*/
			if (!decoder_.Open(filePath))
				return false;

			/* 幅と高さのテクスチャを作る。*/
			if (!frameTexture_.Create(pDevice, decoder_.GetWidth(), decoder_.GetHeight()))
			{
				/* テクスチャの作成に失敗したら、デコーダを閉じる。*/
				decoder_.Close();
				return false;
			}

			/* 開けた。*/
			isOpened_ = true;
			QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&frequency_));
			QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&counter_));
			time_ = decoder_.GetFrameTime() + (1.0f / 60.0f);
			isEnd_ = false;
			return true;
		}


		bool VideoPlayer::Update(ID3D12GraphicsCommandList* pCommandList, float deltaTime)
		{
			/* 開いていない、または終端なら進めない。*/
			if (!isOpened_ || isEnd_)
				return false;

			/* 実際に経過した秒数を足す。*/
			QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&now_));
			time_ += static_cast<float>(now_ - counter_) / static_cast<float>(frequency_);
			counter_ = now_;

			/* 大きく遅れたときは、遅れを捨てて今の位置へ付く。*/
			if (time_ > decoder_.GetFrameTime() * 2.0f)
				time_ = decoder_.GetFrameTime() * 2.0f;
			if (time_ < decoder_.GetFrameTime())
				return true;

			/* フレームを進める。*/
			while (time_ >= decoder_.GetFrameTime())
			{
				/* フレームを読み取る。*/
				if (!decoder_.ReadFrame())
				{
					isEnd_ = true;
					return false;
				}

				/* フレームを進めたので、残り時間を減らす。*/
				time_ -= decoder_.GetFrameTime();
				frameIndex_ += 1.0f;
			}

			/* フレームをテクスチャへコピーする。*/
			return frameTexture_.Copy(pCommandList, decoder_.GetFrame(), decoder_.GetFrameSize());
		}


		void VideoPlayer::Close()
		{
			/* デコーダを閉じる。テクスチャの転送元は FrameTexture のデストラクタが解放する。*/
			decoder_.Close();
			isOpened_ = false;
		}
	}
}