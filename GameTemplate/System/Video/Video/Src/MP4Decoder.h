#pragma once
#include <vector>

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
		 * @file MP4Decoder.h
		 * @brief MP4ファイルを開くクラス。
		 * @details MP4ファイルを開き、デコードするための機能を提供します。
					IMFSourceRenderインターフェースを使用して、MP4ファイルのデータをレンダリングします。
		 * @autor Yamaguchi Hayato
		 * @date 2026/10/22
		 */
		class MP4Decoder
		{
		public:
			/**
			 * @brief MP4ファイルを開く。
			 * @param filePath MP4ファイルのパス。
			 * @return 最終的なファイルパス。
			 */
			bool Open(const wchar_t* filePath);

			/**
			 * @brief MP4ファイルを閉じる。
			 * @details MP4ファイルを閉じ、リソースを解放します。
			 */
			void Close();

			/**
			 * @brief ビデオの幅を取得する。
			 * @return ビデオの幅。
			 */
			unsigned int GetWidth() const;

			/**
			 * @brief ビデオの高さを取得する。
			 * @return ビデオの高さ。
			 */
			unsigned int GetHeight() const;

			/**
			 * @brief MP4ファイルのフレームを読み取る。
			 * @return フレームの読み取りに成功した場合はtrue、失敗した場合はfalse。
			 */
			bool ReadFrame();

			/**
			 * @brief MP4ファイルのフレームを取得する。
			 * @return フレームのデータへのポインタ。
			 */
			const unsigned char* GetFrame() const;

			/**
			 * @brief MP4ファイルのフレームサイズを取得する。
			 * @return フレームサイズ。
			 */
			unsigned int GetFrameSize() const;


		private:
			/**
			 * @brief MP4ファイルのデコードを開始する。
			 * @return デコードの開始に成功した場合はtrue、失敗した場合はfalse。
			 */
			bool StartUP();

			/**
			 * @brief IMFSourceReaderを作成する。
			 * @param filePath MP4ファイルのパス。
			 * @return IMFSourceReaderの作成に成功した場合はtrue、失敗した場合はfalse。
			 */
			bool CreateReader(const wchar_t* filePath);

			/**
			 * @brief RGBフォーマットを設定する。
			 * @return RGBフォーマットの設定に成功した場合はtrue、失敗した場合はfalse。
			 */
			bool SetRGBFormat();

			/**
			 * @brief ビデオの幅と高さを取得する。
			 * @return ビデオの幅と高さの取得に成功した場合はtrue、失敗した場合はfalse。
			 */
			bool ReadSize();

			/**
			 * @brief MP4ファイルのサンプルを取得する。
			 * @return サンプルの取得に成功した場合はtrue、失敗した場合はfalse。
			 */
			bool TakeSample();

			/**
			 * @brief MP4ファイルのフレームをコピーする。
			 * @return フレームのコピーに成功した場合はtrue、失敗した場合はfalse。
			 */
			bool CopyFrame();

			/**
			 * @brief IMFSampleを解放する。
			 */
			void ReleaseSample();

			/**
			 * @brief IMFMediaBufferを解放する。
			 */
			void ReleaseBuffer();

			/**
			 * @brief HRESULTの結果が失敗かどうかを判定する。
			 * @return HRESULTの結果が失敗の場合はtrue、成功の場合はfalse。
			 */
			bool Failed();

			/**
			 * @brief IMFMediaTypeを解放する。
			 * @details IMFMediaTypeを解放します。
			 */
			void ReleaseMediaType();

			/**
			 * @brief IMFAttributesを解放する。
			 * @details IMFAttributesを解放します。
			 */
			void ReleaseAttribute();


		private:
			IMFSourceReader* pReader_ = nullptr; //! MP4の読み取り。
			IMFMediaType* pType_ = nullptr; //! 映像の形式。
			IMFAttributes* pAttribute_ = nullptr; //! 読み取りの設定。
			IMFSample* pSample_ = nullptr; //! 1フレーム。
			IMFMediaBuffer* pBuffer_ = nullptr; //! フレームの画素。
			unsigned int width_ = 0; //! 映像の幅。
			unsigned int height_ = 0; //! 映像の高さ。
			unsigned char* pFrame_ = nullptr;		//! 画素の先頭。
			unsigned long flags_ = 0; //! ReadSample の状態。
			unsigned long frameSize_ = 0; //! 画素のバイト数。
			long result_ = 0; //! HRESULTの結果。
			std::vector<unsigned char> frame_; //! 画素のバイト数。
		};
	}
}