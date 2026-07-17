#define NOMINMAX
#include <algorithm>
#include "k2EngineLowPreCompile.h"
#include "PMXRender.h"
#include "VMDAnimPlayer.h"

#include <unordered_set>
#include <fstream>
#include <cwctype>
using namespace nsK2EngineLow;
using namespace DirectX;

template<class T> static void SafeDelete(T*& p) { if (p) { delete p; p = nullptr; } }

std::unique_ptr<Texture> PMXRender::s_fallbackAlbedo;

// 全PMXRenderで共有する静的リソース（最初の1体生成時に作って以降使い回す）
RootSignature* PMXRender::s_rootSig = nullptr;
PipelineState* PMXRender::s_pso = nullptr;
PipelineState* PMXRender::s_psoAdd = nullptr;
std::unordered_map<std::wstring, Texture*> PMXRender::s_textureCache;

// ★テクスチャ先読みバッファ（ワーカースレッド書き込み/メインスレッド消費）
std::unordered_map<std::wstring, std::vector<char>> PMXRender::s_textureBlobCache;
std::mutex PMXRender::s_textureBlobMutex;

// テクスチャパスのUTF-8→ワイド変換（makeTexとPreloadTextureBlobsで共用）
static std::wstring TexPathToWide(const std::string& path)
{
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring w;
    if (wlen > 0) {
        w.resize(wlen);
        MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &w[0], wlen);
        if (!w.empty() && w.back() == L'\0') w.pop_back();
    }
    else {
        w = std::wstring(path.begin(), path.end()); // 変換失敗時の保険
    }
    return w;
}

// 拡張子が .dds か（大文字小文字を無視）
static bool IsDDSPath(const std::wstring& w)
{
    if (w.size() < 4) return false;
    std::wstring ext = w.substr(w.size() - 4);
    for (auto& c : ext) c = (wchar_t)std::towlower(c);
    return ext == L".dds";
}

//================================================================
// ★テクスチャの先読み（どのスレッドから呼んでもよい。ファイルI/Oのみ）
//================================================================
void PMXRender::PreloadTextureBlobs(const PMXModel& model)
{
    auto preloadOne = [](const std::string& path) {
        if (path.empty()) return;
        std::wstring w = TexPathToWide(path);
        // InitFromMemory はDDS専用なので、.dds 以外は先読みしない（従来どおりファイルから読む）
        if (!IsDDSPath(w)) return;

        // 既に先読み済みならスキップ
        {
            std::lock_guard<std::mutex> lk(s_textureBlobMutex);
            if (s_textureBlobCache.find(w) != s_textureBlobCache.end()) return;
        }

        // ファイル読み込みはロックの外で行う（他スレッドを待たせない）
        std::ifstream ifs(w.c_str(), std::ios::binary | std::ios::ate);
        if (!ifs.is_open()) return;
        const std::streamsize size = ifs.tellg();
        if (size <= 0) return;
        std::vector<char> data((size_t)size);
        ifs.seekg(0, std::ios::beg);
        if (!ifs.read(data.data(), size)) return;

        std::lock_guard<std::mutex> lk(s_textureBlobMutex);
        s_textureBlobCache.emplace(std::move(w), std::move(data));
    };

    for (const auto& mat : model.materials) {
        preloadOne(mat.textureFile);
        preloadOne(mat.sphereTextureFile);
        preloadOne(mat.toonTextureFile);
    }
}

void PMXRender::ClearTextureBlobCache()
{
    std::lock_guard<std::mutex> lk(s_textureBlobMutex);
    s_textureBlobCache.clear();
}

//================================================================
// 初期化
//================================================================
bool PMXRender::Init(const PMXModel& model, DXGI_FORMAT rtvFmt, DXGI_FORMAT dsvFmt)
{
    EnsureFallbackTextures();

    m_model = &model;
    m_validModel = false; // 有効に初期化できたら最後に true にする

    if (model.vertices.empty() || model.indices.empty()) {
        // モデルが空（PMXパスが間違っている等で読めなかった）。
        // ここで描画対象にすると 0サイズのボーンバッファ生成でクラッシュするので、無効のまま返す。
        OutputDebugStringA("[PMXRender] Init failed: empty vertices or indices.\n");
        return false;
    }

    if (!BuildRootSignature())                     return false;
    if (!BuildPipelineState(rtvFmt, dsvFmt))       return false;
    if (!BuildBuffers(model))                      return false;
    //if (!BuildTextures(model))                     return false;

    //if (!m_physics) {
    //    m_physics = std::make_unique<PMXPhysics>();
    //    m_physics->Init(*m_model);
    //}

    // ★追加: 分割ロードの準備
    m_matGpu.clear();
    m_matGpu.reserve(model.materials.size()); // 枠だけ確保
    m_currentTextureIndex = 0;
    m_isLoading = true; // ロード開始！

   /* char buf[128];
    sprintf_s(buf, "[PMXRender] Init OK. bones=%zu, verts=%zu, indices=%zu\n",
        model.bones.size(), model.vertices.size(), model.indices.size());
    OutputDebugStringA(buf);*/
    DebugPrint("[PMXRender] Init OK. bones=%zu, verts=%zu, indices=%zu\n",
        model.bones.size(), model.vertices.size(), model.indices.size());

    m_isFirstFrame = true;
    m_validModel = true; // 正常に初期化できた → 描画OK

    return true;
}

// PMXRender.cpp に追加

