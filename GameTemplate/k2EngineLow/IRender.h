#pragma once

namespace nsK2EngineLow {
	/// <summary>
	/// レンダーラーの基底クラス。
	/// </summary>
	class IRender : public Noncopyable {
	public:
		virtual void OnRenderModel(RenderContext& rc)
		{

		}


		virtual void OnRenderShadowMap(RenderContext& rc,Camera& came) 
		{
		}

		virtual void OnRender2D(RenderContext& rc)
		{
		}

		/// <summary>
		/// モデルより奥に描く2D(背景の画像など)。
		/// メインのレンダリングターゲットをクリアした直後、モデルより先に描かれる。
		/// OnRender2D はモデルの上にかぶさるので、モデルの後ろに画像を敷きたいときはこちらを使う。
		/// </summary>
		virtual void OnRenderBackground2D(RenderContext& rc)
		{
		}

		virtual void OnDraw(RenderContext& rc) {

		}

		virtual void OnShadowDraw(RenderContext&rc,Camera&came) {

		}

	};
}