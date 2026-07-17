#pragma comment(lib, "DirectXTK12.lib")
#include "k2EngineLowPreCompile.h"
#include "Texture.h"
#include <filesystem>
#include <vector>   // TGAローダーのピクセルバッファ用
#include <cstdio>   // TGAファイル読み込み用（_wfopen_s等）
namespace fs = std::filesystem;

namespace nsK2EngineLow {
	Texture::Texture(const wchar_t* filePath)
	{
		LoadTextureFromFile(filePath);
	}
	Texture::~Texture()
	{
		Release();
	}
	void Texture::Release()
	{
		ReleaseD3D12Object(m_texture);
		// ★遅延解放キューへ渡したら所有権を手放す（二重解放防止）。
		//   これをしないと InitFromDDSFile → LoadTextureFromDDSFile のように
		//   Release() が連続で呼ばれた際に同じリソースが二重にキューへ積まれ、
		//   1フレーム後の解放処理で二重Release→アクセス違反でクラッシュする。
		//   （スプライトの再Init＝テクスチャ差し替えで顕在化するバグだった）
		m_texture = nullptr;
	}
	void Texture::InitFromDDSFile(const wchar_t* filePath)
	{
		Release();
		//DDSファイルからテクスチャをロード。
		LoadTextureFromDDSFile(filePath);

	}
	void Texture::InitFromD3DResource(ID3D12Resource* texture)
	{
		Release();
		m_texture = texture;
		m_texture->AddRef();
		m_textureDesc = m_texture->GetDesc();
	}
	void Texture::InitFromMemory(const char* memory, unsigned int size)
	{
		Release();
		//DDSファイルからテクスチャをロード。
		LoadTextureFromMemory(memory, size);

	}
	void Texture::LoadTextureFromMemory(const char* memory, unsigned int size
	)
	{
		Release();
		auto device = g_graphicsEngine->GetD3DDevice();
		DirectX::ResourceUploadBatch re(device);
		re.Begin();
		ID3D12Resource* texture;
		auto hr = DirectX::CreateDDSTextureFromMemoryEx(
			device,
			re,
			(const uint8_t*)memory,
			size,
			0,
			D3D12_RESOURCE_FLAG_NONE,
			0,
			&texture
		);
		re.End(g_graphicsEngine->GetCommandQueue());

		if (FAILED(hr)) {
			//テクスチャの作成に失敗しました。
			return;
		}

		m_texture = texture;
		m_textureDesc = m_texture->GetDesc();
	}
	void Texture::LoadTextureFromDDSFile(const wchar_t* filePath)
	{
		Release();
		auto device = g_graphicsEngine->GetD3DDevice();
		DirectX::ResourceUploadBatch re(device);
		re.Begin();
		ID3D12Resource* texture;
		auto hr = DirectX::CreateDDSTextureFromFileEx(
			device,
			re,
			filePath,
			0,
			D3D12_RESOURCE_FLAG_NONE,
			0,
			&texture,
			nullptr,
			&m_isCubemap
		);
		re.End(g_graphicsEngine->GetCommandQueue());

		if (FAILED(hr)) {
			//テクスチャの作成に失敗しました。
			return;
		}

		m_texture = texture;
		m_textureDesc = m_texture->GetDesc();
	}