//================================================================
// 分割ロード用更新関数 (1フレームに1回呼ぶ)
// 戻り値: true = ロード継続中 / false = ロード完了
//================================================================
bool PMXRender::UpdateLoader()
{
    // ロード中じゃなければ即終了
    if (!m_isLoading) return false;

    // モデルデータがない場合はエラーとして終了
    if (!m_model) {
        m_isLoading = false;
        return false;
    }

    // -------------------------------------------------------
    // 1. まだ読み込んでいないマテリアルがある場合 (継続中)
    // -------------------------------------------------------
    if (m_currentTextureIndex < m_model->materials.size())
    {
        // 今回処理するマテリアルデータ
        const auto& src = m_model->materials[m_currentTextureIndex];

        auto* device = g_graphicsEngine->GetD3DDevice();

        // テクスチャ作成用ヘルパーラムダ
        auto makeTex = [&](const std::string& path)->Texture*
            {
                if (path.empty()) return nullptr;

                // UTF-8パス→ワイド文字（Windows APIで安全に変換）
                std::wstring w = TexPathToWide(path);

                // ★キャッシュヒット：同じパスのテクスチャは1回だけロードして使い回す
                //   （9体同一モデルなら全テクスチャを9回ロードしていたのを1回に削減）
                auto cacheIt = s_textureCache.find(w);
                if (cacheIt != s_textureCache.end()) {
                    return cacheIt->second;
                }

                // ★先読みブロブがあれば「メモリ→GPU転送」だけで済ませる。
                //   （ワーカースレッドがPreloadTextureBlobsでディスクから読んでおいたもの。
                //     メインスレッドのディスクI/Oが消えるのでロード画面が止まらない）
                std::vector<char> blob;
                {
                    std::lock_guard<std::mutex> lk(s_textureBlobMutex);
                    auto bit = s_textureBlobCache.find(w);
                    if (bit != s_textureBlobCache.end()) {
                        blob = std::move(bit->second);
                        s_textureBlobCache.erase(bit);
                    }
                }

                Texture* t = nullptr;
                if (!blob.empty()) {
                    t = new Texture();
                    t->InitFromMemory(blob.data(), (unsigned int)blob.size());
                }
                if (!t || !t->IsValid()) {
                    // 先読みが無い／メモリロード失敗時は従来どおりファイルから読む
                    SafeDelete(t);
                    t = new Texture(w.c_str());
                }
                s_textureCache[w] = t; // 以降の同一パス要求で使い回す

                D3D12_DESCRIPTOR_HEAP_DESC hd{};
                hd.NumDescriptors = 1;
                hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                ComPtr<ID3D12DescriptorHeap> heap;
                device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&heap));
                auto cpu = heap->GetCPUDescriptorHandleForHeapStart();
                t->RegistShaderResourceView(cpu, 0);
                return t;
            };

        // GPU用マテリアルデータの構築
        MatGPU mg{};

        // 各種テクスチャ読み込み
        mg.albedo = makeTex(src.textureFile);
        mg.sphere = makeTex(src.sphereTextureFile);
        mg.toon = makeTex(src.toonTextureFile);

        // ★追加：マテリアル専用の定数バッファを作成
        mg.psCB = new ConstantBuffer();
        mg.psCB->Init(sizeof(PSMaterial));

        // ★追加：このマテリアル専用のディスクリプタヒープを事前構築
        mg.descHeap = new DescriptorHeap();
        mg.descHeap->ResizeConstantBuffer(2); // b0, b1
        mg.descHeap->ResizeShaderResource(2); // t0:albedo, t1:bone SB

        Texture* albedoTex = mg.albedo ? mg.albedo : s_fallbackAlbedo.get();
        mg.descHeap->RegistConstantBuffer(0, *m_vsCB);
        mg.descHeap->RegistConstantBuffer(1, *(mg.psCB));
        mg.descHeap->RegistShaderResource(0, *albedoTex);
        mg.descHeap->RegistShaderResource(1, *m_boneMatrixSB);
        mg.descHeap->Commit(); // ★一生に一回だけCommit！激重処理をロード中に終わらせる

        // ★★★ STEP2: 強制点灯ハックを解除し、初期状態にする ★★★

           // MMDモデルは diffuse を低め＋ambient を高めに設定し、
           // 「diffuse + ambient」で本来の明るさになるものが多い（ambient頼みのモデルはdiffuseだけだと暗い）。
           // このシェーダーはライティングが無い(tex*diffuse)ので、ambientを足し込んで明るさを合わせる。
        auto clamp01 = [](float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); };
        mg.psData.diffuse = {
            clamp01(src.diffuse[0] + src.ambient[0]),
            clamp01(src.diffuse[1] + src.ambient[1]),
            clamp01(src.diffuse[2] + src.ambient[2]),
            src.diffuse[3] // アルファはdiffuseのまま
        };

        // 初期の発光設定
        float emissionIntensity = 0.0f;
        if (src.name.find("Screen") != std::string::npos) {
            emissionIntensity = 0.1f; // スクリーンは常時点灯
        }
        else if (src.name.find("HALO") != std::string::npos) {
            emissionIntensity = 0.5f; // モヤも常時点灯
        }
        // ※LightやSpot等は、VMDの音楽に合わせて動的に光るので 0.0f (消灯) のままにしておく！

        mg.psData.filterParams.w = emissionIntensity;

        // インデックス開始位置の計算
        // 分割ロードなので「前のマテリアルの続き」を計算する必要がある
        int currentOffset = 0;
        for (const auto& m : m_matGpu) {
            currentOffset += m.indexCount;
        }

        mg.indexStart = currentOffset;
        mg.indexCount = src.faceCount;

        // ----------------------------------------------------
        // ★ 奥義1: 文字列判定を一生に一回だけココで終わらせる！
        // ----------------------------------------------------
        mg.isBody = (src.name.find("Body") != std::string::npos);
        mg.isSpot = (src.name.find("Spot") != std::string::npos);
        mg.isScreen = (src.name.find("Screen") != std::string::npos); // ★コレ

        // 光るオブジェクト判定
        if (src.name.find("pillar") != std::string::npos ||
            mg.isSpot ||
            src.name.find("Bar") != std::string::npos ||
            src.name.find("HALO") != std::string::npos ||
            src.name.find("CIRCLE") != std::string::npos ||
            src.name.find("Light") != std::string::npos ||
            src.name.find(u8"発光") != std::string::npos ||
            src.name.find(u8"ネオン") != std::string::npos ||
            src.shininess >= 100.0f)
        {
            mg.isLightObject = true;
        }

        // 加算合成(透過光)にするかどうかの判定（Drawで使う用）
        if (mg.isLightObject && !mg.isBody && !mg.isScreen) {
            mg.isTransparentLight = true;
        }

        // 基本の発光パワーを決定
        if (mg.isScreen) {
            mg.baseEmission = 0.0f;
        }
        else if (mg.isLightObject && !mg.isBody) {
            if (src.name.find("CIRCLE") != std::string::npos) {
                mg.baseEmission = 0.1f;
            }
            else if (src.name == "LightRedR" || src.name == "LightRedL" ||
                src.name == "LightWhiteR" || src.name == "LightWhiteL" ||
                src.name == "LightGreenR" || src.name == "LightGreenL") {
                mg.baseEmission = 4.0f; // 30→4 に下げて白飛びを抑える（強さの主要ノブ）
            }
            else if (src.name.find("HALO") != std::string::npos || src.name.find("Light") != std::string::npos) {
                mg.baseEmission = 0.4f;
            }
            else if (mg.isSpot) {
                mg.baseEmission = 8.0f;
            }
            else {
                mg.baseEmission = 3.0f; // 20→3 に下げる（その他ライト）
            }
        }

        // リストに追加
        m_matGpu.push_back(mg);

        // 次のマテリアルへ進める
        m_currentTextureIndex++;

        // 「まだ続きがあるよ」と伝える
        return true;
    }

    // -------------------------------------------------------
    // 2. テクスチャ完了後：物理演算の初期化「だけ」で1回分を使う
    //    Bullet剛体の構築は重く、複数モデルが同時にテクスチャを読み終わると
    //    物理初期化が1フレームに集中してロード画面が止まる。
    //    呼び出し側が IsPendingPhysicsInit() を見て「1体/フレーム」に
    //    間引けるよう、テクスチャとは別ステップに分離してある。
    // -------------------------------------------------------
    if (m_physicsEnabled && !m_physics) {
        m_physics = std::make_unique<PMXPhysics>();
        m_physics->Init(*m_model);
        return true; // 物理を作ったフレームはここまで（完了は次の呼び出しで返す）
    }

    // ロード完了フラグを下ろす
    m_isLoading = false;

    // 「ロード終わったよ」と伝える
    return false;
}



void nsK2EngineLow::PMXRender::SetAnimPlaySpeed(float speed)
{
    if (m_animPlayer) m_animPlayer->SetPlaySpeed(speed);
}

void nsK2EngineLow::PMXRender::SyncToMusicTime(float musicTimeSec)
{
    if (m_animPlayer) m_animPlayer->SetCurrentFrame(musicTimeSec * 30.0f);
}

float nsK2EngineLow::PMXRender::GetVMDCurrentFrameDebug() const
{
    return m_animPlayer ? m_animPlayer->GetCurrentFrame() : -1.0f;
}

float nsK2EngineLow::PMXRender::GetVMDEndFrameDebug() const
{
    return m_animPlayer ? m_animPlayer->GetEndFrame() : -1.0f;
}

//================================================================
// RootSignature / PipelineState
//================================================================
bool PMXRender::BuildRootSignature()
{
    // 既に生成済みなら共有（最初の1体だけが実際に作る）
    if (s_rootSig) return true;

    s_rootSig = new RootSignature();
    if (!s_rootSig->BuildPMXRootSignature()) {
        OutputDebugStringA("[PMXRender] RootSignature build failed.\n");
        SafeDelete(s_rootSig);
        return false;
    }
    return true;
}

