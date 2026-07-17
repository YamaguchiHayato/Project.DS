#include "k2EngineLowPreCompile.h"
#include "VMDAnimPlayer.h"
#include "pmxLoader.h"
#include "PMXRender.h"
#include <algorithm>

using namespace DirectX;
using namespace nsK2EngineLow;

std::wstring VMDAnimPlayer::Utf8ToWstring(const std::string& utf8)
{
    if (utf8.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return L"";
    std::wstring wide(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], wlen);
    if (!wide.empty() && wide.back() == L'\0') wide.pop_back();
    return wide;
}
// ==== デバッグ用：特定ボーンの deltaT / deltaR をダンプ ====
static void DumpDeltaTR(
    const PMXModel* model,
    const NodeManager& nodeMgr)
{
    if (!model) return;

    const auto& bones = model->bones;
    const auto& nodes = nodeMgr.GetNodes();

    // ★監視したいボーンのインデックス
    //  左肩4, 左腕5, 左ひじ7, 右肩30, 右腕31, 右ひじ33, 首56 あたりを見てみる
    const int watchIndices[] = { 4, 5, 7, 30, 31, 33, 56 };

    DebugPrint("==== DumpDeltaTR ====\n");
    for (int idx : watchIndices) {
        if (idx < 0 || idx >= (int)bones.size() || idx >= (int)nodes.size())
            continue;

        const auto& b = bones[idx];
        const auto& n = nodes[idx];

        DebugPrintW(
            L"[DeltaTR] bone %3d (%s)\n"
            "   deltaT = (%.3f, %.3f, %.3f)\n"
            "   deltaR = (%.3f, %.3f, %.3f, %.3f)\n",
            idx, b.name.c_str(),
            n.deltaT.x, n.deltaT.y, n.deltaT.z,
            n.deltaR.x, n.deltaR.y, n.deltaR.z, n.deltaR.w
        );
        
    }
}


template <typename T>
T clamp(const T& value, const T& minVal, const T& maxVal)
{
    return (value < minVal) ? minVal : (value > maxVal ? maxVal : value);
}

static bool JapanNameEquals(const std::string& a, const std::string& b)
{
    if (a == b) return true;
    auto normalize = [](const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == ' ' || c == '\t') continue;
            out.push_back((char)std::tolower((unsigned char)c));
        }
        return out;
        };
    return normalize(a) == normalize(b);
}

void VMDAnimPlayer::Init(PMXModel* model, VMDLoader* vmd)
{
    m_pmxModel = model;
    m_vmd = vmd;
    m_currentFrame = 0.0f;
    m_playSpeed = 1.0f;
    m_endFrame = 0.0f;

    if (m_pmxModel) {
        m_nodeMgr.BuildFromPMXModel(*m_pmxModel);
    }

    if (m_vmd) {
        for (auto& kv : m_vmd->GetBoneAnims()) {
            if (!kv.second.keyframes.empty())
                m_endFrame = (std::max)(m_endFrame, kv.second.keyframes.back().frame);
        }
    }

    BuildBoneTracks();

    BuildMorphTracks();
}

