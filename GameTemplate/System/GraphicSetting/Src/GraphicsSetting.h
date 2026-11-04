#pragma once
#include "CameraSetting.h"
#include "ControlSetting.h"
#include "VideoSetting.h"
#include "SoundSetting.h"


namespace nsApp
{
	namespace nsData
	{
		/**
		 * @brief 設定のカテゴリ。設定画面の行が、どのカテゴリの値かを指すのに使う。
		 */
		enum class SettingCategory : unsigned char
		{
			Camera,		//! カメラ。
			Control,	//! 操作。
			Video,		//! グラフィックス。
			Sound,		//! サウンド。
			Count
		};

		/**
		 * @file GraphicsSetting.h
		 * @brief カメラ、操作、グラフィックス、サウンドの設定を所持。
		 * @details Get()を返す。GetFOV などは既存の呼び出し側のために残している。
		 * @author Yamaguchi Hayato
		 * @date 2026/11/04: 最終更新日。カテゴリごとのクラスに分けた。
		 */
		class GraphicsSetting
		{
		public:
			/**
			 * @brief 設定の実態を返す。
			 * @return Singleton。
			 */
			static GraphicsSetting& Get()
			{
				static GraphicsSetting instance;
				return instance;
			}

			/**
			 * @brief 起動時の値を返す。
			 */
			void FillDefault();

			/**
			 * @brief 設定用のTSVファイルを読み込む。
			 * @return 設定結果。
			 */
			bool Load();

			/**
			 * @brief 設定項目を保存する。
			 * @return 保存結果。
			 */
			bool Save();

			/**
			 * @brief 画角を設定する。
			 * @param fov 画角。
			 */
			void SetFOV(float fov);

			/**
			 * @brief マウス感度を設定する。
			 * @param sensitivity 感度。
			 */
			void SetSensitivity(float sensitivity);

			/**
			 * @brief 画面の明るさを設定する。
			 * @param brightness 明るさ。
			 */
			void SetBrightness(float brightness);

			/**
			 * @brief ウィンドウサイズを設定する。
			 * @param width 横幅。
			 * @param height 縦幅。
			 */
			void SetWindowSize(unsigned int width, unsigned int height);

			/**
			 * @brief カテゴリと番号で値を返す。設定画面の表から使う。
			 * @param category カテゴリ。
			 * @param index カテゴリ内の項目番号。
			 * @return 値。
			 */
			float GetValue(SettingCategory category, int index)const;

			/**
			 * @brief カテゴリと番号で値を入れる。範囲は各カテゴリの表で収める。
			 * @param category カテゴリ。
			 * @param index カテゴリ内の項目番号。
			 * @param value 入れたい値。
			 */
			void SetValue(SettingCategory category, int index, float value);

			/**
			 * @brief カテゴリと番号で範囲と初期値を返す。
			 * @param category カテゴリ。
			 * @param index カテゴリ内の項目番号。
			 * @return 範囲。
			 */
			const SettingRange& GetRange(SettingCategory category, int index)const;

			/**
			 * @brief 画角を返す。
			 * @return 画角。
			 */
			inline float GetFOV()const
			{
				return camera_.Get(CameraItem::Fov);
			}

			/**
			 * @brief マウス感度を返す。
			 * @return 感度。
			 */
			inline float GetSensitivity()const
			{
				return control_.Get(ControlItem::Sensitivity);
			}

			/**
			 * @brief 画面の明るさを返す。
			 * @return 明るさ。
			 */
			inline float GetBrightness()const
			{
				return video_.Get(VideoItem::Brightness);
			}

			/**
			 * @brief ウィンドウ幅を返す。
			 * @return ウィンドウ幅。
			 */
			inline unsigned int GetWindowWidth()const
			{
				return video_.GetWindowWidth();
			}

			/**
			 * @brief ウィンドウ高さを返す。
			 * @return ウィンドウ高さ。
			 */
			inline unsigned int GetWindowHeight()const
			{
				return video_.GetWindowHeight();
			}

			/**
			 * @brief カメラの設定を返す。
			 * @return カメラの設定。
			 */
			inline CameraSetting& GetCamera()
			{
				return camera_;
			}

			/**
			 * @brief 操作の設定を返す。
			 * @return 操作の設定。
			 */
			inline ControlSetting& GetControl()
			{
				return control_;
			}

			/**
			 * @brief グラフィックスの設定を返す。
			 * @return グラフィックスの設定。
			 */
			inline VideoSetting& GetVideo()
			{
				return video_;
			}

			/**
			 * @brief サウンドの設定を返す。
			 * @return サウンドの設定。
			 */
			inline SoundSetting& GetSound()
			{
				return sound_;
			}

			/**
			 * @brief カメラの設定を返す。読むだけの場合に使う。
			 * @return カメラの設定。
			 */
			inline const CameraSetting& GetCamera()const
			{
				return camera_;
			}

			/**
			 * @brief 操作の設定を返す。読むだけの場合に使う。
			 * @return 操作の設定。
			 */
			inline const ControlSetting& GetControl()const
			{
				return control_;
			}

			/**
			 * @brief グラフィックスの設定を返す。読むだけの場合に使う。
			 * @return グラフィックスの設定。
			 */
			inline const VideoSetting& GetVideo()const
			{
				return video_;
			}

			/**
			 * @brief サウンドの設定を返す。読むだけの場合に使う。
			 * @return サウンドの設定。
			 */
			inline const SoundSetting& GetSound()const
			{
				return sound_;
			}


		private:
			CameraSetting camera_; //! カメラの設定を管理。
			ControlSetting control_; //! 操作の設定を管理。
			VideoSetting video_; //! グラフィックスの設定を管理。
			SoundSetting sound_; //! サウンドの設定を管理。
		};
	}
}