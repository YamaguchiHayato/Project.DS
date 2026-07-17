#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <DirectXMath.h>
#include "pmxLoader.h"
#include "Graphics/RenderContext.h"
#include "PMXPhysics.h"
#include "time/Stopwatch.h"

namespace nsK2EngineLow {

    class VMDAnimPlayer;
    // GPUへ送る頂点構造体の定義をヘッダーに移動
    struct PMXVertexGPU {
        float    pos[3];      // 0
        float    normal[3];   // 12
        float    uv[2];       // 24
        uint16_t boneIdx[4];  // 32 (8 bytes)
        float    boneW[4];    // 40
    };
    class VertexBufferVRAM; // ★これを追加！
    class PMXRender
    {
    public:
        struct VSPerDraw {
            DirectX::XMFLOAT4X4 world;
            DirectX::XMFLOAT4X4 viewProj;
            DirectX::XMFLOAT4X4 oldWorldViewProj; // ★追加: 1フレ前の行列
            DirectX::XMFLOAT4 cameraPos;          // ★追加: xyz=カメラ世界座標(リム計算用), w=リム強度
        };

        struct PSMaterial {
            DirectX::XMFLOAT4 diffuse;
            DirectX::XMFLOAT4 filterParams; // ★追加: g_filterParams
        };

        struct MatGPU {
            PSMaterial psData{};
            int indexStart = 0;
            int indexCount = 0;
            Texture* albedo = nullptr;
            Texture* sphere = nullptr;
            Texture* toon = nullptr;

            ConstantBuffer* psCB = nullptr;
            DescriptorHeap* descHeap = nullptr;

            // ★★★ 奥義1：判定キャッシュ ★★★
            bool isBody = false;             // "Body" か？
            bool isSpot = false;             // "Spot" か？
            bool isScreen = false; // ★追加
            bool isLightObject = false;      // 光るオブジェクトか？
            bool isTransparentLight = false; // 加算合成で描画する透明な光か？
            float baseEmission = 0.0f;       // 発光の基本パワー（CIRCLEなら0.1、Spotなら8.0 など）
        };

    public:
        PMXRender() = default;
        ~PMXRender() { Destroy(); }

        bool Init(const PMXModel& model, DXGI_FORMAT rtvFmt, DXGI_FORMAT dsvFmt);

        // ★追加: 毎フレーム少しずつロードを進める関数
    // 戻り値: trueならロード継続中、falseならロード完了
        bool UpdateLoader();

        // ★追加: ロード中かどうか
        bool IsLoading() const { return m_isLoading; }

        // 次のUpdateLoader呼び出しが「物理初期化（重い）」になるか。
        // 呼び出し側はこれを見て物理初期化を1体/フレームに間引くと、
        // 複数モデルロード時にフレームが止まらない。
        bool IsPendingPhysicsInit() const {
            return m_isLoading && m_model
                && m_currentTextureIndex >= (int)m_model->materials.size()
                && m_physicsEnabled && !m_physics;
        }

        // =============================================================
        // ★テクスチャの先読み（ワーカースレッドから呼べる。ファイルI/Oのみ）
        //   モデルが参照する .dds を裏スレッドでメモリへ読み込んでおくと、
        //   UpdateLoader はディスクを読まずに「メモリ→GPU転送」だけになり、
        //   ロード中のメインスレッド停止（サークルが止まる原因）が大幅に減る。
        //   .dds 以外（tga等）はメモリロード非対応なので従来どおりファイルから読む。
        // =============================================================
        static void PreloadTextureBlobs(const PMXModel& model);

        // 先読みバッファの後始末（ロード完了時に呼ぶ。未消費分のメモリを解放）
        static void ClearTextureBlobCache();

