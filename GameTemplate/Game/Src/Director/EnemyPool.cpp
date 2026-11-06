#include "stdafx.h"
#include "EnemyPool.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Src/Actor/Character/Enemy/BomberEnemy.h"


namespace
{
	/**
	 * @brief 2点のXZ距離を返す。
	 * @param vA 一方の座標。
	 * @param vB もう一方の座標。
	 * @return XZ平面上の距離。
	 */
	float DistanceXZ(const Vector3& vA, const Vector3& vB)
	{
		const float fDx = vA.x - vB.x;
		const float fDz = vA.z - vB.z;
		return sqrtf(fDx * fDx + fDz * fDz);
	}
}


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


		bool EnemyPool::AreAllStarted() const
		{
			/* まだ Init していなければ準備できていない。*/
			if (!bInited_)
				return false;

			/* Common プールを順に見て、Start 前の個体があれば待つ。*/
			for (int i = 0; i < iPoolSize_; ++i)
			{
				if (pCommonPoolNum_[i] == nullptr || !pCommonPoolNum_[i]->IsStart())
					return false;
			}

			/* Bomber プールも同様。*/
			for (int i = 0; i < iBomberPoolSize_; ++i)
			{
				if (pBomberPoolNum_[i] == nullptr || !pBomberPoolNum_[i]->IsStart())
					return false;
			}

			return true;
		}


		int EnemyPool::CountNear(const Vector3& vCenter, float fRange) const
		{
			int iCount = 0; //! 付近の敵の数をカウントする変数。	

			/* Common: アクティブかつ範囲内だけ数える。*/
			for (int i = 0; i < iPoolSize_; ++i)
			{
				nsActor::CommonEnemy* pEnemy = pCommonPoolNum_[i];
				if (pEnemy == nullptr || pEnemy->IsPoolInactive())
					continue;
				if (DistanceXZ(pEnemy->GetPosition(), vCenter) > fRange)
					continue;
				++iCount;
			}

			/* Bomber: 同上。種類を分けず「付近の敵」としてまとめて数える。*/
			for (int i = 0; i < iBomberPoolSize_; ++i)
			{
				nsActor::BomberEnemy* pEnemy = pBomberPoolNum_[i];
				if (pEnemy == nullptr || pEnemy->IsPoolInactive())
					continue;
				if (DistanceXZ(pEnemy->GetPosition(), vCenter) > fRange)
					continue;
				++iCount;
			}

			return iCount;
		}


		bool EnemyPool::HasEnemyNear(const Vector3& vPos, float fClearDistance) const
		{
			/* 1体でも近ければ重なりあり。*/
			return CountNear(vPos, fClearDistance) > 0;
		}


		int EnemyPool::ActiveCount() const
		{
			int iCount = 0;

			/* Common の戦場に出ている数。*/
			for (int i = 0; i < iPoolSize_; ++i)
			{
				if (pCommonPoolNum_[i] == nullptr || pCommonPoolNum_[i]->IsPoolInactive())
					continue;
				++iCount;
			}

			/* Bomber も戦場に出ている分を足す。*/
			for (int i = 0; i < iBomberPoolSize_; ++i)
			{
				if (pBomberPoolNum_[i] == nullptr || pBomberPoolNum_[i]->IsPoolInactive())
					continue;
				++iCount;
			}

			return iCount;
		}
	}
}