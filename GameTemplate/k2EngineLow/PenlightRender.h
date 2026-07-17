#pragma once
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include "Graphics/RenderContext.h"
#include "Graphics/DescriptorHeap.h"

namespace nsK2EngineLow {

    class VertexBuffer;
    class IndexBuffer;
    class ConstantBuffer;
    class StructuredBuffer;
    class RootSignature;
    class PipelineState;

    // 観客のペンライトをGPUインスタンシングで多数描画するクラス。
    // クアッド1枚を共有し、インスタンスバッファ（位置・色・位相）を SV_InstanceID で参照する。
    class PenlightRender
    {
    public:
        // 1本分のインスタンスデータ（シェーダーの PenlightInstance と一致させること）
        struct Instance
        {
            DirectX::XMFLOAT3 position; // 根元のワールド座標
            float             phase;    // 揺れの位相オフセット
            DirectX::XMFLOAT4 color;    // rgb=色, a=強さ
        };

        PenlightRender() = default;
        ~PenlightRender(); // unique_ptr<StructuredBuffer>(前方宣言)のため定義は.cpp側

        // インスタンス群を渡して初期化（rtvFmt/dsvFmt はメインRTに合わせる）
        bool Init(const std::vector<Instance>& instances, DXGI_FORMAT rtvFmt, DXGI_FORMAT dsvFmt);

        // 揺れアニメ用に時間を進める
        void Update(float deltaTime) { m_time += deltaTime; }

        // 3Dパス（PMX描画の後・ブルーム前）で呼ぶ
        void Draw(RenderContext& rc);

        void Destroy();

        bool IsValid() const { return m_instanceCount > 0 && m_pso != nullptr; }

    private:
        bool BuildPipelineState(DXGI_FORMAT rtvFmt, DXGI_FORMAT dsvFmt);

        // b0 グローバル定数（シェーダーの PenlightGlobal と一致）
        struct GlobalCB
        {
            DirectX::XMFLOAT4X4 viewProj;
            DirectX::XMFLOAT4   camRight;   // xyz: カメラ右ベクトル
            DirectX::XMFLOAT4   timeParams; // x: 時間
        };

        struct QuadVertex { float pos[2]; float uv[2]; };

        RootSignature* m_rootSig = nullptr;
        PipelineState* m_pso = nullptr;
        VertexBuffer* m_vb = nullptr;
        IndexBuffer* m_ib = nullptr;
        ConstantBuffer* m_cb = nullptr;
        std::unique_ptr<StructuredBuffer> m_instanceSB;
        DescriptorHeap m_descHeap;

        int   m_instanceCount = 0;
        float m_time = 0.0f;
    };
}