        // =============================================================
        // ★CPUアニメーション更新（毎フレーム・Drawの前に呼ぶ）
        //   VMD再生・ボーン行列・IK・物理・モーフ計算などCPU処理だけを行う。
        //   GPUへの転送は一切しないため、複数モデルを【並列に】呼び出してよい
        //   （RenderingEngineが全PMXオブジェクトを並列更新する。モデル数が
        //     増えてもワーカースレッドに分散されFPSが落ちにくい）。
        //   呼び忘れても Draw が内部で1回だけ呼ぶので互換性は保たれる。
        // 【並列実行の前提】PMXModel/VMDLoaderをPMXRender間で共有しないこと
        //   （このプロジェクトはスロットごとに専用インスタンスなので満たしている）
        // =============================================================
        void UpdateAnimation();

        // 描画（GPU転送＋描画コマンド発行。必ずメインスレッドから呼ぶこと）
        void Draw(RenderContext& rc);
        void Destroy();

        /// VMD をアタッチ / デタッチ
        void AttachVMD(class VMDLoader* vmd);

        void SetRootTransform(const DirectX::XMFLOAT3& pos,
            const DirectX::XMFLOAT4& rot);

        // カメラVMDの有効/無効を切り替える関数
        void SetEnableCameraVMD(bool enable) { m_enableCameraVMD = enable; }

        // ★追加: 外部から明るさとボケを設定する関数
        void SetFilter(float brightness, float blur) {
            m_currentFilter = { brightness, blur, 0.0f, 0.0f };
        }

        void SetAnimPlaySpeed(float speed);

        /// 物理演算の状態をリセットする（リトライ時などに呼ぶ）
        void ResetPhysics() { m_physicsReset = true; m_physicsWarmupRemain = PHYSICS_WARMUP_STEPS; }

        /// 物理演算の有効/無効を設定する（後列キャラはオフにして軽量化）
        void SetPhysicsEnabled(bool enable) { m_physicsEnabled = enable; }

        /// リムライトの強さを設定する（0で無効）。キャラは1.0、ステージ/地面は0にする想定。
        void SetRimStrength(float s) { m_rimStrength = s; }

        /// 音楽経過時間（秒）でVMDフレームを同期する（VMDは30fps基準）
        void SyncToMusicTime(float musicTimeSec);

        /// デバッグ用：現在のVMDフレームと終端フレームを取得（VMDが無ければ-1）
        float GetVMDCurrentFrameDebug() const;
        float GetVMDEndFrameDebug() const;

    private:
        bool BuildRootSignature();
        bool BuildPipelineState(DXGI_FORMAT rtvFmt, DXGI_FORMAT dsvFmt);
        bool BuildBuffers(const PMXModel& model);
        bool BuildTextures(const PMXModel& model);

        /// 1フレーム分のボーン行列を更新して StructuredBuffer に流す
        void UpdateBoneMatrices();

        /// 付与親ボーンの影響を currentRotation／currentPosition に反映
        void ApplyGrant();

        /// currentRotation/currentPosition + offset からグローバル行列を構築
        void BuildGlobals(std::vector<DirectX::XMMATRIX>& globals) const;

        /// PMX の簡易 IK 解決（ひじ・ひざなど）
        void SolveIK(std::vector<DirectX::XMMATRIX>& globals, const std::unordered_map<std::string, bool>& ikStates);

        /// PMX の BoneMorph を current* に加算
        /// （重みは PMXModel::activeMorphWeights を参照）
        void ApplyMorphs();

        /// globals × inverseBindPose からスキニング用行列を作成し GPU にアップロード
        void UploadSkinMatrices(const std::vector<DirectX::XMMATRIX>& globals);

        static void EnsureFallbackTextures();



    private:
        const PMXModel* m_model = nullptr;
        bool m_validModel = false; // Initが空モデルで失敗した場合 false。Drawをスキップしてクラッシュを防ぐ

