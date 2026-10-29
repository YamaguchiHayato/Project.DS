#pragma once
#include <vector>

struct IMSourceReader;
struct IMFSourceReader;
struct IMFAttributes;
struct IMFMediaType;
struct IMFSample;
struct IMFMediaBuffer;

namespace nsApp
{
	namespace nsVideo
	{
		/**
		 * @file PcmDecoder.h
		 * @brief MP4の音声データをPCM形式にするクラス。
		 * @autor Yamaguchi Hayato
		 * @date 2026/10/23
		 */
		class PCMDecoder
		{
		public:
			/**
			 * @brief MP4の音声を開く。
			 * @param filePath MP4ファイルのパス。
			 */
			bool Open(const wchar_t* filePath);

			/**
			 * @brief 音声情報を閉じる。
			 */
			void Close();

			/**
			 * @brief PCMの情報を読む。
			 */
			bool Read();

			/**
			 * @brief 読んだらPCMの先頭配列を返す。
			 */
			const unsigned char* GetPCM();

			/**
			 * @brief 呼んだPCMのバイト数を返す。
			 * @return バイト数。
			 */
			unsigned int GetPCMSize() const;

			/**
			 * @brief 
			 * @return 
			 */
			inline unsigned int GetChannelSize() const
			{
				return channels_;
			}

			/**
			 * @brief サンプルレートを返す。
			 * @return 1秒あたりのサンプル数。
			 */
			inline unsigned int GetSampleRate() const
			{
				return sampleRate_;
			}

			/**
			 * @brief 1サンプルのビット数を返す。
			 * @return ビット数。PCMは16。
			 */
			inline unsigned int GetBitsPerSample() const
			{
				return bitsPerSample_;
			}


		private:
			/**
			 * @brief 
			 * @return 
			 */
			bool StartUP();

			/**
			 * @brief 
			 * @return 
			 */
			bool CreateRender(const wchar_t* filePath);

			/**
			 * @brief 
			 * @return 
			 */
			bool ReadFormat();

			/**
			 * @brief 
			 * @return 
			 */
			bool SetPCMFormat();

			/**
			 * @brief 
			 * @return 
			 */
			bool TakeSample();

			/**
			 * @brief 
			 * @return 
			 */
			bool CopyPCM();

			/**
			 * @brief 
			 */
			void ReleaseSample();

			/**
			 * @brief 
			 */
			void ReleaseBuffer();

			/**
			 * @brief 
			 * @return 
			 */
			bool Failed();

			/**
			 * @brief 
			 */
			void ReleaseMediaType();

			/**
			 * @brief 
			 */
			void ReleaseAttribute();


		private:
			IMFSourceReader* pReader_ = nullptr; //! 音声の読み取り。
			IMFMediaType* pType_ = nullptr; //! 音声の形式。
			IMFAttributes* pAttribute_ = nullptr; //! 読み取りの設定。
			IMFSample* pSample_ = nullptr; //! PCMの1本。
			IMFMediaBuffer* pBuffer_ = nullptr; //! PCMのバッファ。
			unsigned char* pPcm_ = nullptr; //! PCMの先頭。
			unsigned int channels_ = 0; //! チャンネル数。
			unsigned int sampleRate_ = 0; //! サンプルレート。
			unsigned int bitsPerSample_ = 0; //! 1サンプルのビット数。
			unsigned long flags_ = 0; //! ReadSample の状態。
			unsigned long pcmSize_ = 0; //! PCMのバイト数。
			long result_ = 0; //! HRESULTの結果。
			std::vector<unsigned char> pcm_; //! コピーしたPCM。
		};
	}
}