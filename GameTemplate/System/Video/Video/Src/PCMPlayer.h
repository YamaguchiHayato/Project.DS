#pragma once
#include <vector>
#include <xaudio2.h>
#include "PCMDecoder.h"

namespace nsApp
{
	namespace nsVideo
	{
		class PCMPlayer
		{
		public:
			/* デストラクタ。*/
			~PCMPlayer()
			{
				Close();
			}


		public:
			/**
			 * @brief 音声データを読込、再生を開始する。
			 * @param decoder 開く音声データのPCMデータ。
			 * @param pVoice CreatePCMVoice で作ったボイス。
			 * @return 再生を開始できたら true。
			 */
			bool Play(PCMDecoder& decoder, IXAudio2SourceVoice* pVoice);

			/**
			 * @brief 再生を閉じる。 
			 */
			void Close();


		private:
			std::vector<unsigned char> pcm_; //! 再生中に保持するPCM。
			IXAudio2SourceVoice* pVoice_ = nullptr; //! 再生に使うボイス。
			XAUDIO2_BUFFER buffer_ = {}; //! ボイスへ渡すバッファ。
			long result_ = 0; //! XAudio2の戻り値。
		};
	}
}