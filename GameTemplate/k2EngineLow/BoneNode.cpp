#include "k2EngineLowPreCompile.h"
#include "BoneNode.h"
#include "pmxLoader.h"

using namespace DirectX;

namespace nsK2EngineLow {

    //BoneNode::BoneNode()
    //{
    //    XMStoreFloat4x4(&bindLocalM, XMMatrixIdentity());
    //    XMStoreFloat4x4(&localM, XMMatrixIdentity());
    //    XMStoreFloat4x4(&globalM, XMMatrixIdentity());
    //    bindLocalOffset = { 0,0,0 };
    //    deltaT = { 0,0,0 };
    //    deltaR = { 0,0,0,1 };
    //}

    // 行列の平行移動だけ取り出すヘルパ
    static XMFLOAT3 GetMatrixTranslation(const XMFLOAT4X4& m)
    {
        return XMFLOAT3(m._41, m._42, m._43);
    }

    //------------------------------------------------------------
    // PMXModel からノードツリーと bindLocalM を構築
    //------------------------------------------------------------
    void NodeManager::BuildFromPMXModel(const PMXModel& model)
    {
        const auto& bones = model.bones;
        const size_t n = bones.size();
        m_nodes.resize(n);
        m_globals.resize(n);

        if (n == 0) return;

        // 1) 親子と bindLocalM / bindLocalOffset を作る
        for (size_t i = 0; i < n; ++i)
        {
            const PMXBone& b = bones[i];
            BoneNode& node = m_nodes[i];

            node.pmxIndex = static_cast<int>(i);
            node.parentIndex = b.parentIndex;

            // PMXLoader::BuildBindPoseAndInverse で設定された bindPoseGlobal を利用
            XMMATRIX g = XMLoadFloat4x4(&b.bindPoseGlobal);
            XMMATRIX local;

            if (b.parentIndex >= 0) {
                const PMXBone& pb = bones[b.parentIndex];
                XMMATRIX gp = XMLoadFloat4x4(&pb.bindPoseGlobal);
                XMMATRIX invGp = XMMatrixInverse(nullptr, gp);
                local = invGp * g;          // local = parent^-1 * global
            }
            else {
                local = g;                   // root: global == local
            }

            XMStoreFloat4x4(&node.bindLocalM, local);
            XMStoreFloat4x4(&node.localM, local);
            XMStoreFloat4x4(&node.globalM, g);
            m_globals[i] = g;

            // ローカル位置だけ抜き出して保持（デバッグ用）
            node.bindLocalOffset = GetMatrixTranslation(node.bindLocalM);

            node.deltaT = { 0,0,0 };
            node.deltaR = { 0,0,0,1 };
        }

        // デバッグ：最初の数ボーンだけ確認
        for (size_t i = 0; i < n && i < 10; ++i)
        {
            const PMXBone& b = bones[i];
            const BoneNode& node = m_nodes[i];

           /* char buf[512];
            sprintf_s(
                buf,
                "[NodeCheck] i=%zu pmxIndex=%d parent(bone=%d,node=%d) "
                "bone.localOffset=(%.3f,%.3f,%.3f) "
                "bindLocalOffset(fromMatrix)=(%.3f,%.3f,%.3f)\n",
                i,
                node.pmxIndex,
                b.parentIndex,
                node.parentIndex,
                b.localOffset.x, b.localOffset.y, b.localOffset.z,
                node.bindLocalOffset.x, node.bindLocalOffset.y, node.bindLocalOffset.z
            );
            OutputDebugStringA(buf);*/

            //デバック出力を用意した関数に変更。
            DebugPrint("[NodeCheck] i=%zu pmxIndex=%d parent(bone=%d,node=%d) "
                "bone.localOffset=(%.3f,%.3f,%.3f) "
                "bindLocalOffset(fromMatrix)=(%.3f,%.3f,%.3f)\n",
                i,
                node.pmxIndex,
                b.parentIndex,
                node.parentIndex,
                b.localOffset.x, b.localOffset.y, b.localOffset.z,
                node.bindLocalOffset.x, node.bindLocalOffset.y, node.bindLocalOffset.z);
        }
    }

