#pragma once

#include <string>
#include "LightingConfig.h"

namespace nsK2EngineLow
{
    // JSONファイルからライティング設定を読み込む
    bool LoadLightingConfig(
        const std::string& path,
        LightingConfig& outConfig,
        std::string& outError
    );
}