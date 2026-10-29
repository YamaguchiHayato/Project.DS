#include "pch.h"
#include "PCMDecoder.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")


namespace nsApp
{
	namespace nsVideo
	{
		bool PCMDecoder::Open(const wchar_t* filePath)
		{
			/* Media Foundationの初期化。*/
			if (!StartUP())
				return false;

			/* 音声の読み取りを作る。*/
			if (!CreateRender(filePath))
				return false;

			/* 音声の形式を読む。*/
			if(!ReadFormat())
				return false;

			/* PCMの形式を設定する。*/
			return SetPCMFormat();
		}


		bool PCMDecoder::StartUP()
		{
			/* Media Foundationの初期化。*/
			result_ = MFStartup(MF_VERSION);
			return SUCCEEDED(result_);
		}


		bool PCMDecoder::CreateRender(const wchar_t* filePath)
		{
			/* 音声の読み取りを作る。*/
			result_ = MFCreateSourceReaderFromURL(filePath, nullptr, &pReader_);

			/* 失敗したらfalseを返す。*/
			if(Failed())
				return false;

			return true;
		}


		bool PCMDecoder::ReadFormat()
		{
			/* 音声の形式を読む。*/
			result_ = pReader_->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &pType_);
			if (Failed())
				return false;

			/* チャンネル数とサンプルレートを読む。*/
			pType_->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels_);
			pType_->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate_);	
			ReleaseMediaType();

			/* チャンネル数とサンプルレートが0でないか確認する。*/
			return channels_ != 0 && sampleRate_ != 0;
		}


		bool PCMDecoder::Read()
		{
			/* PCMを1本取る。*/
			if (!TakeSample())
				return false;

			return CopyPCM();
		}



		bool PCMDecoder::SetPCMFormat()
		{
			/* PCMの形式を作る。*/
			result_ = MFCreateMediaType(&pType_);
			if(Failed())
				return false;

			/* PCMの形式を設定する。*/
			bitsPerSample_ = 16;
			pType_->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
			pType_->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
			pType_->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, channels_);
			pType_->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate_);
			pType_->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, bitsPerSample_);
			pType_->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, channels_ * 2);
			pType_->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, sampleRate_ * channels_ * 2);
			result_ = pReader_->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pType_);

			/* 失敗したらfalseを返す。*/
			ReleaseMediaType();
			if (Failed())
				return false;

			/* PCMの形式を確認する。*/
			return true;
		}


		bool PCMDecoder::TakeSample()
		{
			/* PCMのサンプルを読む。*/
			ReleaseSample();

			/* PCMのサンプルが取れるまでループする。*/
			while (pSample_ == nullptr)
			{
				/* PCMのサンプルを読む。*/
				result_ = pReader_->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags_, nullptr, &pSample_);

				/* 失敗したらfalseを返す。*/
				if (Failed())
					return false;

				/* PCMのサンプルが取れたらtrueを返す。*/
				if ((flags_ & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
					return false;
			}

			return true;	
		}


		bool PCMDecoder::CopyPCM()
		{
			/* PCMを連続したバッファにする。*/
			result_ = pSample_->ConvertToContiguousBuffer(&pBuffer_);
			if (Failed())
				return false;

			/* PCMの先頭を読む。*/
			result_ = pBuffer_->Lock(&pPcm_, nullptr, &pcmSize_);
			if (Failed())
				return false;

			/* PCMをコピーする。*/
			pcm_.assign(pPcm_, pPcm_ + pcmSize_);
			ReleaseBuffer();
			ReleaseSample();
			return true;
		}


		void PCMDecoder::ReleaseBuffer()
		{
			/* null の場合、処理をとめる。*/
			if (pBuffer_ == nullptr)
				return;

			/* PCMの先頭が null でない場合、アンロックする。*/
			if (pPcm_ != nullptr)
			{
				pBuffer_->Unlock();
				pPcm_ = nullptr;
			}

			/* PCMのバッファを解放する。*/
			pBuffer_->Release();
			pBuffer_ = nullptr;
		}


		void PCMDecoder::ReleaseSample()
		{
			/* null の場合、処理をとめる。*/
			if (pSample_ == nullptr)
				return;

			/* サンプルを解放する。*/
			pSample_->Release();
			pSample_ = nullptr;
		}


		bool PCMDecoder::Failed()
		{
			/* 成功したらfalseを返す。*/
			if (SUCCEEDED(result_))
				return false;

			/* 失敗したらMedia Foundationを終了する。*/
			Close();
			return true;
		}


		void PCMDecoder::ReleaseMediaType()
		{
			/* null の場合、処理をとめる。*/
			if (pType_ == nullptr)
				return;

			/* PCMの形式を解放する。*/
			pType_->Release();
			pType_ = nullptr;
		}


		void PCMDecoder::ReleaseAttribute()
		{
			/* null の場合、処理をとめる。*/
			if (pAttribute_ == nullptr)
				return;

			/* PCMの属性を解放する。*/
			pAttribute_->Release();
			pAttribute_ = nullptr;
		}


		void PCMDecoder::Close()
		{
			/* PCMのサンプルを解放する。*/
			ReleaseSample();
			ReleaseBuffer();
			ReleaseMediaType();
			ReleaseAttribute();

			/* PCMの読み取りを解放する。*/
			if (pReader_ != nullptr)
			{
				pReader_->Release();
				pReader_ = nullptr;
			}

			/* Media Foundationを終了する。*/
			MFShutdown();
		}


		const unsigned char* PCMDecoder::GetPCM()
		{
			/* PCMが空の場合、nullptrを返す。*/
			if (pcm_.empty())
				return nullptr;

			/* PCMの先頭を返す。*/
			return pcm_.data();
		}


		unsigned int PCMDecoder::GetPCMSize() const
		{
			/* PCMのバイト数を返す。*/
			return static_cast<unsigned int>(pcm_.size());
		}
	}
}