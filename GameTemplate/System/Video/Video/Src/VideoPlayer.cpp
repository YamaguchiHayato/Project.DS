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
			isEnd_ = false;
			return true;
		}


		bool VideoPlayer::Update(ID3D12GraphicsCommandList* pCommandList)
		{
			/* 開いていない、または終端なら進めない。*/
			if (!isOpened_ || isEnd_)
				return false;

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