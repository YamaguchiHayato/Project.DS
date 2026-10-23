
#include "k2EngineLowPreCompile.h"
#include "LightingManager.h"
#include "LightingConfigLoader.h"

namespace nsK2EngineLow
{
    namespace
    {
        // 設定データの配列をVector3に変換する
        Vector3 ToVector3(const LightingVector3& value)
        {
            return Vector3(
                value[0],
                value[1],
                value[2]
            );
        }
    }

    bool LightingManager::Initialize(
        Light& light,
        const std::string& configPath,
        std::string& outError)
    {
        // 先に設定を読み込み、成功した場合だけ適用する
        LightingConfig newConfig;

        if (!LoadLightingConfig(
            configPath,
            newConfig,
            outError))
        {
            return false;
        }

        m_light = &light;
        m_configPath = configPath;

        ApplyConfig(light, newConfig);

        m_config = newConfig;
        m_initialized = true;

        outError.clear();
        return true;
    }

    bool LightingManager::Reload(std::string& outError)
    {
        if (!m_initialized || m_light == nullptr)
        {
            outError = "LightingManagerが初期化されていません。";
            return false;
        }

        // 現在の設定を保持したまま、新しい設定を読み込む
        LightingConfig newConfig;

        if (!LoadLightingConfig(
            m_configPath,
            newConfig,
            outError))
        {
            // 読み込みに失敗したら現在の設定を維持する
            return false;
        }

        ApplyConfig(*m_light, newConfig);

        m_config = newConfig;

        outError.clear();
        return true;
    }

    void LightingManager::ApplyConfig(
        Light& light,
        const LightingConfig& config)
    {

        // ディレクショナルライト
        Vector3 direction =
            ToVector3(config.directionalLight.direction);

        Vector3 directionColor =
            ToVector3(config.directionalLight.color);

        if (!config.directionalLight.enabled)
        {
            directionColor = Vector3(0.0f, 0.0f, 0.0f);
        }

        light.SetDirectionLight(direction, directionColor);

        // アンビエントライト
        light.SetAmbient(
            ToVector3(config.ambientLight.color)
        );

        // 半球ライティング
        Vector3 groundColor =
            ToVector3(config.hemisphereLight.groundColor);

        Vector3 skyColor =
            ToVector3(config.hemisphereLight.skyColor);

        Vector3 groundNormal =
            ToVector3(config.hemisphereLight.groundNormal);

        if (!config.hemisphereLight.enabled)
        {
            groundColor = Vector3(0.0f, 0.0f, 0.0f);
            skyColor = Vector3(0.0f, 0.0f, 0.0f);
        }

        light.SetHemLight(
            groundColor,
            skyColor,
            groundNormal
        );

        // ポイントライトの未使用スロットを初期化する
        for (int i = 0; i < 10; ++i)
        {
            light.SetPointLight(
                i,
                Vector3(0.0f, 0.0f, 0.0f),
                Vector3(0.0f, 0.0f, 0.0f),
                0.0f
            );
        }

        // JSONに定義されたポイントライトを適用する
        for (int i = 0;
            i < static_cast<int>(config.pointLights.size());
            ++i)
        {
            const auto& point = config.pointLights[i];

            light.SetPointLight(
                i,
                ToVector3(point.position),
                ToVector3(point.color),
                point.range
            );
        }

        // スポットライトの未使用スロットを初期化する
        for (int i = 0; i < 10; ++i)
        {
            light.SetSpotLight(
                i,
                Vector3(0.0f, 0.0f, 0.0f),
                Vector3(0.0f, 0.0f, 0.0f),
                0.0f,
                Vector3(0.0f, -1.0f, 0.0f),
                0.0f
            );
        }

        // JSONに定義されたスポットライトを適用する
        for (int i = 0;
            i < static_cast<int>(config.spotLights.size());
            ++i)
        {
            const auto& spot = config.spotLights[i];

            light.SetSpotLight(
                i,
                ToVector3(spot.position),
                ToVector3(spot.color),
                spot.range,
                ToVector3(spot.direction),
                spot.angle
            );
        }
    }
}
