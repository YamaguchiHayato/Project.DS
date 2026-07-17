#include "k2EngineLowPreCompile.h"
#include "Light.h"


void nsK2EngineLow::Light::Init()
{
	//ディレクションライトの初期座標の設定。
	m_SceneLight.directionLig.m_direction.x = 0.0f;
	m_SceneLight.directionLig.m_direction.y = -1.0f;
	m_SceneLight.directionLig.m_direction.z = 1.0f;

	//正規化する。
	m_SceneLight.directionLig.m_direction.Normalize();

	//ライトのカラーの設定。
	m_SceneLight.directionLig.m_color.x = 0.5f;
	m_SceneLight.directionLig.m_color.y = 0.5f;
	m_SceneLight.directionLig.m_color.z = 0.5f;

	//環境光。
	m_SceneLight.ambientLight.x = 0.3f;
	m_SceneLight.ambientLight.y = 0.3f;
	m_SceneLight.ambientLight.z = 0.3f;
	//視点の位置を設定する。
	m_SceneLight.eyePos = g_camera3D->GetPosition();

	



	//地面色。、天球色、地面の法線を追加する。
	m_SceneLight.hemiLig.m_groundColor = Vector3(0.1f, 0.1f, 0.1f);

	m_SceneLight.hemiLig.m_skyColor.x = 0.0f;
	m_SceneLight.hemiLig.m_skyColor.y = 0.0f;
	m_SceneLight.hemiLig.m_skyColor.z = 0.0f;

	m_SceneLight.hemiLig.m_groundNormal.x = 0.0f;
	m_SceneLight.hemiLig.m_groundNormal.y = 1.0f;
	m_SceneLight.hemiLig.m_groundNormal.z = 0.0f;


	//カメラの位置を設定
	m_lightCamera.SetPosition(GetLightCamera().GetTarget() + Vector3(0, 1500, -700));
	//カメラの注視点を設定
	m_lightCamera.SetTarget(GetLightCamera().GetTarget());
	//上方向を設定
	m_lightCamera.SetUp(1, 0, 0);
	//ライトビュープロジェクション行列の計算
	m_lightCamera.Update();
	m_SceneLight.mLVP = m_lightCamera.GetViewProjectionMatrix();
}

void nsK2EngineLow::Light::LightCameraUpdate()
{
	//カメラの位置を設定
	m_lightCamera.SetPosition(g_camera3D->GetTarget() + Vector3(0, 1500, -700));
	//カメラの注視点を設定
	m_lightCamera.SetTarget(g_camera3D->GetTarget());
	//ライトビュープロジェクション行列の計算
	m_lightCamera.Update();
	m_SceneLight.mLVP = m_lightCamera.GetViewProjectionMatrix();
}

