#include "pch.h"
#include "GraphicsSetting.h"
#include <cstdlib>
#include <fstream>
#include <string>

namespace
{
	const char* SETTING_PATH = "Assets/data/graphicSetting.tsv"; //! 設定ファイルの場所。
}

namespace nsApp
{
	namespace nsData
	{
		void GraphicsSetting::FillDefault()
		{
			camera_.FillDefault();
			control_.FillDefault();
			video_.FillDefault();
			sound_.FillDefault();
		}


		bool GraphicsSetting::Load()
		{
			/* 先にデフォルトの値を設定する。ファイルに無い項目はこのまま残る。*/
			FillDefault();

			/* 設定項目を反映するTSVファイルを読み込む。*/
			std::ifstream file(SETTING_PATH);

			/* ファイルが開けなかった場合、デフォルトの値のまま終わる。*/
			if (!file.is_open())
				return false;

			/* 1行目は項目名なので読み飛ばす。*/
			std::string line;
			std::getline(file, line);

			while (std::getline(file, line))
			{
				/* タブ区切りで分割する。*/
				const std::size_t tab = line.find('\t');
				if (tab == std::string::npos)
					continue;

				/* 項目名と値を取得する。stof は数字でない値で例外を投げるので、落ちない strtof を使う。*/
				const std::string name = line.substr(0, tab);
				const float value = std::strtof(line.c_str() + tab + 1, nullptr);

				/* 項目名を持っているカテゴリが受け取る。*/
				if (camera_.Read(name, value)) continue;
				if (control_.Read(name, value)) continue;
				if (video_.Read(name, value)) continue;
				sound_.Read(name, value);
			}

			return true;
		}


		bool GraphicsSetting::Save()
		{
			/* 設定項目を反映するTSVファイルを保存する。*/
			std::ofstream file(SETTING_PATH);
			if (!file.is_open())
				return false;

			/* ヘッダー行のあと、カテゴリごとに書き込む。*/
			file << "name\tvalue\n";
			camera_.Write(file);
			control_.Write(file);
			video_.Write(file);
			sound_.Write(file);

			return true;
		}


		void GraphicsSetting::SetFOV(float fov)
		{
			camera_.Set(CameraItem::Fov, fov);
		}


		void GraphicsSetting::SetSensitivity(float sensitivity)
		{
			control_.Set(ControlItem::Sensitivity, sensitivity);
		}


		void GraphicsSetting::SetBrightness(float brightness)
		{
			video_.Set(VideoItem::Brightness, brightness);
		}


		void GraphicsSetting::SetWindowSize(unsigned int width, unsigned int height)
		{
			video_.SetWindowSize(width, height);
		}


		float GraphicsSetting::GetValue(SettingCategory category, int index)const
		{
			/* カテゴリごとに項目の列挙が違うので、番号を各カテゴリの列挙に変えて読む。*/
			switch (category)
			{
			case SettingCategory::Camera:
				return camera_.Get(static_cast<CameraItem>(index));
			case SettingCategory::Control:
				return control_.Get(static_cast<ControlItem>(index));
			case SettingCategory::Video:
				return video_.Get(static_cast<VideoItem>(index));
			case SettingCategory::Sound:
				return sound_.Get(static_cast<SoundItem>(index));
			default:
				return 0.0f;
			}
		}


		void GraphicsSetting::SetValue(SettingCategory category, int index, float value)
		{
			/* カテゴリごとに項目の列挙が違うので、番号を各カテゴリの列挙に変えて入れる。*/
			switch (category)
			{
			case SettingCategory::Camera:
				camera_.Set(static_cast<CameraItem>(index), value);
				break;
			case SettingCategory::Control:
				control_.Set(static_cast<ControlItem>(index), value);
				break;
			case SettingCategory::Video:
				video_.Set(static_cast<VideoItem>(index), value);
				break;
			case SettingCategory::Sound:
				sound_.Set(static_cast<SoundItem>(index), value);
				break;
			default:
				break;
			}
		}


		const SettingRange& GraphicsSetting::GetRange(SettingCategory category, int index)const
		{
			/* カテゴリごとに項目の列挙が違うので、番号を各カテゴリの列挙に変えて引く。*/
			switch (category)
			{
			case SettingCategory::Control:
				return control_.GetRange(static_cast<ControlItem>(index));
			case SettingCategory::Video:
				return video_.GetRange(static_cast<VideoItem>(index));
			case SettingCategory::Sound:
				return sound_.GetRange(static_cast<SoundItem>(index));
			case SettingCategory::Camera:
			default:
				return camera_.GetRange(static_cast<CameraItem>(index));
			}
		}
	}
}