void VMDAnimPlayer::Update(float dt)
{
    if (!m_pmxModel || !m_vmd) return;

    // 外部クロック（音楽の再生時刻）で駆動している場合は、ここでは進めない。
    // SetCurrentFrame で毎フレーム正しい時刻がセットされるので二重加算を防ぐ。
    if (!m_useExternalClock) {
        m_currentFrame += dt * 30.0f * m_playSpeed;
        if (m_endFrame > 0.0f && m_currentFrame > m_endFrame)
            m_currentFrame = fmod(m_currentFrame, m_endFrame);
    }

    UpdateMorphs(m_currentFrame);

    m_nodeMgr.ResetLocals();
    auto& bones = m_pmxModel->bones;

    // Root
    XMFLOAT3 rootPos{ 0,0,0 };
    XMFLOAT4 rootRot{ 0,0,0,1 };
    if (m_rootTrackIdx >= 0) {
        SampleBone(*m_tracks[m_rootTrackIdx].anim, m_currentFrame, rootPos, rootRot);
    }

    // Center/Groove
    XMFLOAT3 centerPos{ 0,0,0 }, groovePos{ 0,0,0 };
    XMFLOAT4 centerRot{ 0,0,0,1 }, grooveRot{ 0,0,0,1 };

    if (m_centerTrackIdx >= 0)
        SampleBone(*m_tracks[m_centerTrackIdx].anim, m_currentFrame, centerPos, centerRot);

    if (m_grooveTrackIdx >= 0)
        SampleBone(*m_tracks[m_grooveTrackIdx].anim, m_currentFrame, groovePos, grooveRot);

    // Normal bones
    for (size_t ti = 0; ti < m_tracks.size(); ++ti) {
        if ((int)ti == m_rootTrackIdx || (int)ti == m_centerTrackIdx || (int)ti == m_grooveTrackIdx)
            continue;

        const auto& bt = m_tracks[ti];
        int bi = bt.pmxBoneIndex;
        if (bi < 0 || bi >= (int)bones.size()) continue;

        XMFLOAT3 pos{}; XMFLOAT4 rot{};
        SampleBone(*bt.anim, m_currentFrame, pos, rot);

        m_nodeMgr.SetLocal(bi, pos, rot);
    }

    // Apply center
    if (m_centerTrackIdx >= 0) {
        int ci = m_tracks[m_centerTrackIdx].pmxBoneIndex;
        if (ci >= 0 && ci < (int)bones.size()) {
            m_nodeMgr.AddLocalTranslation(ci, centerPos);
            m_nodeMgr.MulLocalRotation(ci, centerRot);
        }
    }

    // Apply groove to center
    if (m_grooveTrackIdx >= 0 && m_centerTrackIdx >= 0) {
        int ci = m_tracks[m_centerTrackIdx].pmxBoneIndex;
        if (ci >= 0 && ci < (int)bones.size()) {
            m_nodeMgr.AddLocalTranslation(ci, groovePos);
            m_nodeMgr.MulLocalRotation(ci, grooveRot);
        }
    }

    if (m_ownerRender) {
        m_ownerRender->SetRootTransform(rootPos, rootRot);
    }

    m_nodeMgr.BuildGlobals();
}


void VMDAnimPlayer::SampleBone(const VMDBoneAnim& anim, float frame,
    XMFLOAT3& outPos,
    XMFLOAT4& outRot) const
{
    const auto& kfs = anim.keyframes;
    if (kfs.empty()) {
        outPos = { 0,0,0 };
        outRot = { 0,0,0,1 };
        return;
    }
    if (kfs.size() == 1) {
        outPos = kfs[0].pos;
        outRot = kfs[0].rot;
        return;
    }

    // --- 2分探索で [prev, next] を取得 ---
    auto it = std::lower_bound(
        kfs.begin(), kfs.end(), frame,
        [](const auto& k, const auto& f) { // ★ auto& にして汎用性と速度を向上！
            return k.frame < f;
        });

    const VMDKeyFrame* prev = nullptr;
    const VMDKeyFrame* next = nullptr;

    if (it == kfs.begin()) {
        prev = &kfs[0];
        next = &kfs[1];
    }
    else if (it == kfs.end()) {
        prev = &kfs[kfs.size() - 2];
        next = &kfs[kfs.size() - 1];
    }
    else {
        next = &(*it);
        prev = &(*(it - 1));
    }

    float span = next->frame - prev->frame;
    if (span <= 0.0001f) {
        outPos = prev->pos;
        outRot = prev->rot;
        return;
    }

    float t = (frame - prev->frame) / span;
    t = clamp(t, 0.0f, 1.0f);

    // 座標: Bezier 補間
    float tx = EvalBezier(prev->interp[0], t);
    float ty = EvalBezier(prev->interp[1], t);
    float tz = EvalBezier(prev->interp[2], t);

    outPos.x = prev->pos.x + (next->pos.x - prev->pos.x) * tx;
    outPos.y = prev->pos.y + (next->pos.y - prev->pos.y) * ty;
    outPos.z = prev->pos.z + (next->pos.z - prev->pos.z) * tz;

    // 回転: Bezier + Slerp
    float tr = EvalBezier(prev->interp[3], t);
    XMVECTOR q = SlerpQuatSafe(prev->rot, next->rot, tr);
    XMStoreFloat4(&outRot, q);
}

