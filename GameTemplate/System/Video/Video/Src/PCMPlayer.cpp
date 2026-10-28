#include "pch.h"
#include "PCMPlayer.h"

namespace nsApp
{
	namespace nsVideo
	{
		bool PCMPlayer::Play(PCMDecoder& decoder, IXAudio2SourceVoice* pVoice)
		{
			/* ボイスが無ければ鳴らせない。*/
			pVoice_ = pVoice;

			if (pVoice_ == nullptr)
				return false;

			/* 音声を最後までコピーする。*/
			while (decoder.Read())
				pcm_.insert(pcm_.end(), decoder.GetPCM(), decoder.GetPCM() + decoder.GetPCMSize());
			if (pcm_.empty())
				return false;

			/* 1本のバッファとして再生を開始する。*/
			buffer_.AudioBytes = static_cast<UINT32>(pcm_.size());
			buffer_.pAudioData = pcm_.data();
			buffer_.Flags = XAUDIO2_END_OF_STREAM;
			result_ = pVoice_->SubmitSourceBuffer(&buffer_);

			/* 読み込めない場合、再生しない。*/
			if (FAILED(result_))
				return false;
			result_ = pVoice_->Start(0);
			return SUCCEEDED(result_);
		}


		void PCMPlayer::Close()
		{
			if (pVoice_ == nullptr)
				return;
			pVoice_->Stop(0);
			pVoice_->DestroyVoice();
			pVoice_ = nullptr;
		}
	}
}