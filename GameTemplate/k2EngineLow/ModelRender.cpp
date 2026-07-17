#include "k2EngineLowPreCompile.h"
#include "ModelRender.h"



void nsK2EngineLow::ModelRender::Init(const char* filePath, AnimationClip* animationClips, int numAnimationCrips, EnModelUpAxis enModelUpAxis, bool shadowCast, bool shadowdrop)
{
	// スケルトンを初期化。
	InitSkeleton(filePath);
	// アニメーションを初期化。
	InitAnimation(animationClips, numAnimationCrips, enModelUpAxis);
	//影描画するなら。
	if (shadowCast == true) {
		InitShadowCasterDrawing(filePath, enModelUpAxis);
	}
	InitModel(filePath,enModelUpAxis,shadowdrop);
}


void nsK2EngineLow::ModelRender::InitShadowCasterDrawing(const char* filePath, EnModelUpAxis enModelUpAxis)
{
	ModelInitData shadowInitData;
	shadowInitData.m_tkmFilePath = filePath;
	shadowInitData.m_fxFilePath = "Assets/shader/shadowMap.fx";
	shadowInitData.m_vsEntryPointFunc = "VSMain";
	shadowInitData.m_psEntryPointFunc = "PSShadowCaster";
	if (m_animationClips != nullptr) {
		shadowInitData.m_vsSkinEntryPointFunc = "VSSkinMain";
		shadowInitData.m_skeleton = &m_skeleton;
	}
	shadowInitData.m_colorBufferFormat[0] = DXGI_FORMAT_R32_FLOAT;
	shadowInitData.m_expandConstantBuffer = &m_alpha;
	shadowInitData.m_expandConstantBufferSize = sizeof(m_alpha);

	shadowInitData.m_modelUpAxis = enModelUpAxis;
	m_shadowModel.Init(shadowInitData);
}

void nsK2EngineLow::ModelRender::InitSkyCubeModel(ModelInitData& initData)
{
	m_model.Init(initData);
}

void nsK2EngineLow::ModelRender::InitSkeleton(const char* filePath)
{

	//スケルトンのデータを読み込み。
	std::string skeletonFilePath = filePath;
	int pos = (int)skeletonFilePath.find(".tkm");
	skeletonFilePath.replace(pos, 4, ".tks");
	m_skeleton.Init(skeletonFilePath.c_str());
}

void nsK2EngineLow::ModelRender::InitAnimation(AnimationClip* animationClips, int numAnimationClips, EnModelUpAxis enModelUpAxis)
{
	m_animationClips = animationClips;
	m_numAnimationClips = numAnimationClips;
	if (m_animationClips != nullptr) {
		m_animation.Init(m_skeleton,
			m_animationClips,
			numAnimationClips);
	}
}

void nsK2EngineLow::ModelRender::InitModel(const char* filePath, EnModelUpAxis enModelUpAxis,bool shadowReceiver)
{
	ModelInitData initData;
	//tkmファイルのファイルパスを指定する。
	initData.m_tkmFilePath = filePath;
	//シェーダーファイルのファイルパスを指定する。
	initData.m_fxFilePath = "Assets/shader/model.fx";
	//ノンスキンメッシュ用の頂点シェーダーのエントリーポイントを指定する。
	initData.m_vsEntryPointFunc = "VSMain";

	//影を描画するか？
	if (shadowReceiver==true) {
		//影用のシェーダー。
		initData.m_psEntryPointFunc = "PSShadowRecieverMain";
		//シャドウマップを取得。
		initData.m_expandShaderResoruceView[0] = &(g_renderingEngine->GetShadow().GetRenderTargetTexture());
	}
	else {
		//影を落とすモデルのシェーダーを指定する。
		initData.m_psEntryPointFunc = "PSNormalMain";
	}


	if (m_animationClips != nullptr) {
		//スキンメッシュ用の頂点シェーダーのエントリーポイントを指定。
		initData.m_vsSkinEntryPointFunc = "VSSkinMain";
		//スケルトンを指定する。
		initData.m_skeleton = &m_skeleton;
	}

	//モデルの上方向を指定する。
	initData.m_modelUpAxis = enModelUpAxis;
	initData.m_expandConstantBuffer = &g_renderingEngine->GetLight();
	initData.m_expandConstantBufferSize = sizeof(g_renderingEngine->GetLight());

	//作成した初期化データをもとにモデルを初期化する、
	m_model.Init(initData);
}
void nsK2EngineLow::ModelRender::Update()
{
	//スケルトンを更新。
	if (m_skeleton.IsInited())
	{
		m_skeleton.Update(m_model.GetWorldMatrix());
	}

	//モデルの更新。
	m_model.UpdateWorldMatrix(m_position, m_rotation, m_scale);
	//影のモデルに移動回転拡大を渡す
	m_shadowModel.UpdateWorldMatrix(m_position, m_rotation, m_scale);
	
	//アニメーションを進める。
	m_animation.Progress(g_gameTime->GetFrameDeltaTime());

}

void nsK2EngineLow::ModelRender::Draw(RenderContext& rc)
{
	g_renderingEngine->AddRenderObject(this);

}


float nsK2EngineLow::ModelRender::GetAnimationRatio()
{
	return m_animation.GetAnimationRatio();
}
//
//void nsK2EngineLow::ModelRender::OnDraw(RenderContext& rc) {
//	if (m_model.IsInited()) {
//		m_model.Draw(rc, 1);
//	}
//}
void nsK2EngineLow::ModelRender::OnRenderModel(RenderContext& rc)
{
	//初期化されていたら
	if (m_model.IsInited())
	{
		//モデルの描画
		m_model.Draw(rc, 1);
	}
}

void nsK2EngineLow::ModelRender::OnRenderShadowMap(RenderContext&rc,Camera&came) 
{
	if (m_shadowModel.IsInited())
	{
		m_shadowModel.Draw(rc, came, 1);

	}
}
