// [RHYTHM_GAME_MOD] 
#include "k2EngineLowPreCompile.h"
#include "VertexBufferVRAM.h"

namespace nsK2EngineLow {

    VertexBufferVRAM::~VertexBufferVRAM()
	{
		Release();
	}

	void VertexBufferVRAM::Release()
	{
		ReleaseD3D12Object(m_vertexBuffer);
	}

	void VertexBufferVRAM::Init(void* srcVertices, int size, int stride)
	{
        Release();
        auto device = g_graphicsEngine->GetD3DDevice();
        auto cmdQueue = g_graphicsEngine->GetCommandQueue(); // ←エンジンに合わせて変更

        // ---- 1. DEFAULTヒープ（本番VRAM）バッファ作成 ----
        auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto rDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
        device->CreateCommittedResource(
            &defaultHeap, D3D12_HEAP_FLAG_NONE, &rDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, IID_PPV_ARGS(&m_vertexBuffer));
        m_vertexBuffer->SetName(L"VertexBuffer_VRAM_Default");

        // ---- 2. UPLOADヒープ（転送用一時バッファ）作成 ----
        // ★ローカル変数にする（Init完了後に自動解放）
        ID3D12Resource* uploadBuffer = nullptr;
        auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        device->CreateCommittedResource(
            &uploadHeap, D3D12_HEAP_FLAG_NONE, &rDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&uploadBuffer));

        // ---- 3. CPUからUPLOADへ書き込み ----
        uint8_t* pData;
        uploadBuffer->Map(0, nullptr, (void**)&pData);
        memcpy(pData, srcVertices, size);
        uploadBuffer->Unmap(0, nullptr);

        // ---- 4. 専用コマンドリストを作ってコピー命令を記録 ----
        ComPtr<ID3D12CommandAllocator> allocator;
        device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));

        ComPtr<ID3D12GraphicsCommandList> cmdList;
        device->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            allocator.Get(), nullptr, IID_PPV_ARGS(&cmdList));

        cmdList->CopyBufferRegion(m_vertexBuffer, 0, uploadBuffer, 0, size);

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_vertexBuffer,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        cmdList->ResourceBarrier(1, &barrier);
        cmdList->Close();

        // ---- 5. GPUに実行させる ----
        ID3D12CommandList* lists[] = { cmdList.Get() };
        cmdQueue->ExecuteCommandLists(1, lists);

        // ---- 6. フェンスでGPU完了を確実に待つ ----
        // ★ここが今まで抜けていた！これがないと「たまに消える」が起きる
        ComPtr<ID3D12Fence> fence;
        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        cmdQueue->Signal(fence.Get(), 1);

        HANDLE waitEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        fence->SetEventOnCompletion(1, waitEvent);
        WaitForSingleObject(waitEvent, INFINITE); // GPU完了まで待機
        CloseHandle(waitEvent);

        // ---- 7. 転送完了 → UPLOADバッファは不要なので即解放 ----
        ReleaseD3D12Object(uploadBuffer);

        // ---- 8. ビュー作成 ----
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.SizeInBytes = size;
        m_vertexBufferView.StrideInBytes = stride;
    }
}