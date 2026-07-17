#include "PMXLoaderTestUtils.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "k2EngineLowPreCompile.h"

using namespace nsK2EngineLow;
using namespace DirectX;

// 行列を人間が見やすくログ出力
static void LogMatrix(const char* title, const XMFLOAT4X4& m)
{
    char buf[256];
    OutputDebugStringA(title);
    sprintf_s(buf, "  % .3f % .3f % .3f % .3f\n", m._11, m._12, m._13, m._14); OutputDebugStringA(buf);
    sprintf_s(buf, "  % .3f % .3f % .3f % .3f\n", m._21, m._22, m._23, m._24); OutputDebugStringA(buf);
    sprintf_s(buf, "  % .3f % .3f % .3f % .3f\n", m._31, m._32, m._33, m._34); OutputDebugStringA(buf);
    sprintf_s(buf, "  % .3f % .3f % .3f % .3f\n", m._41, m._42, m._43, m._44); OutputDebugStringA(buf);
}

// NaN / Inf 判定
static bool HasBadValue(const XMFLOAT4X4& m)
{
    const float* f = reinterpret_cast<const float*>(&m);
    for (int i = 0; i < 16; i++) {
        if (!std::isfinite(f[i])) return true;
    }
    return false;
}

// PMX のバインドポーズ検査
void TestPMXBindPose(const std::string& pmxPath)
{
    OutputDebugStringA("=== PMX BindPose Test BEGIN ===\n");

    PMXLoader loader;
    PMXModel model;

    if (!loader.LoadFromFile(pmxPath, model)) {
        OutputDebugStringA("PMX load failed.\n");
        return;
    }

    if (model.bones.empty()) {
        OutputDebugStringA("No bones.\n");
        return;
    }

    // 最初の10個だけチェック
    int count = std::min<int>((int)model.bones.size(), 10);

    for (int i = 0; i < count; i++) {
        const auto& b = model.bones[i];

        char head[256];
        sprintf_s(head, "Bone[%d] name=%s parent=%d\n",
            i, b.name.c_str(), b.parentIndex);
        OutputDebugStringA(head);

        LogMatrix("bindPoseGlobal:\n", b.bindPoseGlobal);
        if (HasBadValue(b.bindPoseGlobal))
            OutputDebugStringA("  !!! bindPose has NaN/Inf !!!\n");

        LogMatrix("inverseBindPose:\n", b.inverseBindPoseMatrix);
        if (HasBadValue(b.inverseBindPoseMatrix))
            OutputDebugStringA("  !!! inverseBindPose has NaN/Inf !!!\n");

        OutputDebugStringA("--------------------------------\n");
    }

    OutputDebugStringA("=== PMX BindPose Test END ===\n");
}