        // RootSignature / PSO は全PMXRenderで完全に同一（同じシェーダー・頂点レイアウト・出力フォーマット）。
        // インスタンスごとに生成するとPSO生成（DX12最重量級）とシェーダーCSO読込が11回走りロードが重くなるため、
        // static にして「最初の1体だけ生成→全員で共有」する。生成済みなら以降はスキップ。
        static RootSignature* s_rootSig;
        static PipelineState* s_pso;
        static PipelineState* s_psoAdd; // 光るポリゴン専用（加算合成）

        // 同一パスのテクスチャを使い回すためのキャッシュ（9体同一モデルで全テクスチャを9回ロードするのを防ぐ）
        static std::unordered_map<std::wstring, Texture*> s_textureCache;

        // ★テクスチャ先読みバッファ（パス→DDSファイルの生バイト列）。
        //   ワーカースレッドが書き、メインスレッド(UpdateLoader)が消費するためミューテックスで守る。
        static std::unordered_map<std::wstring, std::vector<char>> s_textureBlobCache;
        static std::mutex s_textureBlobMutex;

        VertexBuffer* m_vb = nullptr;
        VertexBufferVRAM* m_vbVRAM = nullptr; // [RHYTHM_GAME_MOD] ステージなどの静的モデル用
        IndexBuffer* m_ib = nullptr;
        ConstantBuffer* m_vsCB = nullptr;
        //ConstantBuffer* m_psCB = nullptr;

        std::vector<MatGPU> m_matGpu;

        std::vector<DirectX::XMFLOAT4X4> m_boneMatrices;
        std::unique_ptr<StructuredBuffer> m_boneMatrixSB;

        std::unique_ptr<VMDAnimPlayer> m_animPlayer;

        static std::unique_ptr<Texture> s_fallbackAlbedo;

        DirectX::XMFLOAT3 m_rootPos = { 0,0,0 };
        DirectX::XMFLOAT4 m_rootRot = { 0,0,0,1 };

    private:
        // ★追加: モーフ計算用の「変形前の初期状態」を保持する配列
    // BuildBuffers で作成した PMXVertexGPU をコピーして持っておく
        std::vector<PMXVertexGPU> m_originalVerticesGPU;

        // ★追加: モーフ更新関数
        void UpdateVertices();

        // ★追加: 再帰的にモーフを適用するためのヘルパー（グループモーフ用）
        void ApplyMorphInternal(const PMXMorph& morph, float weight, std::vector<PMXVertexGPU>& destVerts);

        // ★追加: マテリアルモーフ（照明）の更新用関数
        void UpdateMaterialMorphs();
        void ApplyMaterialMorphInternal(const nsK2EngineLow::PMXMorph& morph, float weight, std::vector<DirectX::XMFLOAT4>& mulD, std::vector<DirectX::XMFLOAT4>& addD);

    private:
        std::unique_ptr<PMXPhysics> m_physics; // 追加

    private:
        static constexpr int PHYSICS_WARMUP_STEPS = 120; // 総ウォームアップステップ数
        static constexpr int PHYSICS_WARMUP_PER_FRAME = 10; // 1フレームあたりの上限
        bool m_physicsReset = true;
        int  m_physicsWarmupRemain = PHYSICS_WARMUP_STEPS;
        bool m_physicsEnabled = true; // false にすると物理演算をスキップ

        float m_rimStrength = 1.0f; // リムライトの強さ（キャラ=1.0, ステージ/地面=0に設定）

        bool m_enableCameraVMD = true; // デフォルトは有効
        // ★追加: 現在のフィルター設定を保存する変数 (初期値: 明るさ1.0, ボケ0.0)
        DirectX::XMFLOAT4 m_currentFilter = { 1.0f, 0.0f, 0.0f, 0.0f };

        private:
            // 作業用変数
            bool m_isLoading = false;
            int  m_currentTextureIndex = 0; // 今何枚目を読んでいるか