float VMDAnimPlayer::EvalBezier(const uint8_t ctrl[4], float t) const
{
    // ctrl = { x1, y1, x2, y2 } (0..127)

    // ★高速化：線形カーブなら解く必要がない（y(x) = x になるので t をそのまま返す）。
    //   制御点が対角線上（x1==y1 かつ x2==y2）にあるカーブは数学的に恒等写像。
    //   VMDのキーフレームはデフォルト補間(20,20,107,107)＝線形が大半を占めるため、
    //   下の二分探索10回をほとんどのキーでスキップできる。
    if (ctrl[0] == ctrl[1] && ctrl[2] == ctrl[3]) {
        return t;
    }

    float x1 = ctrl[0] / 127.0f;
    float y1 = ctrl[1] / 127.0f;
    float x2 = ctrl[2] / 127.0f;
    float y2 = ctrl[3] / 127.0f;

    auto bezier = [](float s, float p0, float p1, float p2, float p3) {
        float inv = 1.0f - s;
        return inv * inv * inv * p0
            + 3.0f * inv * inv * s * p1
            + 3.0f * inv * s * s * p2
            + s * s * s * p3;
        };

    // x(s) を二分探索して t に近い s を求め、その s で y(s) を返す
    float sMin = 0.0f;
    float sMax = 1.0f;
    float s = t;

    for (int i = 0; i < 10; ++i) {
        float x = bezier(s, 0.0f, x1, x2, 1.0f);
        if (x > t) sMax = s;
        else       sMin = s;
        s = 0.5f * (sMin + sMax);
    }

    float y = bezier(s, 0.0f, y1, y2, 1.0f);
    return y;
}

XMVECTOR VMDAnimPlayer::SlerpQuatSafe(
    const XMFLOAT4& a,
    const XMFLOAT4& b,
    float t) const
{
    XMVECTOR qa = XMLoadFloat4(&a);
    XMVECTOR qb = XMLoadFloat4(&b);

    qa = XMQuaternionNormalize(qa);
    qb = XMQuaternionNormalize(qb);

    float dot = XMVectorGetX(XMVector4Dot(qa, qb));
    if (dot < 0.0f) {
        qb = XMVectorNegate(qb);
        dot = -dot;
    }

    // 計算誤差による NaN 爆発を完全に防ぐ
    if (dot >= 0.9995f) { // ★ >= に変更
        // 線形補間で十分（ほぼ回転していない）
        XMVECTOR q = XMVectorLerp(qa, qb, t);
        return XMQuaternionNormalize(q);
    }

    return XMQuaternionSlerp(qa, qb, t);
}


const VMDBoneAnim* VMDAnimPlayer::FindBoneAnimAny(
    std::initializer_list<const char*> names) const
{
    if (!m_vmd) return nullptr;
    const auto& map = m_vmd->GetBoneAnims();

    for (auto n : names) {
        auto it = map.find(n);
        if (it != map.end() && !it->second.keyframes.empty())
            return &it->second;
    }
    return nullptr;
}


