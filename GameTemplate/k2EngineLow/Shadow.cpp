#include "k2EngineLowPreCompile.h"
#include "Shadow.h"
namespace nsK2EngineLow
{
	void Shadow::Init()
	{
		// シャドウマップ用レンダリングターゲットの作成。
		shadowMapTarget.Create(
			1024,                      // 解像度（シャドウ品質）
			1024,
			1,
			1,
			DXGI_FORMAT_R32_FLOAT,
			DXGI_FORMAT_D32_FLOAT,    
			clearColor
		);
	}
	void Shadow::Execute(RenderContext& rc, std::vector<IRender*>& obj)
	{
		//ターゲットをシャドウマップに変更
		rc.WaitUntilToPossibleSetRenderTarget(shadowMapTarget);
		rc.SetRenderTargetAndViewport(shadowMapTarget);
		rc.ClearRenderTargetView(shadowMapTarget);

		// まとめて影モデルレンダーを描画
		for (auto MobjData :obj)
		{ 
			//影モデルの描画
			MobjData->OnRenderShadowMap(rc,g_renderingEngine->GetLightCamera());
		}
		rc.WaitUntilFinishDrawingToRenderTarget(shadowMapTarget);
	}
}