bool PMXRender::BuildPipelineState(DXGI_FORMAT rtvFmt, DXGI_FORMAT dsvFmt)
{
    // 既に生成済みなら共有（全レンダーで同じシェーダー・レイアウト・出力フォーマットなので作り直さない）
    if (s_pso) return true;

    static D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R16G16B16A16_UINT,  0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_BLEND_DESC blend = {};
    blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    auto& rt = blend.RenderTarget[0];
    rt.BlendEnable = TRUE;
    rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    rt.BlendOp = D3D12_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D12_BLEND_ONE;
    rt.DestBlendAlpha = D3D12_BLEND_ZERO;
    rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;

    D3D12_RASTERIZER_DESC rast = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rast.CullMode = D3D12_CULL_MODE_NONE;

    D3D12_DEPTH_STENCIL_DESC dss = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

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
    auto vs = loadCso(L"x64/Debug/PMX_VS.cso");
    auto ps = loadCso(L"x64/Debug/PMX_PS.cso");
#else
    auto vs = loadCso(L"x64/Release/PMX_VS.cso");
    auto ps = loadCso(L"x64/Release/PMX_PS.cso");
#endif

    if (!vs.bc.pShaderBytecode || !ps.bc.pShaderBytecode) {
        OutputDebugStringA("[PMXRender] Failed to load shader CSOs.\n");
        return false;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = s_rootSig->Get();
    desc.VS = vs.bc;
    desc.PS = ps.bc;
    desc.InputLayout = { layout, _countof(layout) };
    desc.BlendState = blend;
    desc.RasterizerState = rast;
    desc.DepthStencilState = dss;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // ★変更: 2枚に出力する設定へ
    desc.NumRenderTargets = 2;
    desc.RTVFormats[0] = rtvFmt;                  // カラー
    desc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT; // ★ここを変更; // ベロシティ
    desc.DSVFormat = dsvFmt;
    desc.SampleMask = UINT_MAX;
    desc.SampleDesc.Count = 1;

    s_pso = new PipelineState();
    s_pso->Init(desc);

    if (!s_pso->Get()) {
        OutputDebugStringA("[PMXRender] PipelineState build failed.\n");
        SafeDelete(s_pso);
        return false;
    }
    // ★★★ ここから追加：光るポリゴン専用の「加算合成＆Z書き込みOFF」ステート ★★★
    D3D12_GRAPHICS_PIPELINE_STATE_DESC descAdd = desc; // 通常の設定を丸写しする

    // 1. Zバッファ書き込みをOFFにする（これで床に穴が空く貫通バグを100%防ぐ！）
    descAdd.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    // 2. 加算合成にする（MMDの光エフェクト本来の美しい合成方法）
    auto& rtAdd = descAdd.BlendState.RenderTarget[0];
    rtAdd.SrcBlend = D3D12_BLEND_ONE;
    rtAdd.DestBlend = D3D12_BLEND_ONE; // 背景色にそのまま足し算する
    rtAdd.BlendOp = D3D12_BLEND_OP_ADD;

    s_psoAdd = new PipelineState();
    s_psoAdd->Init(descAdd);
    // ★★★ 追加ここまで ★★★
    return true;
}

//================================================================
// Vertex / Index / Constant Buffer
//================================================================
//struct PMXVertexGPU {
//    float    pos[3];      // 0
//    float    normal[3];   // 12
//    float    uv[2];       // 24
//    uint16_t boneIdx[4];  // 32 (8 bytes)
//    float    boneW[4];    // 40
//}; // sizeof = 56 の想定
//static_assert(offsetof(PMXVertexGPU, pos) == 0, "pos offset");
//static_assert(offsetof(PMXVertexGPU, normal) == 12, "normal offset");
//static_assert(offsetof(PMXVertexGPU, uv) == 24, "uv offset");
//static_assert(offsetof(PMXVertexGPU, boneIdx) == 32, "boneIdx offset");
//static_assert(offsetof(PMXVertexGPU, boneW) == 40, "boneW offset");

bool PMXRender::BuildBuffers(const PMXModel& m)
{
    // ---- 頂点バッファ ----
    std::vector<PMXVertexGPU> vb;
    vb.reserve(m.vertices.size());

    for (auto& v : m.vertices) {
        PMXVertexGPU g{};
        memcpy(g.pos, v.pos, sizeof(g.pos));
        memcpy(g.normal, v.normal, sizeof(g.normal));
        memcpy(g.uv, v.uv, sizeof(g.uv));

        uint32_t bi32[4] = { 0,0,0,0 };
        float    bw[4] = { 0,0,0,0 };

        int n = (int)std::min<size_t>(4, v.boneIndices.size());
        int nw = (int)std::min<size_t>(4, v.boneWeights.size());

        for (int k = 0; k < n; ++k) {
            uint32_t idx = (uint32_t)v.boneIndices[k];
            if (idx >= 0xFFFF) idx = 0xFFFF;   // safety
            bi32[k] = idx;
        }

        float sum = 0.0f;
        for (int k = 0; k < nw; ++k) { bw[k] = (float)v.boneWeights[k]; sum += bw[k]; }

        if (sum <= 1e-6f) {
            bw[0] = 1.0f;
            for (int k = 1; k < 4; ++k) bw[k] = 0.0f;
        }
        else {
            float inv = 1.0f / sum;
            for (int k = 0; k < 4; ++k) bw[k] *= inv;
        }

        for (int k = 0; k < 4; ++k) {
            g.boneIdx[k] = static_cast<uint16_t>(bi32[k]);
            g.boneW[k] = bw[k];
        }

        vb.push_back(g);
    }

    // ★★★ ここに追加 ★★★
    // 作成したGPU用頂点データをバックアップ（モーフ計算の基準にするため）
    m_originalVerticesGPU = vb;
    // [RHYTHM_GAME_MOD] モーフが存在しない（＝背景ステージ等）場合はVRAM専用バッファを使う
    if (m.morphs.empty()) {
        m_vbVRAM = new VertexBufferVRAM();

        // ※注意: エンジン仕様に合わせて、コマンドリストを取得してください
        // K2EngineLowの場合、RenderContextなどから取得できることが多いです
        ID3D12GraphicsCommandList* cmdList = g_graphicsEngine->GetCommandList(); // ←※適宜エンジンに合わせて修正してください

        m_vbVRAM->Init(vb.data(), (int)(vb.size() * sizeof(PMXVertexGPU)), sizeof(PMXVertexGPU));
    }
    else {
        // モーフが存在する（＝キャラ）場合は今まで通り
        m_vb = new VertexBuffer();
        m_vb->Init((int)(vb.size() * sizeof(PMXVertexGPU)), sizeof(PMXVertexGPU));
        m_vb->Copy(vb.data());
    }

    // ---- インデックスバッファ ----
    m_ib = new IndexBuffer();
    std::vector<uint32_t> idx(m.indices.begin(), m.indices.end());
    m_ib->Init((int)(idx.size() * sizeof(uint32_t)), sizeof(uint32_t));
    m_ib->Copy(idx.data());

    // ---- 定数バッファ ----
    m_vsCB = new ConstantBuffer();
    m_vsCB->Init(sizeof(VSPerDraw));
   /* m_psCB = new ConstantBuffer();
    m_psCB->Init(sizeof(PSMaterial));*/

    // ---- ボーン行列（初期：Identity）----
    m_boneMatrices.resize(m.bones.size());
    for (auto& M : m_boneMatrices) XMStoreFloat4x4(&M, XMMatrixIdentity());
    // ★追加：ロード中にヒープに登録するため、ボーン用バッファをここで事前に作っておく
    if (!m_boneMatrixSB) {
        m_boneMatrixSB = std::make_unique<StructuredBuffer>();
        m_boneMatrixSB->Init(sizeof(DirectX::XMFLOAT4X4), (int)m_boneMatrices.size(), nullptr);
        m_boneMatrixSB->Update(m_boneMatrices.data());
    }
    return true;
}

//================================================================
// テクスチャ
//================================================================
bool PMXRender::BuildTextures(const PMXModel& model)
{
    m_matGpu.clear();
    m_matGpu.reserve(model.materials.size());

    auto* device = g_graphicsEngine->GetD3DDevice();
    int indexOffset = 0;

    for (auto& src : model.materials) {
        MatGPU mg{};

        auto makeTex = [&](const std::string& path)->Texture*
            {
                if (path.empty()) return nullptr;
                std::wstring w(path.begin(), path.end());
                Texture* t = new Texture(w.c_str());

                D3D12_DESCRIPTOR_HEAP_DESC hd{};
                hd.NumDescriptors = 1;
                hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                ComPtr<ID3D12DescriptorHeap> heap;
                device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&heap));
                auto cpu = heap->GetCPUDescriptorHandleForHeapStart();
                t->RegistShaderResourceView(cpu, 0);
                return t;
            };

        mg.albedo = makeTex(src.textureFile);
        mg.sphere = makeTex(src.sphereTextureFile);
        mg.toon = makeTex(src.toonTextureFile);

        mg.psData.diffuse = {
            src.diffuse[0], src.diffuse[1],
            src.diffuse[2], src.diffuse[3]
        };

        // ★★★ ここを追加：AutoLuminousの再現 ★★★
        float emissionIntensity = 0.0f;
        // 反射強度が100以上なら発光とみなす（MMDのAutoLuminous標準仕様）
        if (src.shininess >= 100.0f) {
            emissionIntensity = 3.0f; // 3倍の強さで光らせる（好みに合わせて調整）
        }
        // おまけ：マテリアル名に特定の文字が入っていたら強制発光
        if (src.name.find("発光") != std::string::npos ||
            src.name.find("ネオン") != std::string::npos) {
            emissionIntensity = 5.0f;
        }

        // filterParams の x,y,z はDraw内で上書きされるので、初期値としてwに発光強度を入れておく
        mg.psData.filterParams.w = emissionIntensity;

        mg.indexStart = indexOffset;
        mg.indexCount = src.faceCount;
        indexOffset += mg.indexCount;

        m_matGpu.push_back(mg);
    }

    return true;
}

