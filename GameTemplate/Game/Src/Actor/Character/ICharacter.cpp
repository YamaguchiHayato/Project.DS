#include "stdafx.h"
#include "ICharacter.h"

namespace
{
	const int HP_MIN = 0; //! HPの下限。
}


namespace nsApp
{
	namespace nsActor
	{
		void ICharacter::Update()
		{
			/* Actorクラスの更新。*/
			Actor::Update();
		}


		void ICharacter::Render(RenderContext& rc)
		{
			/* キャラクターを描画する。*/
			stModel_.DrawCharacterModel(rc);
		}
	}
}