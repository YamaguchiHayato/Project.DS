#pragma once

namespace nsApp
{
	namespace nsUI
	{
		/**
		 * @file   SettingsUI.h
		 * @brief  設定画面。画像の上に文字を載せ、数値は GraphicSetting(別ソリューション) を呼ぶ。
		 * @author Yamaguchi Hayato
		 * @date   2026/11/02: 新規作成日。
		 */
		class SettingUI : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			SettingUI();
			virtual ~SettingUI();


		public:
			/* ライフサイクル。*/
			bool Start();
			void Update();
			void Render(RenderContext& rc);


		public:
			/**
			 * @brief 設定画面が閉じているか。
			 * @return その判定。
			 */
			inline bool IsClose()
			{
				return close_;
			}


		private:
			/**
			 * @brief Textureを読み込む。
			 */
			void LoadTexture();

			/**
			 * @brief 
			 */
			void InitSprite();


		private:
			struct Impl; //! 
			Impl* pImpl_ = nullptr; //!  
			bool close_; //! s設定画面が閉じているか。
		};
	}
}