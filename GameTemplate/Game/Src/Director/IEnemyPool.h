#pragma once
#include "Src/Actor/Character/Common/CharacterModel.h"

namespace nsApp
{
	namespace nsActor
	{
		class IEnemy;
		class ICharacter;
	}


	namespace nsDirector
	{
		class IEnemyPool
		{
		public:
			/* デストラクタ。*/
			virtual ~IEnemyPool() = default;


		public:
			/**
			 * @brief 敵キャラクターを生成する。
			 * @param enType 生成するモデルの種類。
			 * @param vPosition 生成位置。
			 * @param pTarget 追跡対象。
			 * @return　生成した敵キャラクターのポインタ。失敗した場合は nullptr を返す。
			 */
			virtual nsActor::IEnemy* Spawn(CharacterModelType enType, const Vector3& vPosition, nsActor::ICharacter* pTarget) = 0;

			/**
			 * @brief 生成対象のキャラクターの生成数を確保する。
			 */
			virtual void Init() = 0;

			/**
			 * @brief 敵キャラクターを解放する。
			 * @param pEnemy 解放する敵キャラクターのポインタ。
			 */
			virtual void Release(nsActor::IEnemy* pEnemy) = 0;

			/**
			 * @brief ステージ上に生成しているゾンビの数を取得する。
			 * @return 生成数。
			 */
			virtual int ActiveCount() const = 0;

			/**
			 * @brief プール内の敵が全員 Start を終えたか。
			 * @details FindGOs で世界を探さず、プールが持っている個体だけを見る。
			 * @return 全員終わっていれば true。未 Init なら false。
			 */
			virtual bool AreAllStarted() const = 0;

			/**
			 * @brief 中心から半径内にいるアクティブな敵の数を返す。
			 * @details XZ 平面の距離で判定する。プール待機中(IsPoolInactive)は数えない。
			 * @param vCenter 中心座標。
			 * @param fRange 半径(cm)。
			 * @return 付近の敵の数。
			 */
			virtual int CountNear(const Vector3& vCenter, float fRange) const = 0;

			/**
			 * @brief 指定位置の近くにアクティブな敵がいるか。
			 * @details 湧き位置の重なり避け用。CountNear が1以上なら true。
			 * @param vPos 調べる位置。
			 * @param fClearDistance この距離未満を「近い」とみなす(cm)。
			 * @return 近すぎる敵がいれば true。
			 */
			virtual bool HasEnemyNear(const Vector3& vPos, float fClearDistance) const = 0;
		};
	}
}