//================================================================
// Bone Matrices 更新のメイン
//================================================================
void PMXRender::UpdateBoneMatrices()
{
    if (!m_model) return;
    auto& bones = const_cast<std::vector<PMXBone>&>(m_model->bones);
    size_t boneCount = bones.size();

    // 1. 作業用グローバル行列の確保
    //    ※毎フレーム new/delete しないようメンバのスクラッチを使い回す
    //    （9体×約300ボーン×64byteの確保・解放が毎フレーム走っていた）
    m_globalsScratch.resize(boneCount);
    std::vector<DirectX::XMMATRIX>& globals = m_globalsScratch;

    // 2. VMDの結果を PMXBone に反映 (Sync)
    if (m_animPlayer) {
        const auto& nodes = m_animPlayer->GetNodeManager().GetNodes();
        for (size_t i = 0; i < boneCount; ++i) {
            bones[i].currentPosition = nodes[i].deltaT;
            bones[i].currentRotation = nodes[i].deltaR;
        }
    }
    else {
        // VMDがない場合は初期値リセット
        for (auto& b : bones) {
            b.currentPosition = { 0,0,0 };
            b.currentRotation = { 0,0,0,1 };
        }
    }

    // ★追加1：ボーンモーフ（武器の持ち手やサイズの変形など）の適用！
    // VMDの姿勢を反映した直後、グローバル行列を計算する前に呼び出します。
    ApplyMorphs();

    // 3. 基本のグローバル行列を計算 (FK)
    BuildGlobals(globals);

    // ★追加2：現在のフレームのIK状態（ON/OFF）を取得
    std::unordered_map<std::string, bool> currentIKStates;
    if (m_animPlayer && m_animPlayer->GetVMDLoader()) {
        const auto& ikAnim = m_animPlayer->GetVMDLoader()->GetDisplayIKAnim();
        if (!ikAnim.empty()) {
            float curFrame = m_animPlayer->GetCurrentFrame();
            // 現在のフレーム以下の最新キーを探す
            auto it = std::upper_bound(ikAnim.begin(), ikAnim.end(), curFrame,
                [](float f, const auto& k) { return f < k.frame; });

            if (it != ikAnim.begin()) {
                const auto& key = *(it - 1);
                for (const auto& state : key.ikStates) {
                    currentIKStates[state.boneName] = state.enable;
                }
            }
        }
    }

    // 4. IK (Inverse Kinematics) を解く！（★引数を追加して呼び出す）
    SolveIK(globals, currentIKStates);

    // ApplyGrantはcurrentRotationしか読まないのでIK後のBuildGlobalsを省略し
    // Grant適用後にまとめて1回だけ再計算する（3回→2回に削減）
    ApplyGrant();
    BuildGlobals(globals);
    // ★★★ 物理演算 ★★★
    if (m_physicsEnabled && m_physics && m_animPlayer) {

        // 初回リセット＋ウォームアップ（PHYSICS_WARMUP_PER_FRAME ステップ/フレームに分散）
        if (m_physicsReset) {
            m_physics->ResetRigidBodies(globals);
            m_physicsReset = false;
        }
        if (m_physicsWarmupRemain > 0) {
            int steps = (m_physicsWarmupRemain < PHYSICS_WARMUP_PER_FRAME)
                        ? m_physicsWarmupRemain : PHYSICS_WARMUP_PER_FRAME;
            for (int i = 0; i < steps; ++i)
                m_physics->Update(1.0f / 60.0f, globals);
            m_physicsWarmupRemain -= steps;
            return; // ウォームアップ中はスキニングをスキップして軽く
        }

        // 通常更新（1回だけ）。固定dt(1/60)で歩進する。
        //   ※以前は実測フレーム時間を渡していたが、stepSimulation(dt,5,1/60) は dt が
        //     大きいほどサブステップが増えるため、物理体が多いRhythmGameでフレーム落ち時に
        //     コストが倍増し「もっさり」する悪循環を招いていた。固定1/60なら常に1サブステップで軽い。
        m_physics->Update(g_gameTime->GetFrameDeltaTime(), globals);
    }

    // 6. ルート移動の反映 (Model World Transform)
    XMMATRIX Rroot = XMMatrixRotationQuaternion(XMLoadFloat4(&m_rootRot));
    XMMATRIX Troot = XMMatrixTranslation(m_rootPos.x, m_rootPos.y, m_rootPos.z);
    XMMATRIX Mroot = Rroot * Troot;

    for (auto& g : globals) {
        g = g * Mroot;
    }

    // 7. スキニング行列の構築 (InvBind * Global)
    //    ※GPUへの転送はここでは行わない（Draw側でシリアルに行う）。
    //      これによりUpdateBoneMatrices全体をワーカースレッドで並列実行できる。
    UploadSkinMatrices(globals);
    m_skinDirtyFrames = 2; // StructuredBufferはダブルバッファなので両面に転送する（詳細はヘッダ参照）
    m_staticPoseUploaded = true; // 静的モデルはこれ以降の再計算をスキップできる
}
//================================================================
// ★CPUアニメーション更新（並列実行可。GPUには一切触らない）
//   VMD再生 → カメラ評価（結果は保存のみ） → ボーン/IK/物理 → 頂点・マテリアルモーフ
//   GPU転送（頂点バッファ・ボーン行列SB・定数バッファ）はDraw側でシリアルに行う。
//================================================================
void PMXRender::UpdateAnimation()
{
    if (!m_validModel || !m_model) return;
    if (m_cpuUpdated) return; // 同一フレームの二重更新防止（Drawフォールバックとの併用対策）
    m_cpuUpdated = true;

    // VMD 再生
    if (m_animPlayer) {
        m_animPlayer->Update(g_gameTime->GetFrameDeltaTime());

        // カメラVMDの評価。g_camera3Dはグローバルなので【ここでは適用しない】。
        // 結果だけ保存して、Draw（メインスレッド）で適用する。
        m_hasPendingCamera = false;
        if (m_enableCameraVMD)
        {
            VMDCameraState camState;
            if (m_animPlayer->EvaluateCamera(m_animPlayer->GetCurrentFrame(), camState))
            {
                m_pendingCamEye = { camState.eye.x, camState.eye.y, -camState.eye.z };
                m_pendingCamFocus = { camState.focus.x, camState.focus.y, camState.focus.z };
                m_pendingCamUp = { camState.up.x, camState.up.y, camState.up.z };
                m_hasPendingCamera = true;
            }
        }
    }
    else {
        m_hasPendingCamera = false;

        // ★静的モデルの丸ごとスキップ（地面などVMDも物理も無いモデル）。
        //   一度ポーズを転送済みなら、ボーン・IK・モーフの再計算は不要。
        const bool physicsActive = m_physicsEnabled && m_physics && m_physics->HasRigidBodies();
        if (m_staticPoseUploaded && !physicsActive) {
            return;
        }
    }

    // ボーン行列更新（IK・物理込み。結果は m_boneMatrices に格納される）
    UpdateBoneMatrices();

    // 頂点モーフ（表情）の更新（結果は m_morphVertScratch とダーティ範囲に格納される）
    UpdateVertices();

    // マテリアルモーフ（照明）の更新（結果は m_matGpu[].psData に格納される）
    UpdateMaterialMorphs();
}

