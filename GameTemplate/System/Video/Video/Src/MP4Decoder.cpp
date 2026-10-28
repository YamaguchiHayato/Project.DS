#include "pch.h"
#include "MP4Decoder.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

/**/
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")


namespace nsApp
{
	namespace nsVideo
	{
		bool MP4Decoder::Open(const wchar_t* filePath)
		{
			/* デコードの開始を行う。*/
			if (!StartUP())
				return false;

			/* IMFSourceReaderの作成を行う。*/
			if (!CreateReader(filePath))
				return false;

			/* RGBフォーマットの設定を行う。*/
			if (!SetRGBFormat())
				return false;

			/* ビデオの幅と高さの取得を行う。*/
			return ReadSize();
		}


		bool MP4Decoder::StartUP()
		{
			/* Media Foundationの初期化を行う。*/
			result_ = MFStartup(MF_VERSION);
			return SUCCEEDED(result_);
		}


		bool MP4Decoder::CreateReader(const wchar_t* filePath)
		{
			/* IMFSourceReaderの作成を行う。*/
			result_ = MFCreateAttributes(&pAttribute_, 1);

			/* 失敗した場合、処理を止める。*/
			if(Failed())
				return false;

			/* IMFSourceReaderの作成を行う。*/
			pAttribute_->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
			result_ = MFCreateSourceReaderFromURL(filePath, pAttribute_, &pReader_);
			ReleaseAttribute();

			/* 失敗した場合、処理を止める。*/
			if (Failed())
				return false;


			return true;
		}


		bool MP4Decoder::SetRGBFormat()
		{
			/* ファイル側の幅と高さを取る。*/
			result_ = pReader_->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &pType_);
			if (Failed())
				return false;
			/* フレームレートの取得を行う。*/
			MFGetAttributeRatio(pType_, MF_MT_FRAME_RATE, &rateNumerator_, &rateDenominator_);
\
			/* ビデオの幅と高さの取得を行う。*/
			MFGetAttributeSize(pType_, MF_MT_FRAME_SIZE, &width_, &height_);
			ReleaseMediaType();

			/* 出力形式を作り直す。*/
			result_ = MFCreateMediaType(&pType_);
			if (Failed())
				return false;

			/* 幅と高さを付けて RGB32 を指定し、リーダーへ渡す。*/
			pType_->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
			pType_->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
			MFSetAttributeSize(pType_, MF_MT_FRAME_SIZE, width_, height_);
			result_ = pReader_->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pType_);
			ReleaseMediaType();
			if (Failed())
				return false;

			/* ビデオの幅と高さの取得を行う。*/
			return width_ != 0 && height_ != 0;
		}


		bool MP4Decoder::ReadSize()
		{
			/* ビデオの形式を取得する。*/
			result_ = pReader_->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pType_);

			/* 失敗した場合、処理を止める。*/
			if (Failed())
				return false;

			/* ビデオの幅と高さの取得を行う。*/
			MFGetAttributeSize(pType_, MF_MT_FRAME_SIZE, &width_, &height_);
			ReleaseMediaType();

			return width_ != 0 && height_ != 0;
		}


		bool MP4Decoder::ReadFrame()
		{
			/* サンプルの取得を行う。*/
			if (!TakeSample())
				return false;

			/* フレームのコピーを行う。*/
			return CopyFrame();
		}


		bool MP4Decoder::TakeSample()
		{
			/* サンプルの取得を行う。*/
			ReleaseSample();

			/* Sample が空になるまで待機。 */
			while (pSample_ == nullptr)
			{
				/* サンプルの取得を行う。*/
				result_ = pReader_->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, &flags_, nullptr, &pSample_);

				/* 失敗した場合、処理を止める。*/
				if (Failed())
					return false;

				/* サンプルが空の場合、次のサンプルを取得する。*/
				if ((flags_ & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
					return false;
			}

			return true;
		}


		bool MP4Decoder::CopyFrame()
		{
			/* フレームのコピーを行う。*/
			result_ = pSample_->ConvertToContiguousBuffer(&pBuffer_);
			if(Failed())
				return false;

			/* フレームのロックを行う。*/
			result_ = pBuffer_->Lock(&pFrame_, nullptr, &frameSize_);
			if (Failed())
				return false;

			/* フレームのコピーを行う。*/
			frame_.assign(pFrame_, pFrame_ + frameSize_);
			ReleaseBuffer();
			ReleaseSample();

			return true;
		}


		void MP4Decoder::ReleaseSample()
		{
			/* null の場合、処理を止める。*/
			if (pSample_ == nullptr)
				return;

			/* IMFSampleを解放する。*/
			pSample_->Release();
			pSample_ = nullptr;
		}


		void MP4Decoder::ReleaseBuffer()
		{
			/* null の場合、処理を止める。*/
			if (pBuffer_ == nullptr)
				return;

			/* null じゃない場合、ロック解除する。*/
			if (pFrame_ != nullptr)
			{
				pBuffer_->Unlock();
				pFrame_ = nullptr;
			}

			/* IMFMediaBufferを解放する。*/
			pBuffer_->Release();
			pBuffer_ = nullptr;	
		}


		bool MP4Decoder::Failed()
		{
			/* HRESULTの結果が失敗かどうかを判定する。*/
			if(SUCCEEDED(result_))
				return false; 

			/* 失敗した場合、HRESULTの結果を出力する。*/
			Close();
			return true;			
		}


		void MP4Decoder::ReleaseMediaType()
		{
			/* null の場合、処理を止める。*/
			if (pType_ == nullptr)
				return;

			/* IMFMediaTypeを解放する。*/
			pType_->Release();
			pType_ = nullptr;
		}


		void MP4Decoder::ReleaseAttribute()
		{
			/* null の場合、処理を止める。*/
			if (pAttribute_ == nullptr)
				return;

			/* IMFAttributesを解放する。*/
			pAttribute_->Release();
			pAttribute_ = nullptr;
		}


		void MP4Decoder::Close()
		{
			/* IMFMediaTypeとIMFAttributesを解放する。*/
			ReleaseSample();
			ReleaseBuffer();
			ReleaseMediaType();
			ReleaseAttribute();

			/* IMFSourceReaderを解放する。*/
			if (pReader_)
			{
				pReader_->Release();
				pReader_ = nullptr;
			}

			/* シャットダウンする。*/
			MFShutdown();
		}


		const unsigned char* MP4Decoder::GetFrame() const
		{
			/* エンプティの取得後、nullを返す。*/
			if (frame_.empty())
				return nullptr;

			/* フレームのデータへのポインタを取得する。*/
			return frame_.data();
		}


		unsigned int MP4Decoder::GetFrameSize() const
		{
			/* フレームサイズを取得する。*/
			return static_cast<unsigned int>(frame_.size());
		}


		unsigned int MP4Decoder::GetWidth() const
		{
			/* ビデオの幅を取得する。*/
			return width_;
		}


		unsigned int MP4Decoder::GetHeight() const
		{
			/* ビデオの高さを取得する。*/
			return height_;
		}


		float MP4Decoder::GetFrameTime() const
		{
			/* フレームごとの時間帯を取得する。*/
			if (rateNumerator_ == 0)
				return 1.0f / 30.0f;

			/* フレームごとの時間帯を取得する。*/
			return static_cast<float>(rateDenominator_) / static_cast<float>(rateNumerator_);
		}
	}
}