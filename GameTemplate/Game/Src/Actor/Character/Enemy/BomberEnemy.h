#pragma once

#include "Src/Actor/Character/Enemy/IEnemy.h"
#include "Src/Actor/Character/Common/CharacterMovement.h"
#include "Src/Actor/Character/Common/CharacterAnimation.h"
#include "Src/Actor/Character/Enemy/Transition/EnemyTransition.h"
#include "Src/System/RayTest/SightCheck.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   BomberEnemy.h
		 * @brief  爆弾ゾンビ。max のアニメ本数だけ動作を持つ（Idle/Run/Roar/Hit/Deth）。
		 * @details 徘徊しない。攻撃距離で自爆、死亡時も未爆発なら爆風を出す。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/08: 新規作成日にち。
		 */
		class BomberEnemy : public IEnemy
		{
		public:
			/* コンストラクタとデストラクタ。*/
			BomberEnemy() = default;
			virtual ~BomberEnemy() = default;


		public:
			/* ライフサイクル。*/
			/**
			 * @brief 初期化処理。
			 * @return 初期化に成功したら true。
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
			 * @brief プールから出して戦場に出す。
			 * @param vPos 出現位置。
			 * @param pTarget 追跡対象。
			 */
			void Activate(const Vector3& vPos, ICharacter* pTarget) override;

			/**
			 * @brief プール待機へ戻す。
			 */
			void Deactivate() override;

			/**
			 * @brief プール待機中か。
			 * @return 待機中なら true。
			 */
			bool IsPoolInactive() const override
			{
				return bPoolInactive_;
			}


		public:
			/**
			 * @brief Idle.max を再生する。
			 */
			void PlayIdle() override;

			/**
			 * @brief Walk は max に無い。呼ばれても何もしない。
			 */
			void PlayWalk() override;

			/**
			 * @brief 徘徊はしない。IEnemy 互換の空実装。
			 */
			void BeginWander() override;

			/**
			 * @brief 徘徊はしない。IEnemy 互換の空実装。
			 */
			void ExecuteWander() override
			{
			}

			/**
			 * @brief 待機完了で徘徊へ行かない。常に false。
			 * @return 常に false。
			 */
			bool IsIdleDone() const override
			{
				return false;
			}

			/**
			 * @brief 徘徊しないので常に終わっている扱い。
			 * @return 常に true。
			 */
			bool IsWanderDone() const override
			{
				return true;
			}

			/**
			 * @brief Idle タイマーは使わないが、Enter 用に残す。
			 */
			void ResetIdleTimer() override
			{
				fIdleTimer_ = 0.0f;
			}

			/**
			 * @brief Run.max を再生する。
			 */
			void PlayRun() override;

			/**
			 * @brief Roar.max を再生する（攻撃＝咆哮）。
			 */
			void PlayAttack() override;

			/**
			 * @brief Deth.max を再生し、未爆発なら爆風を出す。
			 */
			void PlayDeath() override;

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
			 * @brief ノックバック開始時の準備をする（Hit.max）。
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
			 * @brief 現在位置を設定する。
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
				pTarget_ = pTarget;
			}

			/**
			 * @brief 追跡対象が死亡しているか。
			 * @return 対象がいない、または死亡していれば true。
			 */
			inline bool IsTargetDead() const override
			{
				if (pTarget_ == nullptr)
					return true;

				return pTarget_->IsDead();
			}

			/**
			 * @brief すぐに自爆できる状態にする。
			 */
			inline void ReadyAttack() override
			{
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
			 * @brief 自爆距離に入っているか。
			 * @return 入っていれば true。
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
			 * @brief 自爆する。殴らない。
			 */
			void AttackTarget() override;

			/**
			 * @brief 死亡ステートの処理を実行する。
			 */
			void ExecuteDeth() override;

			/**
			 * @brief 死亡フェードが終わったか。
			 * @return 終わっていれば true。
			 */
			bool IsDeathFadeDone() const override
			{
				return fModelAlpha_ <= 0.0f;
			}

			/**
			 * @brief 位置をモデルへ反映する。
			 */
			void ApplyModelTransform();


		private:
			/**
			 * @brief CharacterAnimBank 経由でアニメを読み、CharacterModel をロードする。
			 */
			void InitCharacterModel();

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
			 * @brief Hit.max を再生する。
			 */
			void PlayHit();

			/**
			 * @brief 死亡ステートか。
			 * @return 死亡ステートなら true。
			 */
			bool IsDeathState() const;

			/**
			 * @brief 目の高さの座標を作る。
			 * @param vPos 基準位置。
			 * @return 目の高さの座標。
			 */
			Vector3 MakeEyePosition(const Vector3& vPos) const;

			/**
			 * @brief 死亡演出の経過時間から、表示用のα値を求める。
			 * @param fTimer 死亡からの経過秒数。
			 * @return 表示用のα値。
			 */
			float CalcDeathFadeAlpha(float fTimer) const;

			/**
			 * @brief 消滅エフェクトを出すべきタイミングかを図る。
			 * @param fAlpha 現在のα値。
			 * @return 出すべきタイミングなら true。
			 */
			inline bool ShouldPlayDeathEffect(float fAlpha) const
			{
				return !bDeathEffectPlayed_ && fAlpha <= 0.5f;
			}

			/**
			 * @brief 爆発範囲内の対象へ、一度だけダメージを出す。
			 */
			void Explode();

			/**
			 * @brief 自爆後に自分を死亡扱いにする。
			 */
			void KillSelfAfterExplode();


		private:
			EnemyTransition stTransition_; //! ステート遷移。
			CharacterMovement stMovement_; //! 移動処理。
			CharacterAnimation stAnimation_; //! Bomber 用アニメーション。
			ICharacter* pTarget_ = nullptr; //! 追跡対象。
			Vector3 vPosition_ = { 0.0f, 0.0f, 500.0f }; //! 現在位置。
			Vector3 vToTarget_ = Vector3::Zero; //! 対象への水平ベクトル。
			Quaternion qLook_ = Quaternion::Identity; //! 対象方向の回転。
			Vector3 vSpeed_ = Vector3::Zero; //! 移動速度。
			float fAggroRange_ = 600.0f; //! アグロ円の半径。
			float fChaseSpeed_ = 120; //! 追跡速度（Common より遅い）。
			float fAttackRange_ = 120.0f; //! 自爆距離。
			float fAttackInterval_ = 0.0f; //! 自爆は間隔なし。
			float fAttackTimer_ = 0.0f; //! 攻撃タイマー。
			int iPlayingAnimation_ = -1; //! 再生中のアニメーション番号。
			nsSystem::SightCheck stSightCheck_; //! 視線判定。
			Vector3 vKnockBackSpeed_ = Vector3::Zero; //! ノックバック速度。
			float fKnockBackTimer_ = 0.0f; //! ノックバック残り時間（秒）。
			float fKnockBackDuration_ = 0.15f; //! ノックバック時間。
			float fKnockBackPower_ = 200.0f; //! ノックバック初速（秒速）。
			bool bKnockBackPending_ = false; //! ノックバック開始待ち。
			bool bKnockBackFinished_ = false; //! ノックバック終了。
			Vector3 vAway_ = Vector3::Zero; //! ノックバック方向。
			float fIdleTimer_ = 0.0f; //! Idle Enter 用（徘徊判定には使わない）。
			float fDeathFadeDuration_ = 1.5f; //! 死亡演出全体の長さ（秒）。
			float fDeathHoldDuration_ = 0.3f; //! 最初に不透明のまま見せる時間（秒）。
			float fDeathFadeTimer_ = 0.0f; //! 死亡演出経過時間（秒）。
			float fModelAlpha_ = 1.0f; //! 表示用のα値。
			bool bDeathEffectPlayed_ = false; //! 死亡エフェクトを出したか。
			bool bPoolInactive_ = true; //! true ならプール待機中。
			float fExplodeRange_ = 250.0f; //! 爆発の半径(cm)。
			int iExplodeDamage_ = 20; //! 爆発のダメージ。
			bool bExploded_ = false; //! 接近爆発と死亡爆発で2回入らないようにする。
		};
	}
}