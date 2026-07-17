#pragma once
#include <vector>
#include <DirectXMath.h>

namespace nsK2EngineLow {

    struct PMXModel;
    struct PMXBone;

    /// ランタイム用ボーンノード（PMXBone とは別に、計算結果をここで管理）
    struct BoneNode {
        int pmxIndex = -1;      // 対応する PMXBone のインデックス
        int parentIndex = -1;   // 親ノード

        // Tポーズ時のローカル行列（bindPoseLocal）
        DirectX::XMFLOAT4X4 bindLocalM;
        // 上の行列から抜き出した Tポーズ時のローカル位置（デバッグ用）
        DirectX::XMFLOAT3   bindLocalOffset{ 0,0,0 };

        // VMD 等からの差分（ローカル）
        DirectX::XMFLOAT3   deltaT{ 0,0,0 };          // 差分平行移動
        DirectX::XMFLOAT4   deltaR{ 0,0,0,1 };        // 差分回転(クォータニオン)

        // 計算結果
        DirectX::XMFLOAT4X4 localM = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
        DirectX::XMFLOAT4X4 globalM = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

        BoneNode() = default; // .cpp側での実装を不要にする
    };

    /// PMXModel から作ったボーンツリー＋行列計算担当
    class NodeManager {
    public:
        /// PMXModel からノードツリーを作成（bindPoseLocal もここで復元）
        void BuildFromPMXModel(const PMXModel& model);

        /// 差分を全部 0/identity に戻す（Tポーズ基準に戻る）
        void ResetLocals();

        /// 「そのボーンのローカル差分」を設定（通常ボーン）
        void SetLocal(int nodeIndex,
            const DirectX::XMFLOAT3& t,
            const DirectX::XMFLOAT4& r);

        /// センター／グルーブ用の「加算系」
        void AddLocalTranslation(int nodeIndex, const DirectX::XMFLOAT3& add);
        void MulLocalRotation(int nodeIndex, const DirectX::XMFLOAT4& qMul);

        /// bindLocalM ＋ 差分からグローバル行列を構築
        void BuildGlobals();

        const std::vector<BoneNode>& GetNodes() const { return m_nodes; }
        /// PMXRender から使う用（グローバル行列一覧）
        const std::vector<DirectX::XMMATRIX>& GetGlobalMatrices() const { return m_globals; }

        // ★追加: 物理演算の結果でグローバル行列を上書きする
        void SetPhysicsMatrix(int boneIndex, const DirectX::XMMATRIX& m)
        {
            if (boneIndex >= 0 && boneIndex < (int)m_globals.size()) {
                m_globals[boneIndex] = m;

                // 必要なら BoneNode 側の globalM も更新
                XMStoreFloat4x4(&m_nodes[boneIndex].globalM, m);
            }
        }

    private:
        std::vector<BoneNode>             m_nodes;
        std::vector<DirectX::XMMATRIX>    m_globals;
    };

} // namespace nsK2EngineLow
