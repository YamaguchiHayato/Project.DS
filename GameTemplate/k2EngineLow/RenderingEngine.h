#pragma once
#include "Bloom.h"
#include "Light.h"
#include "IRender.h"
#include "Shadow.h"
#include "PMXRender.h"
#include "PenlightRender.h"


namespace nsK2EngineLow {

	class RenderingEngine :public Noncopyable
	{
	public:
		//初期化。
		void Init();
		//描画処理の実行。
		void Execute(RenderContext& rc);
		// レンダリングオブジェクトを追加する関数
		void AddRenderObject(IRender* renderObject)
		{
			m_renderObjects.push_back(renderObject);
		}
		//ディレクションライトの設定。
		void SetDirectionalLight(Vector3 direction,Vector3 color)
		{
			m_light.SetDirectionLight(direction,color);
		}

		//環境光の設定。
		void SetAmbientLight(Vector3 ambient)
		{
			m_light.SetAmbient(ambient);
		}

		//ポイントライトの設定
		void SetPointLight(int num, Vector3 position, Vector3 color, float range)
		{
			m_light.SetPointLight(num, position, color, range);
		}

		//スポットライトの設定。
		void SetSpotLight(int num, Vector3 position, Vector3 color, float range, Vector3 direction, float angle)
		{
			m_light.SetSpotLight(num, position, color, range, direction, angle);
		}
		//半球ライトの設定。
		void SetHemLight(Vector3 groundColor, Vector3 skyColor, Vector3 groundNormal)
		{
			m_light.SetHemLight(groundColor, skyColor, groundNormal);
		}

		void SetLightCameraTarget(Vector3 target)
		{
			m_light.SetLightCameraTarget(target);
		}

		RenderTarget& GetShadow()
		{
			return m_shadow.GetShadowTarget();
		}
		SceneLight& GetLight()
		{
			return m_light.GetLight();
		}

		Camera& GetLightCamera()
		{
			return m_light.GetLightCamera();
		}
		bool isResultFlag = false;//リザルト画面かどうか。
		bool isMotionBlurEnabled = true;//モーションブラーを適用するか（ホーム等で切りたい時にfalse）。

		// === PMX専用描画登録 ===
		void AddPMXObject(PMXRender* render) {
			m_pmxObjects.push_back(render);
		}
		// PMX専用描画登録解除
		void RemovePMXObject(PMXRender* obj);

		// === ペンライト（観客）描画登録 ===
		// PMX描画の直後・ブルーム前に描画される（発光がブルームに乗る）。nullptrで無効。
		void SetPenlightObject(PenlightRender* p) { m_penlight = p; }

		// メインレンダリングターゲットを取得（PSO作成などに使用）
		RenderTarget& GetMainRenderTarget() {
			return m_mainRenderingTarget;
		}

	private:
		void InitMainRenderTarget();
		void Init2DSprite();
		void InitFinalSprite();
		void InitMotionBlurSprite();
		//モデルの描画。
		void ModelDraw(RenderContext& rc);
		void SpriteFontDraw(RenderContext& rc);
		void CopyMainRenderTargetToFrameBuffer(RenderContext& rc);
		// PMX描画処理
		void PMXModelDraw(RenderContext& rc);
		
		Light m_light;
		Bloom m_bloom;
		Shadow m_shadow;
		
		RenderTarget m_mainRenderingTarget;
		RenderTarget m_2DRenderTarget;
		Sprite m_mainSprite;
		Sprite m_2DSprite;
		Sprite m_copyToframeBufferSprite;
			
		std::vector<IRender*> m_renderObjects;	// すべてのレンダリングオブジェクトを格納するリスト
		//// 追加
		std::vector<PMXRender*> m_pmxObjects;
		PenlightRender* m_penlight = nullptr; // 観客のペンライト（任意）

		//モーションブラー用変数。
		// モーションブラー用ベロシティバッファ
		RenderTarget m_velocityRenderTarget;

		RenderTarget m_tempRenderTarget; // 作業用バッファ
		Sprite m_motionBlurSprite;       // ブラー描画用
		Sprite m_copySprite;             // 書き戻し用
	};
}

