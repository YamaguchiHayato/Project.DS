
#pragma once

#include <array>
#include <string>
#include <vector>

namespace nsK2EngineLow
{
    // ライティング設定で使用する3次元ベクトル
    using LightingVector3 = std::array<float, 3>;

    // ディレクショナルライト
    struct DirectionalLightConfig
    {
        bool enabled = true;

        LightingVector3 direction = {
            0.0f, -1.0f, 1.0f
        };

        LightingVector3 color = {
            0.5f, 0.5f, 0.5f
        };
    };

    // アンビエントライト
    struct AmbientLightConfig
    {
        LightingVector3 color = {
            0.3f, 0.3f, 0.3f
        };
    };

    // 半球ライティング
    struct HemisphereLightConfig
    {
        bool enabled = true;

        LightingVector3 groundColor = {
            0.1f, 0.1f, 0.1f
        };

        LightingVector3 skyColor = {
            0.0f, 0.0f, 0.0f
        };

        LightingVector3 groundNormal = {
            0.0f, 1.0f, 0.0f
        };
    };

    // ポイントライト
    struct PointLightConfig
    {
        LightingVector3 position = {
            0.0f, 0.0f, 0.0f
        };

        LightingVector3 color = {
            1.0f, 1.0f, 1.0f
        };

        float range = 100.0f;
    };

    // スポットライト
    struct SpotLightConfig
    {
        LightingVector3 position = {
            0.0f, 0.0f, 0.0f
        };

        LightingVector3 color = {
            1.0f, 1.0f, 1.0f
        };

        float range = 100.0f;

        LightingVector3 direction = {
            0.0f, -1.0f, 0.0f
        };

        float angle = 30.0f;
    };

    // フォグ設定
    struct FogConfig
    {
        bool enabled = false;

        float start = 100.0f;
        float end = 400.0f;

        LightingVector3 color = {
            0.8f, 0.8f, 0.9f
        };
    };

    // 影設定
    struct ShadowConfig
    {
        bool enabled = true;
    };

    // ライティング設定全体
    struct LightingConfig
    {
        int version = 1;

        std::string presetName = "overcast";

        DirectionalLightConfig directionalLight;
        AmbientLightConfig ambientLight;
        HemisphereLightConfig hemisphereLight;

        std::vector<PointLightConfig> pointLights;
        std::vector<SpotLightConfig> spotLights;

        FogConfig fog;
        ShadowConfig shadows;
    };
}
