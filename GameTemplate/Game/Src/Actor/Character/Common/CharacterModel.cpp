#include "stdafx.h"
#include "CharacterModel.h"

namespace nsApp
{
	CharacterModel::CharacterModel()
	{
		/* キャラモデルの格納。*/
	}


	bool CharacterModel::LoadCharacterModel(CharacterModelType characterType, AnimationClip* pAnimationClip, int iNumClips)
	{
		/* 読み込むモデルがリストの中にあるか確認。*/
		if (mapFilePathList_.count(characterType) == 0)
			return false; //! 登録されていないならロード失敗。

		/* IDに対応するモデルパスを取り出す。*/
		sModelFilePath_ = mapFilePathList_[characterType];

		/* モデルロードクラスを生成する。*/
		pCharacterModelRender_ = std::make_unique<ModelRender>();

		/* モデルをロードする。*/
		pCharacterModelRender_->Init(sModelFilePath_.c_str(),pAnimationClip,iNumClips,enModelUpAxisZ);

		return true;
	}


	void CharacterModel::DrawCharacterModel(RenderContext& rc)
	{
		/* 中身がある場合、描画する。*/
		if (pCharacterModelRender_ != nullptr)
			pCharacterModelRender_->Draw(rc);
	}


	void CharacterModel::Update()
	{
		/* キャラモデル本体が無いなら処理を終了させる。*/
		if (pCharacterModelRender_ == nullptr)
			return;

		/* モデルの更新処理。*/
		pCharacterModelRender_->Update();
	}
}