    //------------------------------------------------------------
    // 差分を全部リセット（Tポーズに戻す）
    //------------------------------------------------------------
    void NodeManager::ResetLocals()
    {
        for (auto& node : m_nodes) {
            node.deltaT = { 0,0,0 };
            node.deltaR = { 0,0,0,1 };
        }
    }

    //------------------------------------------------------------
    // そのボーンのローカル差分を設定
    //------------------------------------------------------------
    void NodeManager::SetLocal(
        int idx,
        const XMFLOAT3& t,
        const XMFLOAT4& r)
    {
        if (idx < 0 || idx >= static_cast<int>(m_nodes.size())) return;
        m_nodes[idx].deltaT = t;
        m_nodes[idx].deltaR = r;
    }

    //------------------------------------------------------------
    // 平行移動だけ加算
    //------------------------------------------------------------
    void NodeManager::AddLocalTranslation(int idx, const XMFLOAT3& add)
    {
        if (idx < 0 || idx >= static_cast<int>(m_nodes.size())) return;
        auto& t = m_nodes[idx].deltaT;
        t.x += add.x;
        t.y += add.y;
        t.z += add.z;
    }

    //------------------------------------------------------------
    // 回転だけ合成（左から乗算：qMul * current）
    //------------------------------------------------------------
    void NodeManager::MulLocalRotation(int idx, const XMFLOAT4& qMul)
    {
        if (idx < 0 || idx >= static_cast<int>(m_nodes.size())) return;

        XMVECTOR qCur = XMLoadFloat4(&m_nodes[idx].deltaR);
        XMVECTOR qAdd = XMLoadFloat4(&qMul);
        XMVECTOR q = XMQuaternionMultiply(qAdd, qCur); // 左から
        q = XMQuaternionNormalize(q);
        XMStoreFloat4(&m_nodes[idx].deltaR, q);
    }

    //------------------------------------------------------------
    // bindLocalM ＋ 差分からグローバル行列を再構築
    //------------------------------------------------------------
    void NodeManager::BuildGlobals()
    {
        const size_t n = m_nodes.size();
        m_globals.resize(n);

        for (size_t i = 0; i < n; ++i) {

            BoneNode& node = m_nodes[i];

            //// ---- 回転（deltaR）----
            //XMVECTOR q = XMLoadFloat4(&node.deltaR);
            //q = XMQuaternionNormalize(q);
            //XMMATRIX R = XMMatrixRotationQuaternion(q);

            //// ---- 平行移動（bind + deltaT）----
            //XMMATRIX T = XMMatrixTranslation(
            //    node.bindLocalOffset.x + node.deltaT.x,
            //    node.bindLocalOffset.y + node.deltaT.y,
            //    node.bindLocalOffset.z + node.deltaT.z
            //);

            //// ---- ローカル行列：R → T の順 ----
            //// 「回転」させてから「所定の位置（親からのオフセット）へ移動」
            //XMMATRIX local = R * T;

            // ✅ 修正版 — bindLocalM をベースにして delta を後乗算
            XMMATRIX bindLocal = XMLoadFloat4x4(&node.bindLocalM);

            // delta 回転
            XMVECTOR dQ = XMQuaternionNormalize(XMLoadFloat4(&node.deltaR));
            XMMATRIX dR = XMMatrixRotationQuaternion(dQ);

            // delta 平行移動
            XMMATRIX dT = XMMatrixTranslation(node.deltaT.x, node.deltaT.y, node.deltaT.z);

            // バインドポーズの上に差分を乗せる
            XMMATRIX local = bindLocal * dR * dT;

            // ---- 親のグローバルを乗算 ----
            XMMATRIX global;
            if (node.parentIndex >= 0)
                // 修正前（現在のコード）
                // global = m_globals[node.parentIndex] * local;

                // 修正後（正しい順序：Local * Parent）
                global = local * m_globals[node.parentIndex];
            else
                global = local;

            XMStoreFloat4x4(&node.localM, local);
            XMStoreFloat4x4(&node.globalM, global);
            m_globals[i] = global;
        }
    }


} // namespace nsK2EngineLow
