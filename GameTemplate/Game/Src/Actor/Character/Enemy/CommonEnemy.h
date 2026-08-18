#pragma once
/**
 * @file   CommonEnemy.h
 * @brief  雑魚敵。
 * @author Yamaguchi Hayato
 * @date   2026/08/18
 */

#include "Src/Actor/Character/ICharacter.h"

namespace nsApp
{
	namespace nsActor
	{
		class CommonEnemy : public ICharacter
		{
		public:
			/* コンストラクタとデストラクタ。*/
			CommonEnemy() = default;
			virtual ~CommonEnemy() = default;


		public:
			/* ライフサイクル。*/
			/**
			 * @brief 初期化処理。
			 * @return 初期化に成功したらtrue。
			 */
			bool Start() override;

			/**
			 * @brief 更新処理。
			 */
			void Update() override;

			/**
			 * @brief 描画処理。
			 * @param rc レンダリングコンテキスト。
			 */
			void Render(RenderContext& rc) override;


		public:
			/**
			 * @brief 現在位置を取得する。
			 * @return 現在位置。
			 */
			Vector3& GetPosition() override
			{
				return vPosition_;
			}

			/**
			 * @brief 追跡対象を設定する。
			 * @param pTarget 追跡対象。
			 */
			inline void SetTarget(ICharacter* pTarget)
			{
				/* 追跡対象を受け取る。*/
				pTarget_ = pTarget;
			}

			/**
	          * @brief すぐに攻撃できる状態にする。
               */
			inline void ReadyAttack()
			{
				/* 攻撃間隔を満たした扱いにする。*/
				fAttackTimer_ = fAttackInterval_;
			}

			/**
			 * @brief 追跡対象までの距離を取得する。
			 * @return 追跡対象までの距離。
			 */
			float GetDistanceToTarget();

			/**
			 * @brief 発見距離に入っているか。
			 * @return 入っていればtrue。
			 */
			bool IsTargetInDetectRange();

			/**
			 * @brief 攻撃距離に入っているか。
			 * @return 入っていればtrue。
			 */
			bool IsTargetInAttackRange();

			/**
			 * @brief 追跡対象へ移動する。
			 */
			void MoveToTarget();

			/**
			 * @brief 追跡対象を攻撃する。
			 */
			void AttackTarget();

			/**
			 * @brief 位置をモデルへ反映する。
			 */
			void ApplyModelTransform();


		private:
			/**
			 * @brief 対象への水平ベクトルを更新する。
			 */
			void UpdateToTargetVector();

			/**
			 * @brief 対象の方向を向く。
			 */
			void LookAtTarget();


		private:
			ModelRender stModelRender_;					//! 仮モデル。
			ICharacter* pTarget_ = nullptr;				//! 追跡対象。
			Vector3 vPosition_ = { 50.0f, 0.0f, 0.0f };	//! 現在位置。
			Vector3 vToTarget_;							//! 対象への水平ベクトル。
			Quaternion qLook_;							//! 対象方向の回転。
			float fDetectRange_ = 250.0f;				//! 発見距離。
			float fChaseSpeed_ = 120.0f;				//! 追跡速度。
			float fAttackRange_ = 120.0f;				//! 攻撃距離。
			float fAttackInterval_ = 1.0f;				//! 攻撃間隔。
			float fAttackTimer_ = 0.0f;					//! 攻撃タイマー。
			int iAttackPower_ = 10;						//! 攻撃力。
		};
	}
}