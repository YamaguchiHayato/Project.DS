#include "pch.h"
#include "GraphicsSetting.h"
#include <fstream>
#include <string>

namespace nsApp
{
	namespace nsData
	{
		void GraphicsSetting::FillDefault()
		{
			fov_.FillDefault();
			sensitivity_ = 1.0f;
			brightness_ = 1.0f;
			windowWidth_ = 1980;
			windowHeight_ = 1080;
		}


		bool GraphicsSetting::Load()
		{
			/* 設定項目を反映するTSVファイルを読み込む。*/
			std::fstream file("Assets/data/graphicSetting.tsv");

			/* ファイルが開けなかった場合、デフォルトの値を設定する。*/
			if (!file.is_open())
			{
				
				FillDefault();
				return false;
			}

			/* 1行目は項目名なので読み飛ばす。*/
			std::string line; //! メンバ変数
			std::getline(file, line);

			while (std::getline(file, line))
			{
				/* タブ区切りで分割する。*/
				const std::size_t tab = line.find('\t');
				if (tab == std::string::npos)
					continue;

				/* 項目名と値を取得する。*/
				const std::string name = line.substr(0, tab);
				const std::string value = line.substr(tab + 1);

				/* 項目名に応じて値を設定する。*/
				if (name == "fov")
					fov_.Set(std::stof(value));
				else if (name == "sensitivity")
					sensitivity_ = std::stof(value);
				else if (name == "brightness")
					brightness_ = std::stof(value);
				else if (name == "windowWidth")
					windowWidth_ = static_cast<unsigned int>(std::stoul(value));
				else if (name == "windowHeight")
					windowHeight_ = static_cast<unsigned int>(std::stoul(value));
			}


			return true;
		}


		bool GraphicsSetting::Save()
		{
			/* 設定項目を反映するTSVファイルを保存する。*/
			std::ofstream file("Assets/data/graphicSetting.tsv");
			if (!file.is_open())
				return false;

			/* ヘッダー行を書き込む。*/
			file << "name\tvalue\n";
			file << "fov\t" << fov_.Get() << "\n";
			file << "sensitivity\t" << sensitivity_ << "\n";
			file << "brightness\t" << brightness_ << "\n";
			file << "windowWidth\t" << windowWidth_ << "\n";
			file << "windowHeight\t" << windowHeight_ << "\n";

			return true;
		}


		void GraphicsSetting::SetFOV(float fov)
		{
			fov_.Set(fov);
		}


		void GraphicsSetting::SetSensitivity(float sensitivity)
		{
			sensitivity_ = sensitivity;
		}


		void GraphicsSetting::SetBrightness(float brightness)
		{
			brightness_ = brightness;
		}


		void GraphicsSetting::SetWindowSize(unsigned int width, unsigned int height)
		{
			windowWidth_ = width;
			windowHeight_ = height;
		}
	}
}