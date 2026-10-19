#include "stdafx.h"
#include "EnemyPool.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Actor/Character/Enemy/BomberEnemy.h"


namespace nsApp
{
	namespace nsDirector 
	{
		void EnemyPool::Init()
		{
			/* すでに初期化済みなら何もしない。*/
			if (bInited_)
				return;

			/* プールのサイズ分、敵キャラクターを生成する。*/
			for (int i = 0; i < iPoolSize_; i++)
				pCommonPoolNum_[i] = NewGO<nsActor::CommonEnemy>(0, "commonEnemy");
			for (int i = 0; i < iBomberPoolSize_; i++)
				pBomberPoolNum_[i] = NewGO<nsActor::BomberEnemy>(0, "bomberEnemy");

			/* 初期化済みフラグを立てる。*/
			bInited_ = true;
		}


		nsActor::IEnemy* EnemyPool::Spawn(CharacterModelType enType, const Vector3& vPosition, nsActor::ICharacter* pTarget)
		{
			/* 未初期化の場合、nullptrを返す。*/
			if (!bInited_)
				return nullptr;

			/* プールから未使用の敵キャラクターを探す。*/
			if (enType == CharacterModelType::Common)
			{
				/* プールのサイズ分、敵キャラクターを探す。*/
				for (int i = 0; i < iPoolSize_; ++i)
				{
					/* プールの敵キャラクターが非アクティブであれば、生成する。*/
					if (pCommonPoolNum_[i] == nullptr || !pCommonPoolNum_[i]->IsPoolInactive())
						continue;

					/* プールから出して戦場に出す。*/
					pCommonPoolNum_[i]->Activate(vPosition, pTarget);
					return pCommonPoolNum_[i];
				}

				/* プールに空きがない場合、nullptrを返す。*/
				return nullptr;
			}

			/* ボンバーゾンビの生成。*/
			if (enType == CharacterModelType::Bomber)
			{
				/* プールのサイズ分、敵キャラクターを探す。*/
				for (int i = 0; i < iBomberPoolSize_; ++i)
				{
					/* プールの敵キャラクターが非アクティブであれば、生成する。*/
					if (pBomberPoolNum_[i] == nullptr || !pBomberPoolNum_[i]->IsPoolInactive())
						continue;

					/* プールから出して戦場に出す。*/
					pBomberPoolNum_[i]->Activate(vPosition, pTarget);
					return pBomberPoolNum_[i];
				}
				return nullptr;
			}

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