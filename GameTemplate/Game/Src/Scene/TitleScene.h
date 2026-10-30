#pragma once
/**
 * @file   TitleScene.h
 * @brief  タイトルシーン管理クラス。
 * @author Yamaguchi Hayato
 * @date   2026/08/18
 */

#include "Src/Scene/IScene.h"
#include "Src/UI/Title/TitleUI.h"

namespace nsApp
{
	namespace nsUI {
		class TitleStage;
		class KeyConfigUI;
		class VideoScreen;
	}

	namespace nsScene
	{
		class TitleScene : public IScene
		{
		public:
			/* コンストラクタとデストラクタ。*/
			TitleScene() = default;
			virtual ~TitleScene();


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override {}


		private:
			/**
			 * @brief 動画の再生中の更新。何かキーを押すか再生が終わったら、タイトルへ戻る。
			 * @return 動画の再生中なら true。
			 */
			bool UpdateVideo();

			/**
			 * @brief タイトルを隠して動画を流し始める。
			 */
			void StartDemoVideo();

			/**
			 * @brief 動画を止めて、PRESS ANY KEY の画面へ戻す。
			 */
			void StopDemoVideo();

			/**
			 * @brief 設定画面を開いている間の更新。閉じられたら破棄して入力を戻す。
			 * @return 設定画面を開いている間は true。
			 */
			bool UpdateKeyConfig();

			/**
			 * @brief タイトルの表示を生成する。
			 */
			void CreateTitle();

			/**
			 * @brief タイトルで決まった行き先を実行する。
			 * @param enCommand 決まった行き先。
			 */
			void ApplyTitleCommand(nsUI::EnTitleCommand enCommand);


		private:
			nsUI::TitleUI* pTitleUI_ = nullptr; //! タイトル本体。
			nsUI::TitleStage* pTitleStage_ = nullptr; //! タイトルに立たせるモデル。
			nsUI::KeyConfigUI* pKeyConfig_ = nullptr; //! キーコンフィグUI。
			nsUI::VideoScreen* pVideoScreen_ = nullptr; //! 動画の再生画面。タイトルでしばらく操作が無いときだけ作る。
		};
	}
}
