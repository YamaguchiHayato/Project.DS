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
			time_ = decoder_.GetFrameTime();
			isEnd_ = false;
			return true;
		}


		bool VideoPlayer::Update(ID3D12GraphicsCommandList* pCommandList, float deltaTime)
		{
			/* 開いていない、または終端なら進めない。*/
			if (!isOpened_ || isEnd_)
				return false;

			/* 経過時間を足し、フレーム時間に達していなければ進めない。*/
			time_ += deltaTime;
			if (time_ < decoder_.GetFrameTime())
				return true;
			/* フレーム時間に達したら、次のフレームへ進める。*/
			time_ -= decoder_.GetFrameTime();

			/* 次の画素を読み、テクスチャへコピーする。*/
			if (!decoder_.ReadFrame())
			{
				isEnd_ = true;
				return false;
			}

			/* 読めたらテクスチャへコピーする。*/
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