//================================================================
// 描画（GPU転送＋描画コマンド。必ずメインスレッドから呼ぶこと）
//================================================================
void PMXRender::Draw(RenderContext& rc)
{
    // モデルが正しく初期化できていない（空モデル等）なら描画しない。
    // ここを通すとボーンバッファ生成や頂点参照でクラッシュするため必ず弾く。
    if (!m_validModel || !m_model) return;

    // RenderingEngine が並列更新を呼ばなかった場合の互換フォールバック
    // （単体でDrawだけ呼んでも従来どおり動く）
    if (!m_cpuUpdated) {
        UpdateAnimation();
    }
    m_cpuUpdated = false; // 次フレーム用にリセット

    // ★カメラVMDの適用（UpdateAnimationで評価した結果をここで反映）
    if (m_hasPendingCamera) {
        g_camera3D->SetPosition(Vector3(m_pendingCamEye.x, m_pendingCamEye.y, m_pendingCamEye.z));
        g_camera3D->SetTarget(Vector3(m_pendingCamFocus.x, m_pendingCamFocus.y, m_pendingCamFocus.z));
        g_camera3D->SetUp(Vector3(m_pendingCamUp.x, m_pendingCamUp.y, m_pendingCamUp.z));
        g_camera3D->Update();
        m_hasPendingCamera = false;
    }

    // ★GPU転送（CPU側で用意したデータをここでまとめてアップロード）
    // スキニング行列はダブルバッファの両面が最新になるまで転送する
    if (m_skinDirtyFrames > 0 && m_boneMatrixSB) {
        m_boneMatrixSB->Update(m_boneMatrices.data());
        m_skinDirtyFrames--;
    }
    if (m_vbDirty && m_vb && !m_morphVertScratch.empty()) {
        // モーフが影響する範囲だけを部分転送する（全頂点転送より大幅に軽い）
        const uint32_t stride = sizeof(PMXVertexGPU);
        const uint32_t offset = m_morphMinVert * stride;
        const uint32_t size = (m_morphMaxVert - m_morphMinVert + 1) * stride;
        m_vb->CopyRange(&m_morphVertScratch[m_morphMinVert], offset, size);
        m_vbDirty = false;
    }

    rc.SetViewportAndScissor(g_graphicsEngine->GetFrameBufferViewport());
    rc.SetRootSignature(*s_rootSig);
    rc.SetPipelineState(*s_pso);
    rc.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // [RHYTHM_GAME_MOD] どちらのバッファが作られているかでセットを切り替える
    if (m_vbVRAM) {
        // RenderContextではなく、g_graphicsEngineから直接コマンドリストを取得する
        auto view = m_vbVRAM->GetView();
        g_graphicsEngine->GetCommandList()->IASetVertexBuffers(0, 1, &view);
    }
    else if (m_vb) {
        rc.SetVertexBuffer(*m_vb);
    }
    rc.SetIndexBuffer(*m_ib);

    // VS 定数（b0）
    VSPerDraw vs{};
    XMMATRIX S = XMMatrixIdentity();
    XMMATRIX R = XMMatrixIdentity();
    XMMATRIX T = XMMatrixIdentity();
    XMStoreFloat4x4(&vs.world, XMMatrixTranspose(S * R * T));

    XMStoreFloat4x4(&vs.viewProj,
        XMMatrixTranspose(g_camera3D->GetViewProjectionMatrix()));

    // ★追加: 現在と過去の行列計算
    XMMATRIX mWorld = XMLoadFloat4x4(&vs.world);     // Transpose済みなら戻すなど調整必要
    XMMATRIX mViewProj = XMLoadFloat4x4(&vs.viewProj);
    // エンジンの行列実装に合わせて計算してください (World * View * Proj)
    XMMATRIX mCurrentWVP = mWorld * mViewProj;
    
    if (m_isFirstFrame) {
    XMStoreFloat4x4(&vs.oldWorldViewProj, XMMatrixTranspose(mCurrentWVP));
    m_isFirstFrame = false;
    }
    else {
    vs.oldWorldViewProj = m_prevWorldViewProj;
    }
    // 次回用に保存
    XMStoreFloat4x4(&m_prevWorldViewProj, XMMatrixTranspose(mCurrentWVP));

    // ★リムライト用：カメラ世界座標(xyz)とリム強度(w)をセット
    {
        Vector3 camPos = g_camera3D->GetPosition();
        vs.cameraPos = DirectX::XMFLOAT4(camPos.x, camPos.y, camPos.z, m_rimStrength);
    }

    m_vsCB->CopyToVRAM(&vs);
    // 同一フレームでの二重 Barrier を避けるため
    std::unordered_set<ID3D12Resource*> promoted;

    auto promoteSRV = [&](Texture* t) {
        if (!t || !t->IsValid()) return;
        auto* res = t->Get();
        if (!res) return;
        if (promoted.find(res) != promoted.end()) return;
        rc.TransitionResourceState(
            res, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        promoted.insert(res);
        };

    // ========================================================
     // ★ 完璧な「光」の仕分け専用ルール（完全統一版！）
    auto isTransparentLight = [&](const MatGPU& mg) {
        return mg.isTransparentLight;
        };

    // ========================================================
    // --- 1段目：不透明なもの（床、壁、ライトの「胴体」！）を通常描画 ---
    // ========================================================
    rc.SetPipelineState(*s_pso);

    for (size_t i = 0; i < m_matGpu.size(); ++i)
    {
        auto& m = m_matGpu[i];
        const auto& src = m_model->materials[i];

        // 🚨変更：src.name ではなく、src 本体を渡す！
        if (isTransparentLight(m)) continue;

        float emission = m.psData.filterParams.w;
        m.psData.filterParams = m_currentFilter;
        m.psData.filterParams.z = m_motionBlurMaskVal;
        m.psData.filterParams.w = emission;

        m.psCB->CopyToVRAM(&m.psData);
        Texture* albedo = m.albedo ? m.albedo : s_fallbackAlbedo.get();
        promoteSRV(albedo);
        rc.SetDescriptorHeap(*(m.descHeap));
        rc.DrawIndexed(m.indexCount, m.indexStart);
    }

    // ========================================================
    // --- 2段目：透明な光のパーツだけを「加算合成＆Z書き込みOFF」で乗せる ---
    // ========================================================
    if (s_psoAdd) rc.SetPipelineState(*s_psoAdd);

    for (size_t i = 0; i < m_matGpu.size(); ++i)
    {
        auto& m = m_matGpu[i];
        const auto& src = m_model->materials[i];

        // 🚨変更：src.name ではなく、src 本体を渡す！
        if (!isTransparentLight(m)) continue;

        float emission = m.psData.filterParams.w;
        m.psData.filterParams = m_currentFilter;
        m.psData.filterParams.z = m_motionBlurMaskVal;
        m.psData.filterParams.w = emission;

        m.psCB->CopyToVRAM(&m.psData);
        Texture* albedo = m.albedo ? m.albedo : s_fallbackAlbedo.get();
        promoteSRV(albedo);
        rc.SetDescriptorHeap(*(m.descHeap));
        rc.DrawIndexed(m.indexCount, m.indexStart);
    }
}

//================================================================
// VMD
//================================================================
void PMXRender::AttachVMD(VMDLoader* vmd)
{
    // VMDの付け外しでポーズが変わるため、静的スキップを解除して再計算させる
    m_staticPoseUploaded = false;

    if (!vmd) {
        m_animPlayer.reset();
        return;
    }
    if (!m_animPlayer) {
        m_animPlayer = std::make_unique<VMDAnimPlayer>();
    }
    m_animPlayer->Init(const_cast<PMXModel*>(m_model), vmd);
    m_animPlayer->SetOwner(this);
}

//================================================================
// 後片付け
//================================================================
void PMXRender::Destroy()
{
    // s_rootSig / s_pso / s_psoAdd は全インスタンス共有の静的リソースなので、
    // ここでは解放しない（解放すると他のレンダーが死ぬ）。プロセス終了時にOSが回収する。
    SafeDelete(m_vb);
    SafeDelete(m_vbVRAM); // ★これを追加！
    SafeDelete(m_ib);
    SafeDelete(m_vsCB);
    //SafeDelete(m_psCB);

    for (auto& m : m_matGpu) {
        // albedo/sphere/toon は s_textureCache で共有しているのでここでは解放しない。
        // （キャッシュが所有。プロセス終了時にOSが回収する）
        SafeDelete(m.psCB);      // ★追加
        SafeDelete(m.descHeap);  // ★追加
    }
    m_matGpu.clear();

    // ★モデル差し替え（再Init）でボーン数が変わっても正しく作り直せるようにリセットする。
    //   これらは「存在しなければ作る」方式なので、残しておくと旧モデルのままになり破綻する。
    m_boneMatrixSB.reset();          // ボーン行列バッファ（新モデルのボーン数で再生成させる）
    m_physics.reset();               // 物理（新モデルの剛体で再生成させる）
    m_physicsReset = true;
    m_physicsWarmupRemain = PHYSICS_WARMUP_STEPS;
    m_prevMorphWeights.clear();      // モーフキャッシュ
    m_forceMorphUpdate = true;
    m_validModel = false;            // 再Init完了まで描画させない
    m_isLoading = false;
    m_currentTextureIndex = 0;

    // ★モデル差し替えに備えて、高速化用のキャッシュ・フラグも全部リセットする
    m_morphLookupBuilt = false;
    m_morphIndexByName.clear();
    m_morphAffectedVerts.clear();
    m_morphVertScratch.clear();
    m_globalsScratch.clear();
    m_cpuUpdated = false;
    m_skinDirtyFrames = 0;
    m_vbDirty = false;
    m_staticPoseUploaded = false;
    m_hasPendingCamera = false;
}

//================================================================
// Fallback テクスチャ
//================================================================
void PMXRender::EnsureFallbackTextures()
{
    if (s_fallbackAlbedo) return;

    // ★大元凶の「GetNullTextureMaps()」を完全削除！！！★
    // エンジンの真っ赤なエラーテクスチャを強制ロードする罠を粉砕しました。

    // 用意した「純白の画像」だけを確実に読み込ませます！
    s_fallbackAlbedo = std::make_unique<Texture>(L"Assets/modelData/preset/white1x1.dds");
}

//================================================================
// 付与（Grant） 
//================================================================
void PMXRender::ApplyGrant()
{
    // ★追加：付与も「親→子」の順で計算しないと、付与の連鎖（親の付与→子の付与）が途切れます
    // ソートリストが空（未実装）の対策
    const auto& bones = m_model->bones;
    const auto& indices = m_model->sortedBoneIndices;
    bool useSort = !indices.empty();
    size_t count = bones.size();

    // ★修正1：変形階層順（ソート順）でループする
    for (size_t k = 0; k < count; ++k)
    {
        int i = useSort ? indices[k] : (int)k;

        // const_cast は避けられませんが、参照で受け取ります
        PMXBone& b = const_cast<PMXBone&>(bones[i]);

        if (b.grantParentIndex < 0) continue;
        if (b.grantWeight <= 0.0001f) continue;

        const PMXBone& p = bones[b.grantParentIndex];

        // 回転付与
        if (!b.isTranslationGrant)
        {
            // ★修正2：Slerp（補間）ではなく、Multiply（合成/加算）にする！
            // 以前のコード：qMix = Slerp(qB, qP, w);  <-- これだと自分の回転(qB)が消えてしまう！
            // 正しいコード：qGrant = Slerp(Identity, qP, w); qFinal = qB * qGrant;

            // 1. 親の回転(qP)から、ウェイト分だけ回転量を抽出する
            XMVECTOR qParent = XMLoadFloat4(&p.currentRotation);
            XMVECTOR qGrant = XMQuaternionSlerp(XMQuaternionIdentity(), qParent, b.grantWeight);

            // 2. 自分の回転(qSelf)に、親の回転(qGrant)を「足し合わせる（掛ける）」
            XMVECTOR qSelf = XMLoadFloat4(&b.currentRotation);

            // ※MMDの回転付与は「親の回転 * 自分の回転」の順序が一般的（要確認ですが通常はこれでOK）
            XMVECTOR qFinal = XMQuaternionMultiply(qGrant, qSelf);

            qFinal = XMQuaternionNormalize(qFinal);
            XMStoreFloat4(&b.currentRotation, qFinal);
        }
        else
        {
            // 移動付与（こちらはベクトルの足し算なので Lerp でOKな場合もありますが、Addが確実）
            XMVECTOR tp = XMLoadFloat3(&p.currentPosition);
            XMVECTOR tb = XMLoadFloat3(&b.currentPosition);

            // 親の移動量 * ウェイト
            XMVECTOR tGrant = tp * b.grantWeight;

            // 自分に足す
            XMVECTOR tFinal = XMVectorAdd(tb, tGrant);

            XMStoreFloat3(&b.currentPosition, tFinal);
        }
    }
}

//================================================================
// ローカル → グローバル 
//================================================================
void PMXRender::BuildGlobals(std::vector<XMMATRIX>& globals) const
{
    const auto& bones = m_model->bones;
    globals.resize(bones.size()); // ルートボーンはelseブランチでglobals[i]に直接書くので初期化不要

    // ★追加：ソートリストが空の場合の安全策
    const auto& indices = m_model->sortedBoneIndices;
    bool useSort = !indices.empty();
    size_t count = bones.size();

    // ★修正：単純な 0..count のループではなく、
    // 「ソートされた順番（変形階層順）」で回すループに変更します
    for (size_t k = 0; k < count; ++k)
    {
        // ソートリストがあればそこからIDを取り出し、なければそのまま k を使う
        int i = useSort ? indices[k] : (int)k;

        const PMXBone& b = bones[i];

        // 1. 回転 (R)
        XMVECTOR q = XMLoadFloat4(&b.currentRotation);
        q = XMQuaternionNormalize(q);
        XMMATRIX R = XMMatrixRotationQuaternion(q);

        // 2. 移動 (T)
        //    localOffset と currentPosition を足して移動量にする
        XMMATRIX T = XMMatrixTranslation(
            b.localOffset.x + b.currentPosition.x,
            b.localOffset.y + b.currentPosition.y,
            b.localOffset.z + b.currentPosition.z);

        // ★★★ 修正ポイント1：ローカル行列の順序 ★★★
        // 修正前: XMMATRIX local = T * R;
        // 修正後: 回転させてから移動 (R * T)
        XMMATRIX local = R * T;

        // ★★★ 修正ポイント2：親との結合順序 ★★★
        if (b.parentIndex >= 0)
            // 修正前: globals[i] = globals[b.parentIndex] * local;
            // 修正後: 子(Local) × 親(Parent)
            globals[i] = local * globals[b.parentIndex];
        else
            globals[i] = local;
    }
}
//================================================================
// IK  ※今は UpdateBoneMatrices からは呼んでいない
//================================================================
static inline float Clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }

