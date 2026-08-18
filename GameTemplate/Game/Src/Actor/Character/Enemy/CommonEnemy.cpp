#include "stdafx.h"
#include "CommonEnemy.h"

namespace
{
	const char* sUnityChanModelPath_ = "Assets/modelData/unityChan.tkm";	//! Unityちゃんのモデルパス。
}

namespace nsApp
{
	namespace nsActor
	{
		bool CommonEnemy::Start()
		{
			/* 仮モデルをロードする。*/
			stModelRender_.Init(sUnityChanModelPath_, nullptr, 0, enModelUpAxisY);
			stModelRender_.SetPosition(vPosition_);
			stModelRender_.Update();
			return true;
		}


		void CommonEnemy::Update()
		{
			/* 基底の更新。*/
			ICharacter::Update();

			/* 今は待機のみ。追跡は次。*/
			stModelRender_.SetPosition(vPosition_);
			stModelRender_.Update();
		}


		void CommonEnemy::Render(RenderContext& rc)
		{
			/* 仮モデルを描画する。*/
			stModelRender_.Draw(rc);
		}
	}
}