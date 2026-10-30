#pragma once
/**
 * @file   TitleUI.h
 * @brief  タイトル画面の2D表示。背景の画像・メニュー・クレジット・バージョンを描く。
 * @author Izumida Kiryu
 * @date   2026/10/29
 * @note   立たせるモデルなどの3D表示は TitleStage が受け持つ。こちらは2D表示と項目選びだけを受け持つ。
 */

#include "stdint.h"
#include "Src/UI/BackgroundSprite.h"

namespace nsApp
{
	namespace nsUI
	{
		/**
		 * @enum  EnTitleCommand
		 * @brief タイトルで決まった行き先。
		 */
		enum class EnTitleCommand : uint8_t
		{
			None,		//! まだ決まっていない。
			StartGame,	//! 本編を始める。
			Option,		//! 設定(キーコンフィグ)を開く。
			OpenGitHub,	//! リポジトリのページをブラウザで開く。
			ExitGame,	//! ゲームを終える。
			PlayDemo,	//! 動画を流す。メニューで選ぶものではなく、しばらく操作が無かったときに決まる。
		};


		/**
		 * @file   TitleUI.h
		 * @brief  タイトル画面の2D表示。
		 * @details 最初は「タイトル画像 + PRESS ANY KEY」だけを出し、何か押されたらメニューへ移る。
		 *          しばらく何も押されなければ、動画を流す行き先(PlayDemo)を決める。
		 *          どこへ行くかは決めず、決まった行き先を TitleScene に取りに来てもらう。
		 * @author Izumida Kiryu
		 * @date   2026/10/29
		 */
		class TitleUI : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			TitleUI() = default;
			virtual ~TitleUI() = default;


		public:
			/* ライフサイクル。*/
			/**
			 * @brief 初期化処理。
			 * @return 初期化に成功したらtrue。
			 */
			bool Start() override;

			/**
			 * @brief 更新処理。
			 */
			void Update() override;

			/**
			 * @brief 描画処理。
			 * @param rc レンダリングコンテキスト。
			 */
			void Render(RenderContext& rc) override;


		public:
			/**
			 * @brief 決まった行き先を取り出す。取り出すと未決定に戻る。
			 * @return 決まった行き先。決まっていなければ EnTitleCommand::None。
			 */
			EnTitleCommand PopDecidedCommand();

			/**
			 * @brief 入力の受け付けを切り替える。設定画面を開いている間は止める。
			 * @param bIsActive 受け付けるなら true。
			 */
			void SetInputActive(bool bIsActive);

			/**
			 * @brief 表示するかどうかを切り替える。動画を流している間は隠す。
			 * @param bIsVisible 表示するなら true。
			 */
			inline void SetVisible(bool bIsVisible)
			{
				/* 描くかどうかだけを切り替える。更新は止めない。*/
				bIsVisible_ = bIsVisible;
			}

			/**
			 * @brief PRESS ANY KEY の画面へ戻す。動画から戻ってきたときに使う。
			 */
			void ResetToPressAnyKey();

			/**
			 * @brief メニューを出しているかどうかを返す。
			 * @return メニュー中なら true。PRESS ANY KEY 中なら false。
			 */
			inline bool IsMenuPhase() const
			{
				/* 今の表示の段階を返す。*/
				return bIsMenuPhase_;
			}

			/**
			 * @brief 直前のフレームで、何かキーを押した瞬間だったかを返す。
			 * @return 押した瞬間なら true。
			 * @details 入力を受け付けていない間も毎フレーム調べている。動画を飛ばす判定に使う。
			 */
			inline bool IsAnyKeyTrigger() const
			{
				/* 前フレームとの差で見た「押した瞬間」を返す。*/
				return bIsAnyKeyTrigger_;
			}


		public:
			static constexpr int iMenuCount_ = 4;	//! メニューの行数。表示する文字の数と合わせること。


		private:
			/**
			 * @brief PRESS ANY KEY の点滅を進める。
			 */
			void UpdateBlink();

			/**
			 * @brief 操作が無い時間を数え、一定時間を超えたら動画を流す行き先を決める。
			 * @param bIsAnyKeyPress 今フレームで何か押されているか。
			 * @param fDeltaTime 前フレームからの時間(秒)。
			 */
			void UpdateIdle(bool bIsAnyKeyPress, float fDeltaTime);

			/**
			 * @brief 何か押されたらメニューへ移る。
			 * @param bIsAnyKeyPress 今フレームで何か押されているか。
			 */
			void UpdateAnyKeyPhase(bool bIsAnyKeyPress);

			/**
			 * @brief 上下で項目を選び、決定キーで行き先を決める。
			 * @param bIsPressUp 今フレームで上キーが押されているか。
			 * @param bIsPressDown 今フレームで下キーが押されているか。
			 */
			void UpdateMenuPhase(bool bIsPressUp, bool bIsPressDown);

			/**
			 * @brief メニューを出す段階へ移る。
			 */
			void EnterMenuPhase();

			/**
			 * @brief 選んでいる行だけ色を変える。
			 */
			void RefreshMenuColor();

			/**
			 * @brief 選んでいる行に敷く帯を、その行へ合わせる。
			 */
			void UpdateMenuCursor();


		private:
			BackgroundSprite stBackgroundSprite_;	//! 画面いっぱいに敷く背景の画像。モデルより奥に描く。
			FontRender stPressAnyKeyFont_;			//! PRESS ANY KEY の点滅表示。
			FontRender aMenuFont_[iMenuCount_];		//! メニューの各行。
			SpriteRender stMenuBarSprite_;			//! 選んでいる行に敷く帯。
			FontRender stCreditFont_;				//! クレジット(下の中央)。
			FontRender stVersionFont_;				//! バージョン(右下)。
			EnTitleCommand enDecidedCommand_ = EnTitleCommand::None;	//! 決まった行き先。取り出されるまで持つ。
			float fBlinkTimer_ = 0.0f;				//! PRESS ANY KEY を点滅させる時計(秒)。
			float fMenuTextHeight_ = 0.0f;			//! メニューの文字の実寸の高さ。帯を行に合わせるのに使う。
			float fInputGuardTimer_ = 0.0f;			//! 表示が切り替わった直後に誤って決定しないための猶予(秒)。
			float fIdleTimer_ = 0.0f;				//! 何も押されていない時間(秒)。動画を流すかどうかに使う。
			int iSelectedIndex_ = 0;				//! 今選んでいる行。
			bool bIsMenuPhase_ = false;				//! メニューを出しているか。
			bool bIsInputActive_ = true;			//! 入力を受け付けるか。
			bool bIsVisible_ = true;				//! 表示するか。動画を流している間は隠す。
			bool bIsAnyKeyTrigger_ = false;			//! このフレームで何かキーを押した瞬間だったか。
			bool bWasAnyKeyPress_ = false;			//! 前フレームで何か押されていたか。
			bool bWasPressUp_ = false;				//! 前フレームで上キーが押されていたか。
			bool bWasPressDown_ = false;			//! 前フレームで下キーが押されていたか。
		};
	}
}