// 便利なオイラー角変換関数（追加してください）
static void QuatToEuler(const DirectX::XMFLOAT4& q, DirectX::XMFLOAT3& outEuler)
{
    // Y-up Left-Handed での簡易変換（MMD互換のため ZXY 順などで分解するのが一般的ですが、
    // ここでは X軸制限(膝)が動けばいいので簡易実装にします）
    // 本格的なMMD互換にするなら "ZXY" order decomposition が必要です
    XMVECTOR Q = XMLoadFloat4(&q);
    XMMATRIX M = XMMatrixRotationQuaternion(Q);
    XMFLOAT4X4 m; XMStoreFloat4x4(&m, M);

    // Pitch (X)
    outEuler.x = asinf(-m._32);

    // 簡易的な Yaw/Roll (Gimbal Lock回避等は省略)
    if (cosf(outEuler.x) > 0.0001f) {
        outEuler.y = atan2f(m._31, m._33);
        outEuler.z = atan2f(m._12, m._22);
    }
    else {
        outEuler.y = 0.0f;
        outEuler.z = atan2f(-m._21, m._11);
    }
}

// ---------------------------------------------------------
// [Helper] XYZ順 (膝のようにX軸が深く曲がる関節用)
// ---------------------------------------------------------
static DirectX::XMFLOAT3 QuatToEulerXYZ(const DirectX::XMFLOAT4& qVal)
{
    using namespace DirectX;
    XMMATRIX M = XMMatrixRotationQuaternion(XMLoadFloat4(&qVal));
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, M);

    float x, y, z;
    // XYZ順 (Pitch-Yaw-Roll)
    if (m._32 < -0.99999f) {
        x = XM_PIDIV2; y = 0; z = atan2f(m._12, m._22);
    }
    else if (m._32 > 0.99999f) {
        x = -XM_PIDIV2; y = 0; z = atan2f(m._12, m._22);
    }
    else {
        x = asinf(-m._32);
        y = atan2f(m._31, m._33);
        z = atan2f(-m._12, m._22);
    }
    return { x, y, z };
}

static DirectX::XMFLOAT4 EulerXYZToQuat(const DirectX::XMFLOAT3& rot)
{
    using namespace DirectX;
    XMVECTOR Q = XMQuaternionRotationRollPitchYaw(rot.x, rot.y, rot.z);
    XMFLOAT4 q; XMStoreFloat4(&q, Q);
    return q;
}

// ---------------------------------------------------------
// [Helper] ZXY順 (足首のように3軸回る関節用)
// ---------------------------------------------------------
static DirectX::XMFLOAT3 QuatToEulerZXY(const DirectX::XMFLOAT4& qVal)
{
    using namespace DirectX;
    XMMATRIX M = XMMatrixRotationQuaternion(XMLoadFloat4(&qVal));
    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, M);

    float x, y, z;
    // ZXY順 (Tait-Bryan)
    if (m._32 < -0.99999f) {
        x = XM_PIDIV2; y = 0.0f; z = atan2f(m._12, m._22);
    }
    else if (m._32 > 0.99999f) {
        x = -XM_PIDIV2; y = 0.0f; z = atan2f(m._12, m._22);
    }
    else {
        x = asinf(m._32);
        y = atan2f(-m._31, m._33);
        z = atan2f(-m._12, m._22);
    }
    return { x, y, z };
}

static DirectX::XMFLOAT4 EulerZXYToQuat(const DirectX::XMFLOAT3& rot)
{
    using namespace DirectX;
    // Z -> X -> Y の順序で合成
    XMVECTOR Qz = XMQuaternionRotationAxis(XMVectorSet(0, 0, 1, 0), rot.z);
    XMVECTOR Qx = XMQuaternionRotationAxis(XMVectorSet(1, 0, 0, 0), rot.x);
    XMVECTOR Qy = XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), rot.y);
    XMVECTOR Q = XMQuaternionMultiply(XMQuaternionMultiply(Qy, Qx), Qz);

    XMFLOAT4 q; XMStoreFloat4(&q, Q);
    return q;
}

// ---------------------------------------------------------
// IK解決
// ---------------------------------------------------------
// 引数に ikStates を追加
void PMXRender::SolveIK(std::vector<XMMATRIX>& globals, const std::unordered_map<std::string, bool>& ikStates)
{
    if (!m_model) return;
    auto& bones = const_cast<std::vector<PMXBone>&>(m_model->bones);

    // グローバル座標取得ヘルパ
    auto GetGlobalPos = [&](int i) {
        XMFLOAT3 pos;
        XMStoreFloat3(&pos, globals[i].r[3]);
        return XMLoadFloat3(&pos);
        };

    for (size_t i = 0; i < bones.size(); ++i)
    {
        PMXBone& ikBone = bones[i];
        if (!ikBone.hasIK) continue;

        // ★追加3：VMDでIKがOFFに設定されている場合はスキップする！
        bool isEnabled = true; // デフォルトはON
        auto it = ikStates.find(ikBone.name);
        if (it != ikStates.end()) {
            isEnabled = it->second;
        }

        if (!isEnabled) {
            continue; // IK計算を行わず、FK(VMDの直接指定)の姿勢をそのまま使う
        }

        int targetIdx = ikBone.ikTargetIndex;
        if (targetIdx < 0) continue;

        int  loopCount = (std::min)((std::max)(1, ikBone.ikLoopCount), 8); // 9体×2脚×100→8反復に制限
        float limitRad = ikBone.ikLimitAngle;
        if (limitRad <= 0.0f) limitRad = XM_PI;

        // ★★★ ここから修正 ★★★
        // IKループに入る前に、対象となるIKボーンとその親（足IK親など）の
        // グローバル行列を「最新の VMD ローカル差分」から再計算して確実にする

        auto updateGlobalMatrix = [&](int boneIdx) {
            if (boneIdx < 0) return;
            // 親を先に更新
            int pIdx = bones[boneIdx].parentIndex;
            if (pIdx >= 0) {
                // 再帰的に親を更新（深すぎるとスタックオーバーフローしますが、PMXの構造なら数階層なので問題ありません）
                // ※より安全にするなら、BuildGlobals全体を呼び直すか、事前に変形階層順でソートして更新します。
            }

            XMVECTOR q = XMQuaternionNormalize(XMLoadFloat4(&bones[boneIdx].currentRotation));
            XMMATRIX R = XMMatrixRotationQuaternion(q);
            XMMATRIX T = XMMatrixTranslation(
                bones[boneIdx].localOffset.x + bones[boneIdx].currentPosition.x,
                bones[boneIdx].localOffset.y + bones[boneIdx].currentPosition.y,
                bones[boneIdx].localOffset.z + bones[boneIdx].currentPosition.z
            );
            XMMATRIX local = R * T;
            if (bones[boneIdx].parentIndex >= 0) {
                globals[boneIdx] = local * globals[bones[boneIdx].parentIndex];
            }
            else {
                globals[boneIdx] = local;
            }
            };

        // VMDの値を反映した最新のIK目標位置を取得
        updateGlobalMatrix((int)i);
        // ★★★ 修正ここまで ★★★

        // CCD IK ループ
        for (int it = 0; it < loopCount; ++it)
        {
            XMVECTOR destPos = GetGlobalPos((int)i); // IKボーン位置

            // リンク処理
            for (size_t chainIdx = 0; chainIdx < ikBone.ikLinks.size(); ++chainIdx)
            {
                auto& link = ikBone.ikLinks[chainIdx];
                int linkIdx = link.linkBoneIndex;
                if (linkIdx < 0) continue;

                // ---------------------------------------------------------
          // CCD IK ループの中（リンク処理）
          // ---------------------------------------------------------
                XMVECTOR linkPos = GetGlobalPos(linkIdx);
                XMVECTOR currentEffectorPos = GetGlobalPos(targetIdx);

                // 🚨【修正1】距離が近すぎて「ゼロ割り(NaN)」になるのを防ぐ！
                XMVECTOR vecEffector = currentEffectorPos - linkPos;
                XMVECTOR vecDest = destPos - linkPos;
                if (XMVectorGetX(XMVector3LengthSq(vecEffector)) < 1e-6f ||
                    XMVectorGetX(XMVector3LengthSq(vecDest)) < 1e-6f)
                {
                    continue; // 距離がゼロなら計算をスキップして爆発を防ぐ！
                }

                XMVECTOR vToEffector = XMVector3Normalize(vecEffector);
                XMVECTOR vToDest = XMVector3Normalize(vecDest);

                float dot = XMVectorGetX(XMVector3Dot(vToEffector, vToDest));
                dot = std::clamp(dot, -1.0f, 1.0f);
                float angle = acosf(dot);

                if (fabsf(angle) < 0.0001f) continue;
                if (angle > limitRad) angle = limitRad;

                XMVECTOR cross = XMVector3Cross(vToEffector, vToDest);
                float lenSq = XMVectorGetX(XMVector3LengthSq(cross));
                if (lenSq < 1.0e-8f) continue;

                XMVECTOR axis = XMVector3Normalize(cross);

                // 親空間(Local) へ変換
                XMMATRIX invParent = XMMatrixIdentity();
                if (bones[linkIdx].parentIndex >= 0) {
                    invParent = XMMatrixInverse(nullptr, globals[bones[linkIdx].parentIndex]);
                }
                XMVECTOR localAxis = XMVector3TransformNormal(axis, invParent);
                localAxis = XMVector3Normalize(localAxis);

                // ---------------------------------------------------------
                // 回転反映
                // ---------------------------------------------------------
                XMVECTOR qRotDelta = XMQuaternionRotationAxis(localAxis, angle);
                XMVECTOR qCurrent = XMLoadFloat4(&bones[linkIdx].currentRotation);

                // 🚨【修正2】DirectXMathは「A * B」で「Bの後にAを適用」になります。
                // qRotDeltaは親空間で計算されたので「後」に掛けなければなりません！
                // 変更前: XMQuaternionMultiply(qRotDelta, qCurrent)
                // 変更後: XMQuaternionMultiply(qCurrent, qRotDelta) ←左右逆！
                XMVECTOR qNew = XMQuaternionNormalize(XMQuaternionMultiply(qCurrent, qRotDelta));

                // -------------------------------------------------------------
                // ★ハイブリッド角度制限 (Knee vs Ankle)
                // -------------------------------------------------------------
                if (link.hasLimit)
                {
                    XMFLOAT4 qTmp;
                    XMStoreFloat4(&qTmp, qNew);

                    // Y軸・Z軸の制限範囲がほぼ0なら「膝（1軸関節）」とみなす判定
                    bool isKnee = (fabsf(link.limitMin.y) < 1e-3f && fabsf(link.limitMax.y) < 1e-3f &&
                        fabsf(link.limitMin.z) < 1e-3f && fabsf(link.limitMax.z) < 1e-3f);

                    if (isKnee)
                    {
                        // ■ 膝用：90度以上曲げたときの「ぱかぱか（裏返り）」を完全に防ぐ ■

                        // クォータニオンから直接 X軸の回転角度 を安全に抽出します（-180度～180度まで対応可能）
                        float xAngle = 2.0f * atan2f(qTmp.x, qTmp.w);

                        // 角度を -π ～ +π に正規化して、Clampの誤作動を防ぐ
                        while (xAngle > XM_PI)  xAngle -= XM_2PI;
                        while (xAngle < -XM_PI) xAngle += XM_2PI;

                        // MinとMaxが逆転していても安全にClampするための下準備
                        float limitMin = (std::min)(link.limitMin.x, link.limitMax.x);
                        float limitMax = (std::max)(link.limitMin.x, link.limitMax.x);

                        // 制限内に収める
                        xAngle = std::clamp(xAngle, limitMin, limitMax);

                        // 膝なので Y と Z は強制 0 にして、純粋な X軸回転 クォータニオンを作り直す
                        XMVECTOR qClamped = XMQuaternionRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), xAngle);
                        XMStoreFloat4(&bones[linkIdx].currentRotation, qClamped);
                    }
                    else
                    {
                        // ■ 足首・その他用：ZXY順 ■
                        // (足首は90度以上曲がらないので既存のままでOKです)
                        XMFLOAT3 euler = QuatToEulerZXY(qTmp);

                        euler.x = std::clamp(euler.x, link.limitMin.x, link.limitMax.x);
                        euler.y = std::clamp(euler.y, link.limitMin.y, link.limitMax.y);
                        euler.z = std::clamp(euler.z, link.limitMin.z, link.limitMax.z);

                        XMFLOAT4 qClamped = EulerZXYToQuat(euler);
                        XMStoreFloat4(&bones[linkIdx].currentRotation, XMLoadFloat4(&qClamped));
                    }
                }
                else
                {
                    XMStoreFloat4(&bones[linkIdx].currentRotation, qNew);
                }

                BuildGlobals(globals);
            }
        }
    }
}//================================================================
// モーフ（BoneMorph）  ※今は UpdateBoneMatrices からは呼んでいない
//================================================================
void PMXRender::ApplyMorphs()
{
    if (m_model->morphs.empty()) return;
    if (m_model->activeMorphWeights.empty()) return;

    auto& bones = const_cast<std::vector<PMXBone>&>(m_model->bones);

    auto findWeight = [&](const std::wstring& name)->float {
        auto it = m_model->activeMorphWeights.find(name);
        if (it == m_model->activeMorphWeights.end()) return 0.0f;
        return it->second;
        };

    for (const auto& morph : m_model->morphs)
    {
        if (morph.morphType != PMXMorphType::Bone) continue;

        float w = findWeight(morph.name);
        if (w <= 1e-4f) continue;

        for (const auto& bm : morph.boneMorph)
        {
            if ((int)bm.boneIndex < 0 || (int)bm.boneIndex >= (int)bones.size())
                continue;
            PMXBone& b = bones[bm.boneIndex];

            b.currentPosition.x += bm.position.x * w;
            b.currentPosition.y += bm.position.y * w;
            b.currentPosition.z += bm.position.z * w;

            XMVECTOR q0 = XMLoadFloat4(&b.currentRotation);
            XMVECTOR q1 = XMVectorSet(
                bm.quaternion.x, bm.quaternion.y,
                bm.quaternion.z, bm.quaternion.w);
            XMVECTOR q = XMQuaternionSlerp(q0, q1, w);
            XMStoreFloat4(&b.currentRotation, q);
        }
    }
}

