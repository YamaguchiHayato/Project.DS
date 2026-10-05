#pragma once
#include "Src/Actor/Character/Common/CharacterModel.h"

namespace nsApp
{
	namespace nsActor
	{
		class ICharacter;
		class Player;
	}

	namespace nsDirector
	{
		class IEnemyPool;

		/**
		 * @file   EnemySpawner.h
		 * @brief  配置点から敵をプール経由で出す係。見えない配置オブジェクト想定。
         * @details プールは所有しない。Director などから IEnemyPool を受け取る。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/05
		 */
		class EnemySpawner : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			EnemySpawner() = default;
			virtual ~EnemySpawner() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;


		public:
			/**
			 * @brief 使用するプールをセットする。
			 * @param pPool 使用するプールのポインタ。nullptr の場合はセットしない。
			 */
			inline void SetPool(IEnemyPool* pPool)
			{
				pPool_ = pPool;
			}

			/**
			 * @brief スポナーを生成する場所を設定する。
			 * @param vPos 生成する座標。
			 */
			inline void SetSpawnPosition(const Vector3& vPos)
			{
				vSpawnPos_ = vPos;
			}

			/**
			 * @brief スポナーから生成する敵を設定する。
			 * @param enType 敵モデルの種類。
			 */
			inline void SetEnemyType(CharacterModelType enType)
			{
				enType_ = enType;
			}

			/**
			 * @brief スポナーから生成する敵の数を設定する。
			 * @param count 生成数。
			 */
			inline void SetSpawnCount(int count)
			{
				iSpawnCount_ = count;
			}


		private:
			/**
			 * @brief 未出撃なら、設定数だけ Spawn する。
			 */
			void TrySpawn();


		private:
			IEnemyPool* pPool_ = nullptr; //! 使用するプール（非所有）。
			nsActor::Player* pTarget = nullptr; //! 追跡対象（非所有）。
			Vector3 vSpawnPos_ = Vector3::Zero; //! 出現位置。
			CharacterModelType enType_ = CharacterModelType::Common; //! 出す種類。
			int iSpawnCount_ = 1; //! 出す数。
			bool bSpawned_ = false; //! 一度出したか。
			int iSpawned_ = 0; //! すでに出した数。
		};
	}
}