    private:
        // モーションブラー用
        DirectX::XMFLOAT4X4 m_prevWorldViewProj; // 前回の行列
        bool m_isFirstFrame = true;              // 初回判定
        // モーションブラーをかけるかどうか (1.0 = ON, 0.0 = OFF)
        float m_motionBlurMaskVal = 1.0f; // デフォルトはON
        public:
        // この関数で OFF にできる
        void SetMotionBlurEnable(bool enable) {
             m_motionBlurMaskVal = enable ? 1.0f : 0.0f;
        }
        private:
        // ★追加: モーフのキャッシュと更新フラグ
        std::unordered_map<std::wstring, float> m_prevMorphWeights;
        bool m_forceMorphUpdate = true; // 初回は必ず初期化(転送)させるためのフラグ

    private:
        // =============================================================
        // ★パフォーマンス改善用のメンバ群
        //   ・毎フレームのヒープ確保をなくすためのスクラッチバッファ
        //   ・「CPU計算(UpdateAnimation) → GPU転送(Draw)」分離用のダーティフラグ
        // =============================================================
        bool m_cpuUpdated = false;               // このフレームのUpdateAnimation実行済みフラグ（Drawの互換フォールバック用）
        // スキニング行列の残り転送回数。
        // ★StructuredBufferはダブルバッファ（描画コマンドが1フレーム遅れて実行されるため）で、
        //   Updateは「現在のバックバッファ側」にしか書き込まない。1回だけ転送すると
        //   もう片面が初期値のままになり、静的モデルが偶数/奇数フレームで交互に
        //   正しい姿勢とTポーズを行き来してチカチカする。必ず両面（2回）転送すること。
        int m_skinDirtyFrames = 0;
        bool m_staticPoseUploaded = false;       // 静的モデル（VMDなし・物理なし）のポーズ転送済みフラグ。以降の更新を丸ごとスキップ

        std::vector<DirectX::XMMATRIX> m_globalsScratch;      // ボーングローバル行列の作業領域（毎フレームのvector確保を回避）
        std::vector<DirectX::XMFLOAT4> m_matMulScratch;       // マテリアルモーフ乗算の作業領域
        std::vector<DirectX::XMFLOAT4> m_matAddScratch;       // マテリアルモーフ加算の作業領域

        // 頂点モーフの部分更新用。
        // モーフが動く頂点は顔まわりなど全体のごく一部なので、影響頂点だけを
        // 復元・再計算し、GPUへも[min,max]の範囲だけ転送する。
        std::vector<PMXVertexGPU> m_morphVertScratch;         // 現在の頂点状態（初期状態のコピーを保持し、影響頂点のみ書き換える）
        std::vector<uint32_t> m_morphAffectedVerts;           // 全モーフが影響する頂点インデックス（重複なし・昇順）
        bool m_morphLookupBuilt = false;                      // 上記と名前マップの構築済みフラグ
        uint32_t m_morphMinVert = 0;                          // 影響範囲の最小頂点番号
        uint32_t m_morphMaxVert = 0;                          // 影響範囲の最大頂点番号
        bool m_vbDirty = false;                               // 頂点バッファの部分転送が必要か
        std::unordered_map<std::wstring, int> m_morphIndexByName; // モーフ名→インデックス（毎フレームの線形探索を回避）

        // カメラVMDの評価結果（UpdateAnimationで計算し、Drawでg_camera3Dへ適用する。
        // g_camera3Dはグローバルなので並列実行中に触ってはいけない）
        bool m_hasPendingCamera = false;
        DirectX::XMFLOAT3 m_pendingCamEye = { 0, 0, 0 };
        DirectX::XMFLOAT3 m_pendingCamFocus = { 0, 0, 0 };
        DirectX::XMFLOAT3 m_pendingCamUp = { 0, 1, 0 };

        // モーフ関連の参照テーブルを構築する（初回のみ）
        void BuildMorphLookups();
        // モーフが影響する頂点をmorphから再帰的に集める（グループモーフ対応）
        void CollectMorphVertices(const PMXMorph& morph, int depth);
    };
}
