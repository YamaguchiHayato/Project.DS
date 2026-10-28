
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

    // 屋内ボリューム設定
    // この箱の中だけ、ベース光(方向光・環境光・半球光)を弱めて屋内らしくする。
    // ポイントライトとスポットライトは弱めないので、屋内では手持ちライトや
    // 据え置きの明かりだけが頼りになる。
    // 建物ごとに1つ書く。箱が重なった場合は、一番暗い倍率が採用される。
    struct IndoorVolumeConfig
    {
        bool enabled = true;

        // 箱の最小座標と最大座標(ワールド)。
        LightingVector3 min = {
            0.0f, 0.0f, 0.0f
        };

        LightingVector3 max = {
            0.0f, 0.0f, 0.0f
        };

        // 箱の中でのベース光の倍率。1.0で無効、小さいほど暗い。
        float scale = 1.0f;

        // 箱の端から内側へこの距離進むまでに、倍率が1.0からscaleへ滑らかに変わる。
        // 小さすぎると出入口でパッと切り替わるので、人の身長(175)より大きめにする。
        float feather = 300.0f;
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

        std::vector<IndoorVolumeConfig> indoorVolumes;
    };
}
