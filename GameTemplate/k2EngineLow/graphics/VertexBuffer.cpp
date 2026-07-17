#include "k2EngineLowPreCompile.h"
#include "VertexBuffer.h"

namespace nsK2EngineLow {

	VertexBuffer::~VertexBuffer()
	{
		Release();
	}
	void VertexBuffer::Release()
	{
		ReleaseD3D12Object(m_vertexBuffer);
		m_vertexBuffer = nullptr; // ★キューへ渡したら手放す（二重解放防止。Texture.cpp参照）
	}
	void VertexBuffer::Init(int size, int stride)
	{
		Release();
		auto d3dDevice = g_graphicsEngine->GetD3DDevice();
		auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto rDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
		d3dDevice->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&rDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer));

		m_vertexBuffer->SetName(L"VertexBuffer");
		//頂点バッファのビューを作成。
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.SizeInBytes = size;
		m_vertexBufferView.StrideInBytes = stride;
	}
	void VertexBuffer::Copy(void* srcVertices)
	{
		uint8_t* pData;
		m_vertexBuffer->Map(0, nullptr, (void**)&pData);
		memcpy(pData, srcVertices, m_vertexBufferView.SizeInBytes);
		m_vertexBuffer->Unmap(0, nullptr);
	}
	void VertexBuffer::CopyRange(const void* src, unsigned int offsetBytes, unsigned int sizeBytes)
	{
		if (m_vertexBuffer == nullptr || src == nullptr || sizeBytes == 0) {
			return;
		}
		// 範囲がバッファを超えないようクランプする
		if (offsetBytes >= m_vertexBufferView.SizeInBytes) {
			return;
		}
		if (offsetBytes + sizeBytes > m_vertexBufferView.SizeInBytes) {
			sizeBytes = m_vertexBufferView.SizeInBytes - offsetBytes;
		}
		uint8_t* pData;
		m_vertexBuffer->Map(0, nullptr, (void**)&pData);
		memcpy(pData + offsetBytes, src, sizeBytes);
		m_vertexBuffer->Unmap(0, nullptr);
	}
}