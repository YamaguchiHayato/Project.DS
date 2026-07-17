#include "stdafx.h"
#include "Game.h"
#include "GameCamera.h"
#include "RhythmGame.h"
#include "SelectMusical.h"
#include "SceneManager.h"


Game::Game() = default;

Game::~Game()
{
	DeleteGO(m_skyCube);
	DeleteGO(m_camera);
	DeleteGO(m_sceneManager);
}

bool Game::Start()
{

	NewGOStartGameObject();
	
	return true;
}
void Game::Update() {

}

void Game::NewGOStartGameObject()
{
	m_skyCube = NewGO<SkyCube>(0, "skyCube");
	m_skyCube->SetLuminance(1.0f);
	m_skyCube->SetScale(700.0f);//4000倍にすると描画がなくなるかも。
	m_camera = NewGO<GameCamera>(0, "camera");
	m_camera->Init();
	m_sceneManager = NewGO<SceneManager>(0, "SceneManager");
	m_sceneManager->Init();
}
