#include "stdafx.h"
#include "CharacterAnimation.h"
#include "CharacterAnimBank.h"

namespace
{
	const int iCurrentIndexClear = 0; //! 現在のインデックスをゼロで解放。
}

namespace nsApp
{
	void CharacterAnimation::RegisterAnimation(ANIM_LIST state, const char* pFileStem, bool bIsLoop)
	{
		/* アニメーションのファイルパスを登録する。*/
		/* 拡張子は .tka 固定。*/
		mapBasicAnimationFilePathList_[state] = sBasicAnimationFilePath_ + pFileStem + ".tka";
		mapBasicLoopFlagList_[state] = bIsLoop;
	}


	void CharacterAnimation::Initialize(CharacterModelType characterType)
	{
		/* アニメーション読み込む前に箱をリセット。*/
		mapBasicAnimationFilePathList_.clear();
		mapBasicLoopFlagList_.clear();

		/* 種別 ID からアニメバンクを引く。*/
		const CharacterAnimBank* pBank = FindCharacterAnimBank(characterType);
		if (pBank == nullptr)
			return;

		/* 種別に応じた animData フォルダをセットする。*/
		sBasicAnimationFilePath_ = pBank->pBasePath;

		/* アニメ未用意の種別はエントリ0件のまま終了する。*/
		if (pBank->pEntries == nullptr || pBank->iEntryCount <= 0)
			return;

		/* バンクの分だけ RegisterAnimation を呼ぶ。*/
		for (int i = 0; i < pBank->iEntryCount; ++i)
		{
			const CharacterAnimEntry& entry = pBank->pEntries[i];
			RegisterAnimation(entry.state, entry.pFileStem, entry.bIsLoop);
		}
	}


	void CharacterAnimation::LoadAnimation()
	{
		/* アニメーションを読み込む前に箱をリセット。*/
		/* 基本アニメーションのマップを解放。*/
		mapBasicIndexMap_.clear();
		/* 現在のアニメーションを解放。*/
		iCurrentIndex_ = iCurrentIndexClear;

		/* 必要なアニメーションの合計数を計算する。*/
		iAnimationNum_ = static_cast<int>(mapBasicAnimationFilePathList_.size());

		/* 合計数と同じになるように配列を組む。*/
		pAnimationClipList_ = std::make_unique<AnimationClip[]>(iAnimationNum_);

		/* 基本動作をロード */
		for (auto& pair : mapBasicAnimationFilePathList_)
		{
			/* ループ設定は RegisterAnimation 時に保持した値を使う。*/
			const bool bIsLoop = mapBasicLoopFlagList_[pair.first];
			mapBasicIndexMap_[pair.first] = SetAnimationClip(pair.second, bIsLoop);
		}
	}
}