#pragma once

#include "Src/Actor/Character/Enemy/IEnemy.h"
#include "Src/Actor/Character/Common/CharacterMovement.h"
#include "Src/Actor/Character/Enemy/Transition/EnemyTransition.h"
#include "Src/System/RayTest/SightCheck.h"

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
		class CommonEnemy : public IEnemy
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
			void PlayIdle() override;

			/**
			 * @brief 歩きアニメーションを再生する。
			 */
			void PlayWalk() override;

			/**
			 * @brief 現在のステート名を取得する。
			 * @return 現在のステート名。
			 */
			const wchar_t* GetCurrentStateName() const;

			/**
			 * @brief ダメージを与え、ノックバック要求を立てる。
			 * @param iDamage ダメージ量。
			 */
			void ApplyDamage(int iDamage) override;

			/**
			 * @brief ノックバック開始待ちか。
			 * @return 待ちなら true。
			 */
			bool IsKnockBackPending() const override;

			/**
			 * @brief ノックバックが終了したか。
			 * @return 終了していれば true。
			 */
			bool IsKnockBackFinished() const override;

			/**
			 * @brief ノックバック開始時の準備をする。
			 */
			void BeginKnockBack() override;

			/**
			 * @brief ノックバック移動を1フレーム進める。
			 */
			void ExecuteKnockBack() override;


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
			 * @brief 現在位置を設定する(突き飛ばしなど外部から動かす場合に使う)。
			 *        移動処理側の座標にも反映しないと、次のフレームで元に戻ってしまう。
			 * @param vPosition 設定する位置。
			 */
			void SetPosition(const Vector3& vPosition)
			{
				vPosition_ = vPosition;
				stMovement_.SetPosition(vPosition_);
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
			inline bool IsTargetDead() const override
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
			inline void ReadyAttack() override
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
			 * @brief アグロ円内に対象がいるか。
			 * @return 円内なら true。
			 */
			bool IsTargetInAggroRange() override;

			/**
			 * @brief 攻撃距離に入っているか。
			 * @return 入っていればtrue。
			 */
			bool IsTargetInAttackRange() override;

			/**
			 * @brief 対象が視線上にいるか。
			 * @return 視線が通っていれば true。
			 */
			bool IsTargetVisible() const override;

			/**
			 * @brief 遷移樹に判定を依頼する。
			 * @return ステートを切り替えたら true。
			 */
			bool TryChangeState() override;

			/**
			 * @brief 現在のステート種別を遷移樹へ通知する。
			 * @param enState 入ったステート種別。
			 */
			void NotifyEnemyState(EnEnemyState enState) override;

			/**
			 * @brief ステートマシーンを取得する。
			 * @return ステートマシーン（非所有）。
			 */
			nsState::StateMachine<Actor>* GetStateMachine() override;

			/**
			 * @brief 追跡対象へ移動する。
			 */
			void MoveToTarget() override;

			/**
			 * @brief 追跡対象を攻撃する。
			 */
			void AttackTarget() override;

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

			/**
			 * @brief 目の高さの座標を作る。
			 * @param vPos 基準位置。
			 * @return 目の高さの座標。
			 */
			Vector3 MakeEyePosition(const Vector3& vPos) const;


		private:
			EnemyTransition stTransition_; //! ステート遷移。
			CharacterMovement stMovement_; //! 移動処理。
			ModelRender stModelRender_; //! 仮モデル。
			ICharacter* pTarget_ = nullptr; //! 追跡対象。
			Vector3 vPosition_ = { 0.0f, 0.0f, 500.0f }; //! 現在位置。
			Vector3 vToTarget_ = Vector3::Zero; //! 対象への水平ベクトル。
			Quaternion qLook_ = Quaternion::Identity; //! 対象方向の回転。
			Vector3 vSpeed_ = Vector3::Zero; //! 移動速度。
			float fAggroRange_ = 250.0f; //! アグロ円の半径。
			float fChaseSpeed_ = 120.0f; //! 追跡速度。
			float fAttackRange_ = 120.0f; //! 攻撃距離。
			float fAttackInterval_ = 1.0f; //! 攻撃間隔。
			float fAttackTimer_ = 0.0f; //! 攻撃タイマー。
			int iAttackPower_ = 10; //! 攻撃力。
			int iPlayingAnimation_ = -1; //! 再生中のアニメーション番号。
			nsSystem::SightCheck stSightCheck_; //! 視線判定。
			Vector3 vKnockBackSpeed_ = Vector3::Zero; //! ノックバック速度。
			float fKnockBackTimer_ = 0.0f; //! ノックバック残り時間（秒）。
			float fKnockBackDuration_ = 0.15f; //! ノックバック時間。
			float fKnockBackPower_ = 200.0f; //! ノックバック初速（秒速）。
			bool bKnockBackPending_ = false; //! ノックバック開始待ち。
			bool bKnockBackFinished_ = false; //! ノックバック終了。
			Vector3 vAway_ = Vector3::Zero;//! ノックバック方向。
		};
	}
}