
#include "k2EngineLowPreCompile.h"
#include "LightingConfigLoader.h"

#include <fstream>
#include <nlohmann/json.hpp>

namespace nsK2EngineLow
{
    using json = nlohmann::json;

    namespace
    {
        bool ReadVector3(
            const json& j,
            const char* key,
            LightingVector3& outValue)
        {
            if (!j.contains(key) || !j.at(key).is_array()
                || j.at(key).size() != 3)
            {
                return false;
            }

            for (int i = 0; i < 3; ++i)
            {
                if (!j.at(key).at(i).is_number())
                {
                    return false;
                }

                outValue[i] = j.at(key).at(i).get<float>();
            }

            return true;
        }

        bool ReadOptionalVector3(
            const json& j,
            const char* key,
            LightingVector3& outValue)
        {
            if (!j.contains(key))
            {
                return true;
            }

            return ReadVector3(j, key, outValue);
        }

        bool ReadOptionalBool(
            const json& j,
            const char* key,
            bool& outValue)
        {
            if (!j.contains(key))
            {
                return true;
            }

            if (!j.at(key).is_boolean())
            {
                return false;
            }

            outValue = j.at(key).get<bool>();
            return true;
        }

        bool ReadOptionalFloat(
            const json& j,
            const char* key,
            float& outValue)
        {
            if (!j.contains(key))
            {
                return true;
            }

            if (!j.at(key).is_number())
            {
                return false;
            }

            outValue = j.at(key).get<float>();
            return true;
        }

        bool ReadPointLights(
            const json& j,
            LightingConfig& config)
        {
            if (!j.contains("pointLights"))
            {
                return true;
            }

            if (!j.at("pointLights").is_array()
                || j.at("pointLights").size() > 10)
            {
                return false;
            }

            for (const auto& item : j.at("pointLights"))
            {
                PointLightConfig light;

                if (!ReadVector3(item, "position", light.position)
                    || !ReadVector3(item, "color", light.color)
                    || !ReadOptionalFloat(item, "range", light.range))
                {
                    return false;
                }

                config.pointLights.push_back(light);
            }

            return true;
        }

        bool ReadSpotLights(
            const json& j,
            LightingConfig& config)
        {
            if (!j.contains("spotLights"))
            {
                return true;
            }

            if (!j.at("spotLights").is_array()
                || j.at("spotLights").size() > 10)
            {
                return false;
            }

            for (const auto& item : j.at("spotLights"))
            {
                SpotLightConfig light;

                if (!ReadVector3(item, "position", light.position)
                    || !ReadVector3(item, "color", light.color)
                    || !ReadOptionalFloat(item, "range", light.range)
                    || !ReadOptionalFloat(item, "angle", light.angle)
                    || !ReadVector3(item, "direction", light.direction))
                {
                    return false;
                }

                config.spotLights.push_back(light);
            }

            return true;
        }
    }

    bool LoadLightingConfig(
        const std::string& path,
        LightingConfig& outConfig,
        std::string& outError)
    {
        try
        {
            std::ifstream file(path);

            if (!file.is_open())
            {
                outError = "ライティング設定ファイルを開けません: " + path;
                return false;
            }

            json j;
            file >> j;
            
            if (!j.is_object())
            {
                outError = "JSONのルートはオブジェクトである必要があります。";
                return false;
            }

            LightingConfig config;

            if (j.contains("version"))
            {
                if (!j.at("version").is_number_integer())
                {
                    outError = "version は整数で指定してください。";
                    return false;
                }

                config.version = j.at("version").get<int>();
            }

            if (j.contains("presetName"))
            {
                if (!j.at("presetName").is_string())
                {
                    outError = "presetName は文字列で指定してください。";
                    return false;
                }

                config.presetName = j.at("presetName").get<std::string>();
            }

            if (j.contains("directionalLight"))
            {
                const auto& d = j.at("directionalLight");

                if (!d.is_object()
                    || !ReadOptionalBool(d, "enabled",
                        config.directionalLight.enabled)
                    || !ReadOptionalVector3(d, "direction",
                        config.directionalLight.direction)
                    || !ReadOptionalVector3(d, "color",
                        config.directionalLight.color))
                {
                    outError = "directionalLight の設定が不正です。";
                    return false;
                }
            }

            if (j.contains("ambientLight"))
            {
                const auto& a = j.at("ambientLight");

                if (!a.is_object()
                    || !ReadOptionalVector3(a, "color",
                        config.ambientLight.color))
                {
                    outError = "ambientLight の設定が不正です。";
                    return false;
                }
            }

            if (j.contains("hemisphereLight"))
            {
                const auto& h = j.at("hemisphereLight");

                if (!h.is_object()
                    || !ReadOptionalBool(h, "enabled",
                        config.hemisphereLight.enabled)
                    || !ReadOptionalVector3(h, "groundColor",
                        config.hemisphereLight.groundColor)
                    || !ReadOptionalVector3(h, "skyColor",
                        config.hemisphereLight.skyColor)
                    || !ReadOptionalVector3(h, "groundNormal",
                        config.hemisphereLight.groundNormal))
                {
                    outError = "hemisphereLight の設定が不正です。";
                    return false;
                }
            }

            if (j.contains("fog"))
            {
                const auto& f = j.at("fog");

                if (!f.is_object()
                    || !ReadOptionalBool(f, "enabled",
                        config.fog.enabled)
                    || !ReadOptionalFloat(f, "start", config.fog.start)
                    || !ReadOptionalFloat(f, "end", config.fog.end)
                    || !ReadOptionalVector3(f, "color", config.fog.color))
                {
                    outError = "fog の設定が不正です。";
                    return false;
                }

                if (config.fog.end <= config.fog.start)
                {
                    outError = "fog.end は fog.start より大きくしてください。";
                    return false;
                }
            }

            if (j.contains("shadows"))
            {
                const auto& s = j.at("shadows");

                if (!s.is_object()
                    || !ReadOptionalBool(s, "enabled",
                        config.shadows.enabled))
                {
                    outError = "shadows の設定が不正です。";
                    return false;
                }
            }

            if (j.contains("indoorVolumes"))
            {
                const auto& volumes = j.at("indoorVolumes");

                if (!volumes.is_array())
                {
                    outError = "indoorVolumes は配列で指定してください。";
                    return false;
                }

                for (const auto& v : volumes)
                {
                    IndoorVolumeConfig volume;

                    if (!v.is_object()
                        || !ReadOptionalBool(v, "enabled", volume.enabled)
                        || !ReadOptionalVector3(v, "min", volume.min)
                        || !ReadOptionalVector3(v, "max", volume.max)
                        || !ReadOptionalFloat(v, "scale", volume.scale)
                        || !ReadOptionalFloat(v, "feather", volume.feather))
                    {
                        outError = "indoorVolumes の設定が不正です。";
                        return false;
                    }

                    // enabled が false の箱は読み飛ばす。
                    // 書いたまま残しておけるので、1つずつ消して確かめられる。
                    if (!volume.enabled)
                    {
                        continue;
                    }

                    config.indoorVolumes.push_back(volume);
                }
            }

            if (!ReadPointLights(j, config)
                || !ReadSpotLights(j, config))
            {
                outError = "pointLights または spotLights の設定が不正です。";
                return false;
            }

            outConfig = config;
            outError.clear();
            return true;
        }
        catch (const json::exception& e)
        {
            outError = std::string("JSONの読み込みに失敗しました: ")
                + e.what();
            return false;
        }
        catch (const std::exception& e)
        {
            outError = std::string("エラーが発生しました: ")
                + e.what();
            return false;
        }
    }
}