void VMDAnimPlayer::BuildBoneTracks()
{
    m_tracks.clear();
    m_rootTrackIdx = -1;
    m_centerTrackIdx = -1;
    m_grooveTrackIdx = -1;

    if (!m_pmxModel || !m_vmd) return;

    const auto& boneNameMap = m_pmxModel->boneIndexByName;
    const auto& bones = m_pmxModel->bones;

    // ===== PMX 側 index を取得 =====
    int pmxRootIndex = -1;
    int pmxCenterIndex = -1;
    int pmxGrooveIndex = -1;

    auto findIndexByName = [&](std::initializer_list<const char*> names)->int {
        for (auto n : names) {
            auto it = boneNameMap.find(n);
            if (it != boneNameMap.end())
                return it->second;
        }
        return -1;
        };

    pmxRootIndex = findIndexByName({ "全ての親","全ての親 ","MotherBone" });
    pmxCenterIndex = findIndexByName({ "センター","center","Center","下半身","LowerBody" });
    pmxGrooveIndex = findIndexByName({ "グルーブ","Groove"/*,"上半身","UpperBody"*/ });

    // ===== VMD すべてのトラックを PMX ボーンに対応付け =====
    for (auto& kv : m_vmd->GetBoneAnims())
    {
        const std::string& vmdName = kv.first;
        const VMDBoneAnim& anim = kv.second;

        if (anim.keyframes.empty())
            continue;

        BoneTrack bt{};
        bt.anim = &anim;
        bt.pmxBoneIndex = -1;

        // ① PMX のボーン名で一致検索
        auto it = boneNameMap.find(vmdName);
        if (it != boneNameMap.end()) {
            bt.pmxBoneIndex = it->second;
        }
        else {
            // ② ゆるふわ一致
            for (size_t i = 0; i < bones.size(); ++i) {
                if (JapanNameEquals(bones[i].name, vmdName) ||
                    JapanNameEquals(bones[i].enName, vmdName))
                {
                    bt.pmxBoneIndex = (int)i;
                    break;
                }
            }
        }

        if (bt.pmxBoneIndex >= 0) {
            m_tracks.push_back(bt);
        }
    }

    // ===== root / center / groove の VMD トラックを割り当て =====

    // root = VMD の "センター"（Center）
    for (size_t i = 0; i < m_tracks.size(); i++) {
        const std::string& vname = m_tracks[i].anim->boneName;
        if (JapanNameEquals(vname, "センター") ||
            JapanNameEquals(vname, "Center") ||
            JapanNameEquals(vname, "center"))
        {
            // PMX "全ての親" を参照しているトラックだけ root に採用
            if (m_tracks[i].pmxBoneIndex == pmxRootIndex)
                m_rootTrackIdx = (int)i;
        }
    }

    // center = VMD "下半身"
    for (size_t i = 0; i < m_tracks.size(); i++) {
        const std::string& vname = m_tracks[i].anim->boneName;
        if (JapanNameEquals(vname, "下半身") ||
            JapanNameEquals(vname, "LowerBody"))
        {
            if (m_tracks[i].pmxBoneIndex == pmxCenterIndex)
                m_centerTrackIdx = (int)i;
        }
    }

    // groove = VMD "上半身"
    for (size_t i = 0; i < m_tracks.size(); i++) {
        const std::string& vname = m_tracks[i].anim->boneName;
        if (JapanNameEquals(vname, "上半身") ||
            JapanNameEquals(vname, "UpperBody"))
        {
            if (m_tracks[i].pmxBoneIndex == pmxGrooveIndex)
                m_grooveTrackIdx = (int)i;
        }
    }


    DebugPrintW(
        L"[VMDAnimPlayer] BuildBoneTracks: root=%d center=%d groove=%d (pmxRoot=%d pmxCenter=%d pmxGroove=%d)\n",
        m_rootTrackIdx, m_centerTrackIdx, m_grooveTrackIdx,
        pmxRootIndex, pmxCenterIndex, pmxGrooveIndex
	);
}


bool VMDAnimPlayer::MakeTRSMatrixForCenter(XMFLOAT4X4& outWorld) const
{
    const VMDBoneAnim* anim =
        FindBoneAnimAny({ "センター","center","Center" });
    if (!anim) return false;

    XMFLOAT3 pos;
    XMFLOAT4 rot;
    SampleBone(*anim, m_currentFrame, pos, rot);

    XMMATRIX S = XMMatrixScaling(5.0f, 5.0f, 5.0f);
    XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&rot));
    XMMATRIX T = XMMatrixTranslation(pos.x, pos.y - 10.0f, pos.z + 200.0f);

    XMStoreFloat4x4(&outWorld, XMMatrixTranspose(S * R * T));
    return true;
}


