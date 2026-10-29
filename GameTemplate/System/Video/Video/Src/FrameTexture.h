#pragma once
#include "util/Noncopyable.h"
#include <d3d12.h>
#include "graphics/IShaderResource.h"
#include "graphics/Texture.h"

namespace nsApp
{
	namespace nsVideo
	{
		/**
		 * @file FrameTexture.h
		 * @brief 1フレームの画素を、描画用テクスチャへ載せる。
		 * @details MP4Decoder が取り出した画素を受け取り、GPU上のテクスチャへコピーする。
		 *          ファイルを開く処理と、スプライトで描く処理は持たない。
		 * @author Yamaguchi Hayato
		 * @date 2026/10/22
		 */
		class FrameTexture
		{
		public:
			/* デストラクタ。*/
			~FrameTexture()
			{
				if (pUpload_ != nullptr)
				{
					pUpload_->Release();
					pUpload_ = nullptr;
				}
			}

			/**
			 * @brief 幅と高さのテクスチャを1枚作る。
			 * @param pDevice Direct3Dのデバイス。
			 * @param width テクスチャの幅。
			 * @param height テクスチャの高さ。
			 * @return 作れたら true。失敗したら false。
			 */
			bool Create(ID3D12Device* pDevice, unsigned int width, unsigned int height);

			/**
			 * @brief 画素をテクスチャへコピーする。
			 * @param pCommandList コピー命令を積むコマンドリスト。
			 * @param pPixels 画素の先頭。1ピクセル4バイト。
			 * @param size 画素のバイト数。
			 * @return コピーできたら true。失敗したら false。
			 */
			bool Copy(ID3D12GraphicsCommandList* pCommandList, const unsigned char* pPixels, unsigned int size);

			/**
			 * @brief スプライトへ渡すテクスチャを返す。
			 * @return 描画用テクスチャ。
			 */
			inline nsK2EngineLow::Texture* GetTexture() 
			{
				return &texture_; 
			}


		private:
			/**
			 * @brief 描画先のテクスチャを作る。
			 * @param pDevice Direct3Dのデバイス。
			 * @return 作れたら true。失敗したら false。
			 */
			bool CreateTexture(ID3D12Device* pDevice);

			/**
			 * @brief 画素の転送元バッファを作る。
			 * @param pDevice Direct3Dのデバイス。
			 * @return 作れたら true。失敗したら false。
			 */
			bool CreateUpload(ID3D12Device* pDevice);

			/**
			 * @brief 画素を転送元バッファへ行ごとに書く。
			 * @param pPixels 画素の先頭。
			 * @return 書けたら true。失敗したら false。
			 */
			bool WriteRows(const unsigned char* pPixels);

			/**
			 * @brief 転送元バッファからテクスチャへコピーする。
			 * @param pCommandList コピー命令を積むコマンドリスト。
			 */
			void CopyToGpu(ID3D12GraphicsCommandList* pCommandList);

			/**
			 * @brief テクスチャの状態を切り替える。
			 * @param pCommandList 命令を積むコマンドリスト。
			 * @param before 切り替え前の状態。
			 * @param after 切り替え後の状態。
			 */
			void Barrier(ID3D12GraphicsCommandList* pCommandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);


		private:
			nsK2EngineLow::Texture texture_; //! スプライトへ渡すテクスチャ。
			ID3D12Resource* pResource_ = nullptr; //! 作成直後のリソース。
			ID3D12Resource* pUpload_ = nullptr; //! 画素の転送元。
			void* pMapped_ = nullptr; //! 転送元の書き込み先。
			D3D12_RESOURCE_DESC desc_ = {}; //! テクスチャの情報。
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint_ = {}; //! 転送の並び。
			D3D12_RESOURCE_BARRIER barrier_ = {}; //! 状態の切り替え。
			D3D12_TEXTURE_COPY_LOCATION copyDst_ = {}; //! コピー先。
			D3D12_TEXTURE_COPY_LOCATION copySrc_ = {}; //! コピー元。
			D3D12_HEAP_PROPERTIES heap_ = {}; //! 転送元のヒープ。
			D3D12_RESOURCE_DESC buffer_ = {}; //! 転送元のバッファ情報。
			D3D12_RANGE range_ = {};//! 転送元の書き込み範囲。
			unsigned int width_ = 0; //! 幅。
			unsigned int height_ = 0; //! 高さ。
			unsigned int index_ = 0; //! 行の位置。
			unsigned long long uploadSize_ = 0; //! 転送バッファのバイト数。
			long result_ = 0; //! Direct3D の戻り値。
			bool isShaderResource_ = false; //! 今、描画できる状態か。
			unsigned char* pDst_ = nullptr;
		};
	}
}