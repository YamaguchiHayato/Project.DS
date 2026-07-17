//背景ステージなどの静的モデルをVRAMに完全配置して爆速化するためのクラス
#pragma once
namespace nsK2EngineLow {

    class VertexBufferVRAM {
    public:
        VertexBufferVRAM() = default;
        ~VertexBufferVRAM();

        void Release();

        // ★変更: cmdList引数を削除。内部で完結させる
        void Init(void* srcVertices, int size, int stride);

        ID3D12Resource* Get() const { return m_vertexBuffer; }
        const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return m_vertexBufferView; }
        operator ID3D12Resource* () const { return m_vertexBuffer; }

    private:
        ID3D12Resource* m_vertexBuffer = nullptr;
        // ★変更: m_uploadBufferはInit内で使い捨て → メンバ不要
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    };
}