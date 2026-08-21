#pragma once

#include "Src/Actor/Character/ICharacter.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   CommonEnemy.h
		 * @brief  雑魚敵。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/18
		 */
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
			 * @brief 待機アニメーションを再生する。
			 */
			void PlayIdle();

			/**
			 * @brief 歩きアニメーションを再生する。
			 */
			void PlayWalk();

			/**
			 * @brief 現在のステート名を取得する。
			 * @return 現在のステート名。
			 */
			const wchar_t* GetCurrentStateName() const;


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
			 * @brief 追跡対象が死亡しているか。
			 * @return 対象がいない、または死亡していればtrue。
			 */
			inline bool IsTargetDead() const
			{
				/* 対象が無ければ死亡扱い。*/
				if (pTarget_ == nullptr)
					return true;

				/* 対象の死亡フラグを返す。*/
				return pTarget_->IsDead();
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

			/**
			 * @brief アニメーションを再生する。
			 * @param iAnimationNumber 再生するアニメーション番号。
			 */
			void PlayAnimation(int iAnimationNumber);

			/**
			 * @brief 死亡ステートか。
			 * @return 死亡ステートならtrue。
			 */
			bool IsDeathState() const;

			
		private:
			ModelRender stModelRender_;					//! 仮モデル。
			ICharacter* pTarget_ = nullptr;				//! 追跡対象。
			Vector3 vPosition_ = { 0.0f, 0.0f, 500.0f };	//! 現在位置。
			Vector3 vToTarget_;							//! 対象への水平ベクトル。
			Quaternion qLook_;							//! 対象方向の回転。
			float fDetectRange_ = 250.0f;				//! 発見距離。
			float fChaseSpeed_ = 120.0f;				//! 追跡速度。
			float fAttackRange_ = 120.0f;				//! 攻撃距離。
			float fAttackInterval_ = 1.0f;				//! 攻撃間隔。
			float fAttackTimer_ = 0.0f;					//! 攻撃タイマー。
			int iAttackPower_ = 10;						//! 攻撃力。
			int iPlayingAnimation_ = -1;				//! 再生中のアニメーション番号。
		};
	}
}