#pragma once
#include "MP4Decoder.h"
#include "FrameTexture.h"

namespace nsApp
{
	namespace nsVideo
	{
		/**
		 * @file VideoPlayer.h
		 * @brief MP4の再生を、デコードとテクスチャ更新に分ける。
		 * @details ファイルから画素を取る処理は MP4Decoder、画素をGPUへ載せる処理は FrameTexture が行う。
		 *          スプライトで画面へ描く処理は持たない。
		 * @author Yamaguchi Hayato
		 * @date 2026/10/22
		 */
		class VideoPlayer
		{
		public:
			/**
			 * @brief MP4を開き、その幅と高さのテクスチャを作る。
			 * @param pDevice Direct3Dのデバイス。
			 * @param filePath MP4ファイルのパス。
			 * @return 開けたら true。失敗したら false。
			 */
			bool Open(ID3D12Device* pDevice, const wchar_t* filePath);

			/**
			 * @brief 1フレーム進めて、テクスチャへコピーする。
			 * @param pCommandList コピー命令を積むコマンドリスト。
			 * @return フレームを更新できたら true。終端か失敗なら false。
			 */
			bool Update(ID3D12GraphicsCommandList* pCommandList, float deltaTime);

			/**
			 * @brief 再生を終える。
			 */
			void Close();

			/**
			 * @brief スプライトへ渡すテクスチャを返す。
			 * @return 描画用テクスチャ。
			 */
			inline nsK2EngineLow::Texture* GetTexture()
			{
				return frameTexture_.GetTexture(); 
			}

			/**
			 * @brief 映像の幅を返す。
			 * @return 幅。
			 */
			inline unsigned int GetWidth() const 
			{
				return decoder_.GetWidth(); 
			}

			/**
			 * @brief 映像の高さを返す。
			 * @return 高さ。
			 */
			inline unsigned int GetHeight() const 
			{
				return decoder_.GetHeight(); 
			}

			/**
			 * @brief 映像が終端まで来ているかを返す。
			 * @return 終端なら true。
			 */
			inline bool IsEnd() const
			{
				return isEnd_;
			}


		private:
			MP4Decoder decoder_; //! ファイルから画素を取る。
			FrameTexture frameTexture_; //! 画素をテクスチャへ載せる。
			bool isOpened_ = false; //! 開けているか。
			bool isEnd_ = false; //! 終端まで読んだか。
			float time_ = 0.0f; //! 次のフレームまでの残り時間。
			long long frequency_ = 0; //! パフォーマンスカウンタの周波数。
			long long counter_ = 0; //! パフォーマンスカウンタの値。
			long long now_ = 0; //! パフォーマンスカウンタの現在値。
			float frameIndex_ = 0.0f; //! 現在のフレーム番号。
		};
	}
}