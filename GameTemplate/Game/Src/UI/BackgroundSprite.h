#pragma once
/**
 * @file   BackgroundSprite.h
 * @brief  モデルより奥に描く画像。
 * @author Izumida Kiryu
 * @date   2026/10/30
 * @note   SpriteRender との違い:
 *         SpriteRender は全てのモデルを描いたあとに重ねるので、モデルの後ろに画像を敷けない。
 *         こちらはメインのレンダリングターゲットへ、モデルより先に描く(IRender::OnRenderBackground2D)。
 *         重なり順は「この画像 → モデル → SpriteRender と FontRender」になる。
 */

namespace nsApp
{
	namespace nsUI
	{
		class BackgroundSprite : public IRender
		{
		public:
			/* コンストラクタとデストラクタ。*/
			BackgroundSprite() = default;
			virtual ~BackgroundSprite() = default;


		public:
			/**
			 * @brief 画像を読み込み、画面の中央に指定した大きさで置く。
			 * @param pFilePath DDS のパス。
			 * @param fWidth 表示する幅。
			 * @param fHeight 表示する高さ。
			 */
			void Init(const char* pFilePath, float fWidth, float fHeight);

			/**
			 * @brief 画像に掛ける色を設定する。暗くしたいときなどに使う。
			 * @param vMulColor 掛ける色(RGBA)。
			 */
			void SetMulColor(const Vector4& vMulColor);

			/**
			 * @brief このフレームで描くように登録する。SpriteRender と同じく毎フレーム呼ぶ。
			 * @param rc レンダリングコンテキスト。
			 */
			void Draw(RenderContext& rc);


		private:
			/**
			 * @brief モデルより先に、メインのレンダリングターゲットへ描く。
			 * @param rc レンダリングコンテキスト。
			 */
			void OnRenderBackground2D(RenderContext& rc) override;


		private:
			Sprite stSprite_;	//! 描く画像。
		};
	}
}
