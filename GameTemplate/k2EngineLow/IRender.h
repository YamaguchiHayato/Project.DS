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

		virtual void OnDraw(RenderContext& rc) {

		}

		virtual void OnShadowDraw(RenderContext&rc,Camera&came) {

		}

	};
}