//================================================================
// スキニング行列を GPU へ
//================================================================
void PMXRender::UploadSkinMatrices(const std::vector<XMMATRIX>& globals)
{
    // ※関数名は歴史的経緯でUploadだが、現在は「CPU側でスキニング行列を組み立てる」だけ。
    //   GPU（StructuredBuffer）への転送は Draw() 冒頭でシリアルに行う（並列更新対応のため）。
    const auto& bones = m_model->bones;

    for (size_t i = 0; i < bones.size(); ++i)
    {
        // PMXLoader が作った inverseBindPoseMatrix を利用
        XMMATRIX invBind = XMLoadFloat4x4(&bones[i].inverseBindPoseMatrix);

        // 「Tポーズからローカルに戻し(invBind)、現在位置へ持っていく(global)」
        // ※逆順(globals * invBind)にすると体が爆発・伸長するので注意
        XMMATRIX skin = invBind * globals[i];

        XMStoreFloat4x4(&m_boneMatrices[i], XMMatrixTranspose(skin));
    }
}

//================================================================
// ルート変換
//================================================================
void PMXRender::SetRootTransform(const DirectX::XMFLOAT3& pos,
    const DirectX::XMFLOAT4& rot)
{
    m_rootPos = pos;
    m_rootRot = rot;
    // 静的モデルでも位置が変わったらポーズの再計算・再転送が必要
    m_staticPoseUploaded = false;
}


// PMXRender.cpp に追加

// ---------------------------------------------------------
// モーフの再帰適用（グループモーフ対応）
// ---------------------------------------------------------
void PMXRender::ApplyMorphInternal(const PMXMorph& morph, float weight, std::vector<PMXVertexGPU>& destVerts)
{
    if (weight <= 0.001f) return;

    // --- グループモーフ ---
    if (morph.morphType == PMXMorphType::Group)
    {
        for (const auto& gm : morph.groupMorph)
        {
            // グループ内の対象モーフを探す
            if (gm.morphIndex < 0 || gm.morphIndex >= m_model->morphs.size()) continue;

            const auto& targetMorph = m_model->morphs[gm.morphIndex];
            // 「今のモーフのウェイト × グループ内での倍率」で再帰呼び出し
            ApplyMorphInternal(targetMorph, weight * gm.weight, destVerts);
        }
        return;
    }

    // --- 頂点モーフ (Position) ---
    if (morph.morphType == PMXMorphType::Position)
    {
        for (const auto& pm : morph.positionMorph)
        {
            if (pm.vertexIndex >= destVerts.size()) continue;

            // 元の頂点にオフセットを加算
            destVerts[pm.vertexIndex].pos[0] += pm.position.x * weight;
            destVerts[pm.vertexIndex].pos[1] += pm.position.y * weight;
            destVerts[pm.vertexIndex].pos[2] += pm.position.z * weight;
        }
    }

    // --- UVモーフ (UV) ---
    if (morph.morphType == PMXMorphType::UV)
    {
        for (const auto& um : morph.uvMorph)
        {
            if (um.vertexIndex >= destVerts.size()) continue;

            destVerts[um.vertexIndex].uv[0] += um.uv.x * weight;
            destVerts[um.vertexIndex].uv[1] += um.uv.y * weight;
            // z, w は使わないことが多いが PMX仕様的には vec4
        }
    }

    // ※ボーンモーフは UpdateBoneMatrices 側で処理しているのでここでは無視
    // ※マテリアルモーフはピクセルシェーダー定数でやるのが普通なのでここでは省略
}

// ---------------------------------------------------------
// モーフ関連の参照テーブル構築（初回のみ）
//   ・モーフ名 → インデックスのマップ（毎フレームの線形探索をなくす）
//   ・全モーフが影響する頂点の一覧と範囲（部分更新・部分転送のため）
// ---------------------------------------------------------
void PMXRender::CollectMorphVertices(const PMXMorph& morph, int depth)
{
    if (depth > 4) return; // グループモーフの循環参照ガード

    if (morph.morphType == PMXMorphType::Group) {
        for (const auto& gm : morph.groupMorph) {
            if (gm.morphIndex < 0 || gm.morphIndex >= (int)m_model->morphs.size()) continue;
            CollectMorphVertices(m_model->morphs[gm.morphIndex], depth + 1);
        }
        return;
    }
    if (morph.morphType == PMXMorphType::Position) {
        for (const auto& pm : morph.positionMorph) m_morphAffectedVerts.push_back(pm.vertexIndex);
    }
    if (morph.morphType == PMXMorphType::UV) {
        for (const auto& um : morph.uvMorph) m_morphAffectedVerts.push_back(um.vertexIndex);
    }
}

