#include "stdafx.h"
#include "GameCamera.h"

GameCamera::GameCamera()
{
}


GameCamera::~GameCamera()
{
}
bool GameCamera::Init()
{
	//カメラのニアクリップとファークリップを設定する。
	g_camera3D->SetNear(1.0f);
	g_camera3D->SetFar(10000.0f);

	return true;
}
bool GameCamera::Start()
{
	//プレイヤーのインスタンスを探す。
	//m_player = FindGO<Player>("player");

	return true;
}
void GameCamera::Update()
{
}

void GameCamera::SetTarget(Vector3 targetPos)
{
	g_camera3D->SetTarget(targetPos);
	g_camera3D->Update();
}

void GameCamera::SetPosition(Vector3 pos)
{
	g_camera3D->SetPosition(pos);
	g_camera3D->Update();

}

void GameCamera::RotateOriginTarget(Quaternion rot)
{
	g_camera3D->RotateOriginTarget(rot);
	g_camera3D->Update();
}
