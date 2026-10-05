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
		};
	}
}