// ---------------------------------------------------------
// Init または BuildBoneTracks の最後あたりで呼ぶ
// ---------------------------------------------------------
void VMDAnimPlayer::BuildMorphTracks()
{
    m_morphTracks.clear();
    if (!m_pmxModel || !m_vmd) return;

    const auto& vmdMorphs = m_vmd->GetMorphAnims();

    // VMDにあるモーフ名が、PMX側に存在するかチェックして登録
    // ※PMXは wstringキー、VMDは stringキー なので変換が必要

    // 効率化のため PMX側のモーフ名一覧を一時的に保持してもいいが、
    // ここでは VMD の全モーフについて PMX にあるか確認する
    for (const auto& kv : vmdMorphs)
    {
        const std::string& vmdName = kv.first;
        const VMDMorphAnim& anim = kv.second;

        // UTF8 -> Wide
        std::wstring wName = Utf8ToWstring(vmdName);

        // PMXモデルの activeMorphWeights にキーがあるか？
        // (activeMorphWeights は PMXLoader::PostProcess で作られてるはず)
        // または単に PMXModel::morphs を検索してもOK
        // ここでは単純に「名前が一致するもの」を登録

        MorphTrack mt;
        mt.morphName = wName;
        mt.anim = &anim;
        m_morphTracks.push_back(mt);
    }
}


// ---------------------------------------------------------
// モーフ更新の実装
// ---------------------------------------------------------
void VMDAnimPlayer::UpdateMorphs(float frame)
{
    if (!m_pmxModel) return;

    // 一旦全リセット（VMDにないモーフは手動操作できるように 0 クリアしない手もあるが、
    // 基本は VMD 再生中は VMD が支配する）
    // ※手動と混ぜたい場合はここを工夫する必要あり
    for (auto& kv : m_pmxModel->activeMorphWeights) {
        kv.second = 0.0f;
    }

    for (const auto& track : m_morphTracks)
    {
        if (!track.anim || track.anim->keyframes.empty()) continue;

        const auto& kfs = track.anim->keyframes;
        float weight = 0.0f;

        // --- 2分探索でキーフレームを探して補間 ---
        // (SampleBoneと似たロジックですが、補間曲線はなく線形補間のみ)
        auto it = std::lower_bound(kfs.begin(), kfs.end(), frame,
            [](const auto& k, const auto& f) { return k.frame < f; });

        if (it == kfs.begin()) {
            weight = kfs.front().weight;
        }
        else if (it == kfs.end()) {
            weight = kfs.back().weight;
        }
        else {
            const auto& next = *it;
            const auto& prev = *(it - 1);
            float t = (frame - prev.frame) / (next.frame - prev.frame);
            // 線形補間 (Lerp)
            weight = prev.weight + (next.weight - prev.weight) * t;
        }

        // モデルに反映
        m_pmxModel->activeMorphWeights[track.morphName] = weight;
    }
}

// VMDAnimPlayer.cpp