	void Texture::RegistShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle, int bufferNo)
	{
		if (m_texture) {
			auto device = g_graphicsEngine->GetD3DDevice();
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = m_textureDesc.Format;
			if (m_isCubemap) {
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			}
			else {
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			}
			srvDesc.Texture2D.MipLevels = m_textureDesc.MipLevels;
			device->CreateShaderResourceView(m_texture, &srvDesc, descriptorHandle);
		}
	}
	// ================================================================
	// 簡易TGAローダー。
	// WIC(CreateWICTextureFromFile)はTGAに非対応で、DirectXTexも未導入のため、
	// MMDモデルで多い「非圧縮(type2)/RLE(type10) の 24/32bit トゥルーカラーTGA」を自前で読む。
	// 成功すると R8G8B8A8_UNORM のテクスチャを生成して outTexture に返す。
	// ================================================================
	static HRESULT LoadTGAToTexture(ID3D12Device* device,
		DirectX::ResourceUploadBatch& uploadBatch,
		const wchar_t* filePath,
		ID3D12Resource** outTexture)
	{
		*outTexture = nullptr;

		// ファイル全体をメモリへ
		FILE* fp = nullptr;
		if (_wfopen_s(&fp, filePath, L"rb") != 0 || !fp) return E_FAIL;
		fseek(fp, 0, SEEK_END);
		long fileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		if (fileSize < 18) { fclose(fp); return E_FAIL; }
		std::vector<uint8_t> data((size_t)fileSize);
		fread(data.data(), 1, (size_t)fileSize, fp);
		fclose(fp);

		// ヘッダ（18バイト）
		uint8_t  idLength   = data[0];
		uint8_t  imageType  = data[2];
		uint16_t width      = (uint16_t)(data[12] | (data[13] << 8));
		uint16_t height     = (uint16_t)(data[14] | (data[15] << 8));
		uint8_t  bpp        = data[16];
		uint8_t  descriptor = data[17];
		bool topLeft = (descriptor & 0x20) != 0; // bit5: 上原点（立っていなければ下原点で要反転）

		if (width == 0 || height == 0)            return E_FAIL;
		if (bpp != 24 && bpp != 32)               return E_FAIL; // 24/32bitのみ
		if (imageType != 2 && imageType != 10)    return E_FAIL; // 非圧縮/RLEトゥルーカラーのみ

		const int bpc = bpp / 8; // bytes per pixel
		size_t srcOffset = 18u + idLength; // カラーマップ無し前提
		if (srcOffset >= (size_t)fileSize)        return E_FAIL;

		const size_t pixelCount = (size_t)width * height;
		std::vector<uint8_t> rgba(pixelCount * 4);

		// TGAは BGR(A) 並び → RGBA へ詰め替える
		auto writePixel = [&](size_t idx, const uint8_t* s) {
			rgba[idx * 4 + 0] = s[2];                       // R
			rgba[idx * 4 + 1] = s[1];                       // G
			rgba[idx * 4 + 2] = s[0];                       // B
			rgba[idx * 4 + 3] = (bpc == 4) ? s[3] : 255;    // A
		};

		const uint8_t* src = data.data() + srcOffset;
		const uint8_t* end = data.data() + fileSize;

		if (imageType == 2) {
			// 非圧縮
			if (srcOffset + pixelCount * bpc > (size_t)fileSize) return E_FAIL;
			for (size_t i = 0; i < pixelCount; ++i) writePixel(i, src + i * bpc);
		}
		else {
			// RLE圧縮
			size_t i = 0;
			while (i < pixelCount && src < end) {
				uint8_t hdr = *src++;
				int count = (hdr & 0x7F) + 1;
				if (hdr & 0x80) {
					// RLEパケット：1ピクセルをcount回
					if (src + bpc > end) break;
					for (int k = 0; k < count && i < pixelCount; ++k, ++i) writePixel(i, src);
					src += bpc;
				}
				else {
					// Rawパケット：countピクセルそのまま
					for (int k = 0; k < count && i < pixelCount; ++k, ++i) {
						if (src + bpc > end) break;
						writePixel(i, src);
						src += bpc;
					}
				}
			}
		}

		// 下原点TGAは上下反転して、画像の上が先頭行になるようにする
		if (!topLeft) {
			const size_t rowBytes = (size_t)width * 4;
			std::vector<uint8_t> flipped(rgba.size());
			for (uint32_t y = 0; y < height; ++y)
				memcpy(&flipped[y * rowBytes], &rgba[(height - 1 - y) * rowBytes], rowBytes);
			rgba.swap(flipped);
		}

		// テクスチャ生成（R8G8B8A8）
		D3D12_RESOURCE_DESC td = {};
		td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		td.Width = width;
		td.Height = height;
		td.DepthOrArraySize = 1;
		td.MipLevels = 1;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc.Count = 1;
		td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_DEFAULT);
		HRESULT hr = device->CreateCommittedResource(
			&heap, D3D12_HEAP_FLAG_NONE, &td,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(outTexture));
		if (FAILED(hr)) return hr;

		D3D12_SUBRESOURCE_DATA sub = {};
		sub.pData = rgba.data();
		sub.RowPitch = (LONG_PTR)width * 4;
		sub.SlicePitch = (LONG_PTR)width * height * 4;

		// ResourceUploadBatchがデータを内部ステージングへコピーするので、rgbaはこの後解放されてOK
		uploadBatch.Upload(*outTexture, 0, &sub, 1);
		uploadBatch.Transition(*outTexture,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		return S_OK;
	}

	/// <summary>
	/// テクスチャファイルを読み込み、DirectXリソースとして初期化します。
	/// DDS形式のほか、PNG/JPG/BMP/TGAなどもサポートしています。
	/// </summary>
	/// <param name="filePath">読み込むテクスチャファイルのパス。ワイド文字列形式で指定します。</param>
	void Texture::LoadTextureFromFile(const wchar_t* filePath)
	{
		Release();
		auto device = g_graphicsEngine->GetD3DDevice();
		DirectX::ResourceUploadBatch re(device);
		re.Begin();

		ID3D12Resource* texture = nullptr;
		bool isCube = false;

		// 拡張子で分岐
		std::wstring ext = fs::path(filePath).extension().wstring();
		HRESULT hr = E_FAIL;

		if (_wcsicmp(ext.c_str(), L".dds") == 0)
		{
			hr = DirectX::CreateDDSTextureFromFileEx(
				device,
				re,
				filePath,
				0,
				D3D12_RESOURCE_FLAG_NONE,
				0,
				&texture,
				nullptr,
				&isCube
			);
		}
		else if (_wcsicmp(ext.c_str(), L".tga") == 0)
		{
			// TGAはWIC非対応なので自前ローダーで読む（MMDモデル向け）
			hr = LoadTGAToTexture(device, re, filePath, &texture);
		}
		else
		{
			// PNG / JPG / BMP などはこちら（WIC）
			hr = DirectX::CreateWICTextureFromFileEx(
				device,
				re,
				filePath,
				0,
				D3D12_RESOURCE_FLAG_NONE,
				DirectX::WIC_LOADER_DEFAULT,
				&texture
			);
		}

		auto fut = re.End(g_graphicsEngine->GetCommandQueue());
		fut.wait(); // GPU転送完了を待つ

		if (FAILED(hr) || texture == nullptr)
		{
			wchar_t msg[512];
			swprintf_s(msg, L"[Texture] Failed to load texture: %s\n", filePath);
			OutputDebugStringW(msg);
			return;
		}

		m_texture = texture;
		m_textureDesc = m_texture->GetDesc();
		m_isCubemap = isCube;

		DebugPrint("[Texture] Loaded OK: %s (fmt=%d, mips=%d)\n",
			filePath, m_textureDesc.Format, m_textureDesc.MipLevels);

	}
}