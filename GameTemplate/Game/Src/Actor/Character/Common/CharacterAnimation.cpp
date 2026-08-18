#include "stdafx.h"
#include "CharacterAnimation.h"

namespace
{
	const int iCurrentIndexClear = 0;		//! 現在のインデックスをゼロで解放。
}

namespace nsApp
{
	void CharacterAnimation::Initialize()
	{
		/* アニメーション読み込む。*/
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
			/* 特定のアニメーションは再生ループをオフにする。*/
			if (pair.first == CharacterBasicAnimationList::Attack || pair.first == CharacterBasicAnimationList::Death)
			{
				/* 攻撃と死亡はループさせない。*/
				/* true だと ループ。*/
				bIsLoop_ = false;
			}
			else
			{
				/* それ以外はループするように。*/
				bIsLoop_ = true;
			}

			/* ループ方式を bIsLoop_ に任せる。*/
			mapBasicIndexMap_[pair.first] = SetAnimationClip(pair.second, bIsLoop_);
		}
	}
}