// ★追加: カメラ更新関数
bool VMDAnimPlayer::EvaluateCamera(float frame, VMDCameraState& outState) const
{
    if (!m_vmd || m_vmd->GetCameraAnim().keyframes.empty()) return false;

    // 1. キーフレームから補間
    XMFLOAT3 targetPos; // 注視点
    XMFLOAT3 rotation;  // オイラー角
    float distance;
    float fov;

    SampleCameraInternal(frame, targetPos, rotation, distance, fov);

    // 2. カメラ行列の構築
    // MMDカメラの計算:
    // カメラ位置 = 注視点 - (回転行列 * (0, 0, 距離))
    // ※DirectX(左手系)用に調整

    // 回転行列作成 (Y-up 左手系)
    // MMDのRotationはラジアン
    XMMATRIX mRot = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);

    // 距離ベクトル (0, 0, -distance) を回転させる
    // ※MMDでは距離は「注視点からカメラへの距離」
    XMVECTOR vDist = XMVectorSet(0.0f, 0.0f, -distance, 0.0f);
    XMVECTOR vOffset = XMVector3TransformCoord(vDist, mRot);

    // 注視点
    XMVECTOR vFocus = XMLoadFloat3(&targetPos);

    // カメラ位置 (Eye) = 注視点 + オフセット
    XMVECTOR vEye = vFocus + vOffset; // 左手系なら + で手前に来る想定

    // アップベクトル (回転行列のY軸成分を取り出すとロールも反映される)
    XMVECTOR vUp = XMVectorSet(0, 1, 0, 0);
    vUp = XMVector3TransformNormal(vUp, mRot);

    XMStoreFloat3(&outState.eye, vEye);
    XMStoreFloat3(&outState.focus, vFocus);
    XMStoreFloat3(&outState.up, vUp);
    outState.fov = fov;
    outState.roll = rotation.z;

    return true;
}

// 補間ロジック (SampleBoneとほぼ同じ)
void VMDAnimPlayer::SampleCameraInternal(float frame,
    XMFLOAT3& outPos, XMFLOAT3& outRot, float& outDist, float& outFov) const
{
    const auto& kfs = m_vmd->GetCameraAnim().keyframes;

    // 範囲外チェックなどはSampleBone同様に行う（省略）
    // 2分探索
    auto it = std::lower_bound(kfs.begin(), kfs.end(), frame,
        [](const auto& k, const auto& f) { return k.frame < f; });

    const VMDCameraKeyFrame* prev = nullptr;
    const VMDCameraKeyFrame* next = nullptr;

    // (端っこ処理はSampleBoneと同じなので省略して、通常ケースのみ書きます)
    if (it == kfs.begin()) { prev = next = &kfs[0]; }
    else if (it == kfs.end()) { prev = next = &kfs.back(); }
    else { next = &(*it); prev = &(*(it - 1)); }

    float span = next->frame - prev->frame;
    if (span <= 0.0001f) {
        outPos = prev->pos; outRot = prev->rot;
        outDist = prev->distance; outFov = (float)prev->fov;
        return;
    }

    float t = (frame - prev->frame) / span;
    t = clamp(t, 0.0f, 1.0f);

    // 6要素それぞれベジェ補間 (X, Y, Z, Rot, Dist, FOV)
    // Rotは各軸まとめて補間係数が1つになっていることが多いが、MMD仕様上は軸独立ではない
    // が、VMDではRot用の補間カーブ(index=3)をXYZ全てに使うのが一般的

    float tX = EvalBezier(prev->interp[0], t);
    float tY = EvalBezier(prev->interp[1], t);
    float tZ = EvalBezier(prev->interp[2], t);
    float tR = EvalBezier(prev->interp[3], t); // 回転用
    float tD = EvalBezier(prev->interp[4], t); // 距離用
    float tF = EvalBezier(prev->interp[5], t); // FOV用

    // 線形補間(Lerp)にベジェ係数を適用
    auto Lerp = [](float a, float b, float w) { return a + (b - a) * w; };

    outPos.x = Lerp(prev->pos.x, next->pos.x, tX);
    outPos.y = Lerp(prev->pos.y, next->pos.y, tY);
    outPos.z = Lerp(prev->pos.z, next->pos.z, tZ);

    outRot.x = Lerp(prev->rot.x, next->rot.x, tR);
    outRot.y = Lerp(prev->rot.y, next->rot.y, tR);
    outRot.z = Lerp(prev->rot.z, next->rot.z, tR);

    outDist = Lerp(prev->distance, next->distance, tD);
    outFov = Lerp((float)prev->fov, (float)next->fov, tF);
}