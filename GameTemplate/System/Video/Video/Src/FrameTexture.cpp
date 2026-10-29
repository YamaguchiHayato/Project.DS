#include "pch.h"
#include "FrameTexture.h"
#include <cstring>

namespace nsApp
{
	namespace nsVideo
	{
		bool FrameTexture::Create(ID3D12Device* pDevice, unsigned int width, unsigned int height)
		{
			/* 高さと幅を残し、Textureを作成する。*/
			width_ = width;
			height_ = height;	

			/* 描画用のテクスチャを作る。*/
			if (!CreateTexture(pDevice))
				return false;

			/* 転送元のバッファを作る。*/
			return CreateUpload(pDevice);
		}


		bool FrameTexture::CreateTexture(ID3D12Device* pDevice)
		{
			/* 描画用の2Dテクスチャを定義する。*/
			desc_.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			desc_.Width = width_;
			desc_.Height = height_;
			desc_.DepthOrArraySize = 1;
			desc_.MipLevels = 1;
			desc_.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			desc_.SampleDesc.Count = 1;
			desc_.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

			/* GPU上にTテクスチャを作成する。*/
			heap_.Type = D3D12_HEAP_TYPE_DEFAULT;
			/* GPU上にテクスチャを作る。*/
			result_ = pDevice->CreateCommittedResource(&heap_,D3D12_HEAP_FLAG_NONE,&desc_,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&pResource_));

			/* 作成できなかったら false を返す。*/
			if(FAILED(result_))
				return false;

			/* 作成できたら、Textureに登録する。*/
			texture_.InitFromD3DResource(pResource_);
			pResource_->Release();
			pResource_ = nullptr;
			desc_ = texture_.GetD3DResource()->GetDesc();
			
			return true;
		}


		bool FrameTexture::CreateUpload(ID3D12Device* pDevice)
		{
			/* GPUが受け取るたびに、転送先に必要なバイト数を得る。*/
			pDevice->GetCopyableFootprints(&desc_, 0, 1, 0, &footprint_, nullptr, nullptr, &uploadSize_);

			/* CPUから書き込める転送先のバッファーを作成する。*/
			heap_.Type = D3D12_HEAP_TYPE_UPLOAD;
			buffer_.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			buffer_.Width = uploadSize_;
			buffer_.Height = 1;
			buffer_.DepthOrArraySize = 1;
			buffer_.MipLevels = 1;
			buffer_.SampleDesc.Count = 1;
			buffer_.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			result_ = pDevice->CreateCommittedResource(&heap_, D3D12_HEAP_FLAG_NONE, &buffer_, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pUpload_));

			/* 作成できなかったら false を返す。*/
			return SUCCEEDED(result_);
		}


		bool FrameTexture::Copy(ID3D12GraphicsCommandList* pCommandList, const unsigned char* pPixels, unsigned int size)
		{
			/* 1フレーム分の画素が無ければコピーしない。*/
			if (pPixels == nullptr || size < width_ * height_ * 4)
				return false;

			/* 描画に使ったあとのテクスチャは、コピーを受け取る状態へ戻す。*/
			if (isShaderResource_)
				Barrier(pCommandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

			/* 転送元のバッファへ画素を書き込む。*/
			if (!WriteRows(pPixels))
				return false;

			/* GPUへコピーし、スプライトから読める状態へ戻す。*/
			CopyToGpu(pCommandList);
			Barrier(pCommandList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			isShaderResource_ = true;

			return true;
		}


		bool FrameTexture::WriteRows(const unsigned char* pPixels)
		{
			/* 転送元をCPUから書き込めるようにする。*/
			result_ = pUpload_->Map(0, &range_, &pMapped_);
			pDst_ = static_cast<unsigned char*>(pMapped_);
			if (FAILED(result_))
				return false;

			/* 行の並びが幅そのものとは限らないので、1行ずつ書く。*/
			pDst_ += footprint_.Offset;
			for (index_ = 0; index_ < height_; index_++)
				memcpy(pDst_ + index_ * footprint_.Footprint.RowPitch, pPixels + index_ * width_ * 4,width_ * 4);

			/* 書き込みを終える。*/
			pUpload_->Unmap(0, nullptr);
			pMapped_ = nullptr;
			return true;
		}


		void FrameTexture::CopyToGpu(ID3D12GraphicsCommandList* pCommandList)
		{
			/* コピー先はテクスチャ、コピー元は転送バッファ。*/
			copyDst_.pResource = texture_.Get();
			copyDst_.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			copyDst_.SubresourceIndex = 0;
			copySrc_.pResource = pUpload_;
			copySrc_.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			copySrc_.PlacedFootprint = footprint_;
			pCommandList->CopyTextureRegion(&copyDst_, 0, 0, 0, &copySrc_, nullptr);
		}


		void FrameTexture::Barrier(ID3D12GraphicsCommandList* pCommandList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
		{
			/* テクスチャを、コピーできる状態と描画できる状態のあいだで切り替える。*/
			barrier_.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier_.Transition.pResource = texture_.Get();
			barrier_.Transition.StateBefore = before;
			barrier_.Transition.StateAfter = after;
			barrier_.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			pCommandList->ResourceBarrier(1, &barrier_);
		}
	}
}