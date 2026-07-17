#pragma once
class GameCamera:public IGameObject
{
public:
	GameCamera();
	~GameCamera();
	bool Init();
	bool Start();
	void Update();
	void SetTarget(Vector3 targetPos);
	void SetPosition(Vector3 pos);
	void RotateOriginTarget(Quaternion rot);
	/////////////////////////////////////
	//メンバ変数
	/////////////////////////////////////
	//Player* m_player = nullptr;	//プレイヤー。
	Vector3 m_toCameraPos = { 0.0f, 125.0f, -250.0f };	//注視点から視点に向かうベクトル。
	Vector3 m_fixedPos{ 1.0f,1000.0f,1.0f };
	Vector3 m_targetPos{ 0.0f,0.0f,0.0f };
};

