
#pragma once

#include <string>

#include "Light.h"
#include "LightingConfig.h"

namespace nsK2EngineLow
{
    class LightingManager
    {
    public:
        // ライティング管理を初期化して、JSON設定を適用する
        bool Initialize(
            Light& light,
            const std::string& configPath,
            std::string& outError
        );

        // JSONを再読み込みして、ライティング設定を更新する
        bool Reload(std::string& outError);

        // 現在適用している設定を取得する
        const LightingConfig& GetConfig() const
        {
            return m_config;
        }

    private:
        // 設定値を既存のLightに反映する
        void ApplyConfig(
            Light& light,
            const LightingConfig& config
        );

    private:
        Light* m_light = nullptr;
        std::string m_configPath;
        LightingConfig m_config;
        bool m_initialized = false;
    };
}
