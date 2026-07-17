#define NOMINMAX
#include "k2EngineLowPreCompile.h"
#include "PenlightRender.h"
#include "graphics/VertexBuffer.h"
#include "graphics/IndexBuffer.h"
#include "graphics/ConstantBuffer.h"
#include "graphics/StructuredBuffer.h"
#include "graphics/RootSignature.h"
#include "graphics/PipelineState.h"

using namespace nsK2EngineLow;
using namespace DirectX;

template<class T> static void SafeDelP(T*& p) { if (p) { delete p; p = nullptr; } }

PenlightRender::~PenlightRender()
{
    Destroy();
}

bool PenlightRender::Init(const std::vector<Instance>& instances, DXGI_FORMAT rtvFmt, DXGI_FORMAT dsvFmt)
{
    if (instances.empty()) return false;
    m_instanceCount = (int)instances.size();

    // ルートシグネチャ（標準：CBVテーブル＋SRVテーブル＋サンプラ）
    m_rootSig = new RootSignature();
    if (!m_rootSig->Init(D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP)) {
        OutputDebugStringA("[PenlightRender] RootSignature init failed.\n");
        return false;
    }

    if (!BuildPipelineState(rtvFmt, dsvFmt)) return false;

    // クアッド（4頂点・6インデックス）。localPos x:-0.5..0.5, y:0(根元)..1(先端)
    QuadVertex verts[4] = {
        { {-0.5f, 0.0f}, {0.0f, 0.0f} },
        { { 0.5f, 0.0f}, {1.0f, 0.0f} },
        { { 0.5f, 1.0f}, {1.0f, 1.0f} },
        { {-0.5f, 1.0f}, {0.0f, 1.0f} },
    };
    uint16_t indices[6] = { 0, 2, 1, 0, 3, 2 };

    m_vb = new VertexBuffer();
    m_vb->Init(sizeof(verts), sizeof(QuadVertex));
    m_vb->Copy(verts);

    m_ib = new IndexBuffer();
    m_ib->Init(sizeof(indices), sizeof(uint16_t));
    m_ib->Copy(indices);

    // グローバル定数バッファ
    m_cb = new ConstantBuffer();
    m_cb->Init(sizeof(GlobalCB));

    // インスタンスバッファ（位置・位相・色）
    m_instanceSB = std::make_unique<StructuredBuffer>();
    m_instanceSB->Init(sizeof(Instance), m_instanceCount, (void*)instances.data());

    // ディスクリプタヒープ（b0：グローバル、t0：インスタンス）
    m_descHeap.ResizeConstantBuffer(1);
    m_descHeap.ResizeShaderResource(1);
    m_descHeap.RegistConstantBuffer(0, *m_cb);
    m_descHeap.RegistShaderResource(0, *m_instanceSB);
    m_descHeap.Commit();

    return true;
}

bool PenlightRender::BuildPipelineState(DXGI_FORMAT rtvFmt, DXGI_FORMAT dsvFmt)
{
    static D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    auto loadCso = [](const wchar_t* path) {
        struct R { std::vector<uint8_t> data; D3D12_SHADER_BYTECODE bc{}; } r;
        FILE* fp = nullptr;
        if (_wfopen_s(&fp, path, L"rb") != 0 || !fp) return r;
        fseek(fp, 0, SEEK_END);
        long sz = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        r.data.resize(sz);
        fread(r.data.data(), 1, sz, fp);
        fclose(fp);
        r.bc = { r.data.data(), (SIZE_T)sz };
        return r;
        };

#ifdef _DEBUG
    auto vs = loadCso(L"x64/Debug/Penlight_VS.cso");
    auto ps = loadCso(L"x64/Debug/Penlight_PS.cso");
#else
    auto vs = loadCso(L"x64/Release/Penlight_VS.cso");
    auto ps = loadCso(L"x64/Release/Penlight_PS.cso");
#endif
    if (!vs.bc.pShaderBytecode || !ps.bc.pShaderBytecode) {
        OutputDebugStringA("[PenlightRender] Failed to load shader CSOs.\n");
        return false;
    }

    // 加算合成（光を足し込む）
    D3D12_BLEND_DESC blend = {};
    blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    auto& rt = blend.RenderTarget[0];
    rt.BlendEnable = TRUE;
    rt.SrcBlend = D3D12_BLEND_ONE;
    rt.DestBlend = D3D12_BLEND_ONE;
    rt.BlendOp = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlendAlpha = D3D12_BLEND_ONE;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[1].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_RASTERIZER_DESC rast = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rast.CullMode = D3D12_CULL_MODE_NONE;

    // 半透明なので深度テストON・深度書き込みOFF
    D3D12_DEPTH_STENCIL_DESC dss = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    dss.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = m_rootSig->Get();
    desc.VS = vs.bc;
    desc.PS = ps.bc;
    desc.InputLayout = { layout, _countof(layout) };
    desc.BlendState = blend;
    desc.RasterizerState = rast;
    desc.DepthStencilState = dss;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 2; // メインRTはMRT（色＋速度）
    desc.RTVFormats[0] = rtvFmt;
    desc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.DSVFormat = dsvFmt;
    desc.SampleMask = UINT_MAX;
    desc.SampleDesc.Count = 1;

    m_pso = new PipelineState();
    m_pso->Init(desc);
    if (!m_pso->Get()) {
        OutputDebugStringA("[PenlightRender] PipelineState build failed.\n");
        SafeDelP(m_pso);
        return false;
    }
    return true;
}

void PenlightRender::Draw(RenderContext& rc)
{
    if (!IsValid()) return;

    // グローバル定数を更新（viewProj はPMXと同じく転置して渡す）
    GlobalCB cb{};
    XMStoreFloat4x4(&cb.viewProj, XMMatrixTranspose(g_camera3D->GetViewProjectionMatrix()));

    // ビルボード横方向：right = worldUp × forward
    Vector3 camPos = g_camera3D->GetPosition();
    Vector3 camTgt = g_camera3D->GetTarget();
    Vector3 fwd = camTgt - camPos;
    fwd.Normalize();
    Vector3 right = Cross(Vector3(0.0f, 1.0f, 0.0f), fwd);
    right.Normalize();
    cb.camRight = XMFLOAT4(right.x, right.y, right.z, 0.0f);
    cb.timeParams = XMFLOAT4(m_time, 0.0f, 0.0f, 0.0f);
    m_cb->CopyToVRAM(&cb);

    rc.SetRootSignature(*m_rootSig);
    rc.SetPipelineState(*m_pso);
    rc.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    rc.SetVertexBuffer(*m_vb);
    rc.SetIndexBuffer(*m_ib);
    rc.SetDescriptorHeap(m_descHeap);
    rc.DrawIndexedInstanced(6, m_instanceCount);
}

void PenlightRender::Destroy()
{
    SafeDelP(m_vb);
    SafeDelP(m_ib);
    SafeDelP(m_cb);
    SafeDelP(m_pso);
    SafeDelP(m_rootSig);
    m_instanceSB.reset();
    m_instanceCount = 0;
}
