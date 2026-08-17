#pragma once
class Game:public IGameObject
{
public:
	Game();
	~Game();
	bool Start();
	void Update();
};

