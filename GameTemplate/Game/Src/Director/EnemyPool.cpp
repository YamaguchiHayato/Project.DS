#include "stdafx.h"
#include "EnemyPool.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"

namespace nsApp
{
	namespace nsDirector 
	{
		void EnemyPool::Init()
		{
			/* 二重初期化を防ぐ。*/
			if (bInited_)
				return;

			/* 設定の数だけ生成を行う。*/
			/* todo 次のフェーズではパラメーターをTSV化する。*/
			for (int i = 0; i < iPoolSize_; i++)
			{
				/* CommonEnemyクラスを生成する。*/
				pCommonPoolNum_[i] = NewGO<nsActor::CommonEnemy>(0, "commonEnemy");
			}

			/* 初期化完了フラグをセット。*/
			bInited_ = true;
		}


		nsActor::IEnemy* EnemyPool::Spawn(CharacterModelType enType, const Vector3& vPosition, nsActor::ICharacter* pTarget)
		{
			/* 未初期化の場合、生成しない。*/
			if (!bInited_)
				return nullptr;

			/* 現段階ではCommonEnemyのみを生成する。*/
			if (enType != CharacterModelType::Common)
				return nullptr;

			/* プールから未使用の敵キャラクターを探す。*/
			for (int i = 0; i < iPoolSize_; ++i)
			{
				/* プールの敵キャラクターが非アクティブであれば使用可能。*/
				if (pCommonPoolNum_[i] == nullptr)
					continue;
				if (!pCommonPoolNum_[i]->IsPoolInactive())
					continue;

				/* 使用可能な敵キャラクターをアクティブ化して返す。*/
				pCommonPoolNum_[i]->Activate(vPosition, pTarget);
				return pCommonPoolNum_[i];
			}

			/* プールが満杯で生成できなかった場合は nullptr を返す。*/
			return nullptr; 
		}

		
		void EnemyPool::Release(nsActor::IEnemy* pEnemy)
		{
			/* 未初期化の場合、解放しない。*/
			if(pEnemy == nullptr)
				return;

			/* プールに戻す。*/
			if (pEnemy->IsPoolInactive())
				return;

			/* プールに戻す。*/
			pEnemy->Deactivate();
		}


		int EnemyPool::ActiveCount() const
		{
			int count = 0; //! 生成中の敵キャラクターの数をカウントする変数。

			/* 未初期化の場合、0を返す。*/
			for(int i = 0; i < iPoolSize_; ++i)
			{
				/* プールの敵キャラクターが非アクティブであればカウントしない。*/
				if (pCommonPoolNum_[i] == nullptr)
					continue;
				if (pCommonPoolNum_[i]->IsPoolInactive())
					continue;
				++count;
			}

			return count;
		}
	} 
}