void PMXRender::BuildMorphLookups()
{
    if (m_morphLookupBuilt || !m_model) return;
    m_morphLookupBuilt = true;

    m_morphIndexByName.clear();
    m_morphAffectedVerts.clear();
    for (int i = 0; i < (int)m_model->morphs.size(); ++i) {
        m_morphIndexByName[m_model->morphs[i].name] = i;
        CollectMorphVertices(m_model->morphs[i], 0);
    }

    // 重複を除去して昇順に（範囲転送のためのmin/maxもここで確定）
    std::sort(m_morphAffectedVerts.begin(), m_morphAffectedVerts.end());
    m_morphAffectedVerts.erase(
        std::unique(m_morphAffectedVerts.begin(), m_morphAffectedVerts.end()),
        m_morphAffectedVerts.end());
    // 範囲外インデックスを除外
    while (!m_morphAffectedVerts.empty() &&
        m_morphAffectedVerts.back() >= m_originalVerticesGPU.size()) {
        m_morphAffectedVerts.pop_back();
    }
    if (!m_morphAffectedVerts.empty()) {
        m_morphMinVert = m_morphAffectedVerts.front();
        m_morphMaxVert = m_morphAffectedVerts.back();
    }
}

// ---------------------------------------------------------
// 頂点モーフの更新メイン
//   高速化のポイント：
//   1. モーフ値が前フレームから変わっていなければ何もしない（Dirtyチェック）
//   2. 変わっていても「モーフが影響する頂点」だけを復元・再計算する
//      （表情モーフは顔まわりの数千頂点だけ。全3万頂点は触らない）
//   3. GPUへの転送も影響範囲のみ・Draw側でシリアルに行う
// ---------------------------------------------------------
void PMXRender::UpdateVertices()
{
    if (!m_model || m_originalVerticesGPU.empty()) return;
    if (m_model->activeMorphWeights.empty()) return;

    // =========================================================
    // 1. モーフの値に変化があったか（Dirty判定）をチェック
    // =========================================================
    bool isDirty = false;
    for (const auto& pair : m_model->activeMorphWeights)
    {
        const std::wstring& name = pair.first;
        float currentWeight = pair.second;

        // 前回のウェイトを取得
        auto it = m_prevMorphWeights.find(name);
        float prevWeight = (it != m_prevMorphWeights.end()) ? it->second : 0.0f;

        // 差分が微小な誤差 (0.001f) 以上なら「変化あり」とみなす
        if (std::abs(currentWeight - prevWeight) > 0.001f) {
            isDirty = true;
            // 次回のために現在の値を保存
            m_prevMorphWeights[name] = currentWeight;
        }
    }

    // =========================================================
    // 2. 変化がない ＆ 初回実行でもないなら、ここで処理を完全スキップ！
    // =========================================================
    if (!isDirty && !m_forceMorphUpdate) {
        return;
    }
    m_forceMorphUpdate = false; // 初回フラグを折る

    // =========================================================
    // 3. 影響頂点だけを初期状態へ復元 → モーフを適用
    // =========================================================
    BuildMorphLookups();
    if (m_morphAffectedVerts.empty()) return; // 頂点/UVモーフが無いモデル

    // 作業バッファは「初期状態の完全コピー」を初回だけ作り、以降は影響頂点のみ書き換える
    if (m_morphVertScratch.empty()) {
        m_morphVertScratch = m_originalVerticesGPU;
    }
    for (uint32_t idx : m_morphAffectedVerts) {
        m_morphVertScratch[idx] = m_originalVerticesGPU[idx];
    }

    // 有効なモーフを適用（名前マップで直接引く）
    for (const auto& pair : m_model->activeMorphWeights)
    {
        if (pair.second <= 0.001f) continue;
        auto it = m_morphIndexByName.find(pair.first);
        if (it == m_morphIndexByName.end()) continue;
        ApplyMorphInternal(m_model->morphs[it->second], pair.second, m_morphVertScratch);
    }

    // 4. 転送はDraw側（メインスレッド）で影響範囲のみ行う
    m_vbDirty = true;
}

// ---------------------------------------------------------
// ---------------------------------------------------------
// マテリアルモーフの再帰適用
// ---------------------------------------------------------
void PMXRender::ApplyMaterialMorphInternal(const PMXMorph& morph, float weight, std::vector<DirectX::XMFLOAT4>& mulD, std::vector<DirectX::XMFLOAT4>& addD)
{
    if (weight <= 0.001f) return;

    if (morph.morphType == PMXMorphType::Group) {
        for (const auto& gm : morph.groupMorph) {
            if (gm.morphIndex < 0 || gm.morphIndex >= m_model->morphs.size()) continue;
            const auto& targetMorph = m_model->morphs[gm.morphIndex];
            ApplyMaterialMorphInternal(targetMorph, weight * gm.weight, mulD, addD);
        }
        return;
    }

    if (morph.morphType == PMXMorphType::Material) {
        for (const auto& mm : morph.materialMorph) {
            int matIdx = mm.materialIndex;
            int start = (matIdx == -1) ? 0 : matIdx;
            int end = (matIdx == -1) ? (int)m_matGpu.size() : matIdx + 1;

            for (int i = start; i < end; ++i) {
                if (i >= m_matGpu.size()) continue;

                if (static_cast<int>(mm.opType) == 0) { // 乗算 (Multiply)
                    mulD[i].x *= 1.0f + (mm.diffuse.x - 1.0f) * weight;
                    mulD[i].y *= 1.0f + (mm.diffuse.y - 1.0f) * weight;
                    mulD[i].z *= 1.0f + (mm.diffuse.z - 1.0f) * weight;
                    mulD[i].w *= 1.0f + (mm.diffuse.w - 1.0f) * weight;
                }
                else if (static_cast<int>(mm.opType) == 1) { // 加算 (Add)
                    // ★大修正: 過去の「Ambientも足す」という間違ったハックを完全削除！
                    // これが壁の色を真っ赤にバグらせていた真の元凶です！
                    addD[i].x += mm.diffuse.x * weight;
                    addD[i].y += mm.diffuse.y * weight;
                    addD[i].z += mm.diffuse.z * weight;
                    addD[i].w += mm.diffuse.w * weight;
                }
            }
        }
    }
}

// ---------------------------------------------------------
// マテリアルモーフの更新メイン（VMD連動・自動点灯）
// ---------------------------------------------------------
void PMXRender::UpdateMaterialMorphs()
{
    if (!m_model || m_matGpu.empty()) return;

    // スクラッチをリセットして使い回す（毎フレームのvector確保を回避）
    m_matMulScratch.assign(m_matGpu.size(), { 1.0f, 1.0f, 1.0f, 1.0f });
    m_matAddScratch.assign(m_matGpu.size(), { 0.0f, 0.0f, 0.0f, 0.0f });
    std::vector<DirectX::XMFLOAT4>& mulD = m_matMulScratch;
    std::vector<DirectX::XMFLOAT4>& addD = m_matAddScratch;

    BuildMorphLookups();
    for (const auto& pair : m_model->activeMorphWeights) {
        float weight = pair.second;
        if (weight <= 0.001f) continue;

        auto it = m_morphIndexByName.find(pair.first);
        if (it == m_morphIndexByName.end()) continue;
        ApplyMaterialMorphInternal(m_model->morphs[it->second], weight, mulD, addD);
    }

    for (size_t i = 0; i < m_matGpu.size(); ++i) {
        auto& mg = m_matGpu[i];
        const auto& src = m_model->materials[i];

        //変数にアクセスするだけ！
        bool isLightObject = mg.isLightObject;

        // ーーーーーーーーーーーーーーーーーーーーーーーーーー
        // ★ 2. ベース色の計算
        // ーーーーーーーーーーーーーーーーーーーーーーーーーー
        // MMDモデルは diffuse+ambient で本来の明るさになるので、全マテリアルでambientを足す。
        // （以前は isLightObject のみ加算していたため、肌・衣装などambient頼みのマテリアルが暗かった）
        // 上限は後段の1.0リミッターで抑えるので、ここではそのまま加算してよい。
        float bx = src.diffuse[0] + src.ambient[0];
        float by = src.diffuse[1] + src.ambient[1];
        float bz = src.diffuse[2] + src.ambient[2];
        float bw = src.diffuse[3];

        float finalX = (bx * mulD[i].x) + addD[i].x;
        float finalY = (by * mulD[i].y) + addD[i].y;
        float finalZ = (bz * mulD[i].z) + addD[i].z;
        float finalW = (bw * mulD[i].w) + addD[i].w;

        // 1.0リミッターでベース色の暴走を確実に防ぐ！
        finalX = (finalX > 1.0f) ? 1.0f : finalX;
        finalY = (finalY > 1.0f) ? 1.0f : finalY;
        finalZ = (finalZ > 1.0f) ? 1.0f : finalZ;
        // 色(RGB)だけでなく、透明度(W)も引き上げないと加算合成で消えちゃいます！
        if (isLightObject && !mg.isBody) {
            // 色が暗ければMAXにする
            if (finalX + finalY + finalZ < 0.1f) {
                finalX = 1.0f; finalY = 1.0f; finalZ = 1.0f;
            }
            // これがないと奥のライトが透明になって消滅しますが、
              // 「Spot(光の筋)」のグラデーション（透明度）を潰さないよう、Spotは除外する！
            if (!mg.isSpot && finalW < 0.1f) {
                finalW = 1.0f;
            }
        }

        mg.psData.diffuse = { finalX, finalY, finalZ, finalW };

        // ーーーーーーーーーーーーーーーーーーーーーーーーーー
        // ★ 3. 発光（Bloom）のパワー設定
        // ーーーーーーーーーーーーーーーーーーーーーーーーーー
        mg.psData.filterParams.w = 0.0f; // 基本は消灯

        // 🟢 修正後：用意しておいた数値を代入するだけ！
        if (mg.isLightObject && !mg.isBody && !mg.isScreen) {
            float brightness = finalX + finalY + finalZ;
            if (brightness > 0.05f) {
                mg.psData.filterParams.w = mg.baseEmission; // キャッシュしたパワーを代入
            }
        }
      
    }
}