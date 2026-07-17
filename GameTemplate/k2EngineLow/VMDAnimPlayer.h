#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>
#include "VMDLoader.h"
#include "BoneNode.h"

namespace nsK2EngineLow {

    class PMXRender;
    class PMXModel;


    // ★追加: モーフ再生用のトラック情報
    struct MorphTrack {
        std::wstring morphName;       // PMX側の識別用(wstring)
        const VMDMorphAnim* anim = nullptr;
    };


    // ★追加: 計算結果のカメラ情報
    struct VMDCameraState {
        DirectX::XMFLOAT3 eye;    // カメラの位置
        DirectX::XMFLOAT3 focus;  // 注視点
        DirectX::XMFLOAT3 up;     // 上方向ベクトル
        float fov;                // 画角(度数法)
        float roll;               // ロール回転(ラジアン)
    };

    class VMDAnimPlayer {
    public:
        void Init(PMXModel* model, VMDLoader* vmd);
        void Update(float dt);
        // ★これを追加（PMXRenderからVMDのIKデータを見るため）
        const VMDLoader* GetVMDLoader() const { return m_vmd; }

        bool MakeTRSMatrixForCenter(DirectX::XMFLOAT4X4& outWorld) const;

        void SetPlaySpeed(float s) { m_playSpeed = s; }
        float GetCurrentFrame() const { return m_currentFrame; }
        float GetEndFrame() const { return m_endFrame; }
        bool  IsExternalClock() const { return m_useExternalClock; }
        // 外部（音楽の再生時刻）からフレームを直接指定する。
        // これを使うと Update(dt) の自前加算は止まり、オーディオクロックが唯一の基準になる。
        void SetCurrentFrame(float f) {
            // 最後まで行ったらループせず最終フレームで停止する
            if (m_endFrame > 0.0f && f > m_endFrame)
                f = m_endFrame;
            m_currentFrame = f;
            m_useExternalClock = true;
        }
        void SetOwner(PMXRender* owner) { m_ownerRender = owner; }

        const std::vector<DirectX::XMMATRIX>& GetGlobalMatrices() const {
            return m_nodeMgr.GetGlobalMatrices();
        }

        // ★これを追加：NodeManagerへの参照を返す
        const NodeManager& GetNodeManager() const { return m_nodeMgr; }


        // ★追加: 現在時刻のカメラ情報を計算する
        bool EvaluateCamera(float frame, VMDCameraState& outState) const;
    private:
        std::wstring Utf8ToWstring(const std::string& utf8);

        struct BoneTrack {
            int                 pmxBoneIndex = -1;
            const VMDBoneAnim* anim = nullptr;
            bool                isRoot = false;
        };

        void SampleBone(const VMDBoneAnim& anim, float frame,
            DirectX::XMFLOAT3& outPos,
            DirectX::XMFLOAT4& outRot) const;

        float EvalBezier(const uint8_t ctrl[4], float t) const;

        DirectX::XMVECTOR SlerpQuatSafe(const DirectX::XMFLOAT4& a,
            const DirectX::XMFLOAT4& b,
            float t) const;

        const VMDBoneAnim* FindBoneAnimAny(
            std::initializer_list<const char*> names) const;

        void BuildBoneTracks();

        // ★追加: カメラ用サンプリング(内部)
        void SampleCameraInternal(float frame,
            DirectX::XMFLOAT3& outPos, DirectX::XMFLOAT3& outRot,
            float& outDist, float& outFov) const;

    private:
        PMXModel* m_pmxModel = nullptr;
        VMDLoader* m_vmd = nullptr;
        PMXRender* m_ownerRender = nullptr;

        NodeManager m_nodeMgr;

        float m_currentFrame = 0.0f;
        float m_endFrame = 0.0f;
        float m_playSpeed = 1.0f;
        bool  m_useExternalClock = false; // true なら Update(dt) でフレームを進めない

        std::vector<BoneTrack> m_tracks;
        int m_rootTrackIdx = -1;
        int m_centerTrackIdx = -1;
        int m_grooveTrackIdx = -1;


        // ★追加関数
        void BuildMorphTracks();
        void UpdateMorphs(float frame);

        // ★追加メンバ
        std::vector<MorphTrack> m_morphTracks;
    };

} // namespace nsK2EngineLow