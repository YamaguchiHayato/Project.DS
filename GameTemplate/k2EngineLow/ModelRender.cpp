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
	m_expandCB.light = g_renderingEngine->GetLight();
	m_expandCB.modelAlpha = m_alpha;
	initData.m_expandConstantBuffer = &m_expandCB;
	initData.m_expandConstantBufferSize = sizeof(m_expandCB);
	initData.m_alphaBlendMode = AlphaBlendMode_Trans; // 半透明合成。

	//作成した初期化データをもとにモデルを初期化する、
	m_model.Init(initData);
}
void nsK2EngineLow::ModelRender::Update()
{
	/* シーンライトと、モデルのα値を、このモデル用のコピーへ写す。*/
	RefreshLight();

	m_model.UpdateWorldMatrix(m_position, m_rotation, m_scale);
	m_shadowModel.UpdateWorldMatrix(m_position, m_rotation, m_scale);

	if (m_animation.IsInited()) {
		m_animation.Progress(g_gameTime->GetFrameDeltaTime());
	}
	if (m_skeleton.IsInited()) {
		m_skeleton.Update(m_model.GetWorldMatrix());
	}
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
		//この時点の最新のライトを渡す(手持ちライトなど、毎フレーム変わる光のため)。
		RefreshLight();
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


void nsK2EngineLow::ModelRender::RefreshLight()
{
	m_expandCB.light = g_renderingEngine->GetLight();
	{
		auto& l = m_expandCB.light;
		const float s = m_baseLightScale;

		l.directionLig.m_color.x *= s;
		l.directionLig.m_color.y *= s;
		l.directionLig.m_color.z *= s;

		l.ambientLight.x *= s;
		l.ambientLight.y *= s;
		l.ambientLight.z *= s;

		l.hemiLig.m_groundColor.x *= s;
		l.hemiLig.m_groundColor.y *= s;
		l.hemiLig.m_groundColor.z *= s;
		l.hemiLig.m_skyColor.x *= s;
		l.hemiLig.m_skyColor.y *= s;
		l.hemiLig.m_skyColor.z *= s;
		// pointLig と spotLig は触らない(局所照明は弱めない)
	}

	// 屋内ボリュームはシーン全体で1つなので、毎フレームここで写す。
	// こうしておくと、床も壁もキャラも手に持った銃も同じ条件で暗くなる。
	{
		const auto& indoor = g_renderingEngine->GetIndoorVolume();

		m_expandCB.indoorMin = indoor.min;
		m_expandCB.indoorMax = indoor.max;
		m_expandCB.indoorScale = indoor.scale;
		m_expandCB.indoorFeather = indoor.feather;
	}

	m_expandCB.modelAlpha = m_alpha;
}