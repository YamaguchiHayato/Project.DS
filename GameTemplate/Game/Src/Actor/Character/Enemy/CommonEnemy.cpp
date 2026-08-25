#include "stdafx.h"
#include "CommonEnemy.h"
#include "Src/Actor/Character/Enemy/State/EnemyIdleState.h"
#include "Src/System/GamePause.h"
#include "Src/Actor/Character/Enemy/State/EnemyChaseState.h"
#include "Src/Actor/Character/Enemy/State/EnemyAttackState.h"
#include "Src/Actor/Character/Enemy/State/EnemyDeadState.h"

namespace
{
	const char* sUnityChanModelPath_ = "Assets/modelData/unityChan.tkm";	//! Unityちゃんのモデルパス。
}

namespace nsApp
{
	namespace nsActor
	{
		bool CommonEnemy::Start()
		{
			/* カプセルで壁（PhysicsStaticObject）と当たる。*/
			stCharaCon_.Init(20.0f, 70.0f, vPosition_);

			stSightCheck_.SetEyeHeight(120.0f);

			/* 体力を初期化する。*/
			/* @todo: 外部Parameter化。*/
			stCharacterStatus_.stHp_.iMaxHP_ = 30;
			stCharacterStatus_.stHp_.iCurrentHP_ = 30;

			/* 仮モデルをロードする。*/
			stModelRender_.Init(sUnityChanModelPath_, nullptr, 0, enModelUpAxisZ);
			stModelRender_.SetPosition(vPosition_);


			/* 初期位置をモデルへ反映する。*/
			ApplyModelTransform();

			/* 最初は待機ステートから始める。*/
			pStateMachine_->ChangeState(new EnemyIdleState());

			return true;
		}


		void CommonEnemy::Update()
		{
			/* ポーズ中は動かさない。*/
			if (nsSystem::IsGamePaused())
				return;

			/* ステートマシーンを更新する。*/
			ICharacter::Update();
			/* 対象が死亡している場合は死亡ステートに遷移する。*/
			if(IsDead() && !IsDeathState())
				pStateMachine_->ChangeState(new EnemyDeathState());


			/* 死亡ステートなら何もしない。*/
			if (IsDeathState())
			{
				/* ステートマシーンを更新する。*/
				ICharacter::Update();
				/* 位置をモデルへ反映する。*/
				ApplyModelTransform();
				return;
			}

			/* ステートマシーンを更新する。*/
			ICharacter::Update();

			/* 攻撃タイマーを進める。*/
			fAttackTimer_ += g_gameTime->GetFrameDeltaTime();

			/* 位置をモデルへ反映する。*/
			ApplyModelTransform();
		}


		void CommonEnemy::Render(RenderContext& rc)
		{
			/* 仮モデルを描画する。*/
			stModelRender_.Draw(rc);
		}


		void CommonEnemy::UpdateToTargetVector()
		{
			/* 対象が無ければゼロベクトルにする。*/
			if (pTarget_ == nullptr)
			{
				vToTarget_ = Vector3::Zero;
				return;
			}

			/* 高さは無視した対象へのベクトルを作る。*/
			vToTarget_ = pTarget_->GetPosition() - vPosition_;
			vToTarget_.y = 0.0f;
		}


		float CommonEnemy::GetDistanceToTarget()
		{
			/* 対象へのベクトルを更新して長さを返す。*/
			UpdateToTargetVector();
			return vToTarget_.Length();
		}


		bool CommonEnemy::IsTargetInDetectRange()
		{
			/* 発見距離以内ならtrue。*/
			return GetDistanceToTarget() <= fDetectRange_;
		}


		bool CommonEnemy::IsTargetInAttackRange()
		{
			/* 攻撃距離以内ならtrue。*/
			return GetDistanceToTarget() <= fAttackRange_;
		}


		Vector3 CommonEnemy::MakeEyePosition(const Vector3& vPos) const
		{
			/* 目の高さの座標を作る。*/
			Vector3 vEye = vPos;

			/* 目の高さを加算する。*/
			vEye.y += stSightCheck_.GetEyeHeight();

			/* 目の高さの座標を返す。*/
			return vEye;
		}


		bool CommonEnemy::IsTargetVisible() const
		{
			/* 対象が無ければ視線は通らない。*/
			if (pTarget_ == nullptr)
				return false;

			/* 対象の目の高さまでの視線が通っていればtrue。*/
			return stSightCheck_.HasClearSight( MakeEyePosition(vPosition_), MakeEyePosition(pTarget_->GetPosition()));
		}


		void CommonEnemy::LookAtTarget()
		{
			/* 対象が無ければ向きを変えない。*/
			if (!IsTargetInDetectRange())
				return;

			/* 対象へのベクトルを更新する。*/
			UpdateToTargetVector();

			/* ベクトルが無ければ向きを変えない。*/
			if (vToTarget_.Length() <= 0.0f)
				return;

			/* 対象の方向を向く。*/
			qLook_.SetRotationY(atan2f(vToTarget_.x, vToTarget_.z));
			stModelRender_.SetRotation(qLook_);
		}


		void CommonEnemy::MoveToTarget()
		{
			/* 対象が無ければ動かない。*/
			if (pTarget_ == nullptr)
				return;

			

			/* 攻撃距離外なら対象方向へ進む。*/
			if (!IsTargetInAttackRange())
			{
				UpdateToTargetVector();
				vToTarget_.Normalize();
				vSpeed_ = vToTarget_ * fChaseSpeed_;
			}

			/* 対象方向を向く。*/
			LookAtTarget();

			/* キャラコンで動かして、モデル座標も合わせる。*/
			vPosition_ = stCharaCon_.Execute(vSpeed_, g_gameTime->GetFrameDeltaTime());
		}


		void CommonEnemy::AttackTarget()
		{
			/* 対象が無ければ攻撃しない。*/
			if (pTarget_ == nullptr)
				return;

			/* 対象が死亡していれば攻撃しない。*/
			if (pTarget_->IsDead())
				return;

			/* 対象の方向を向く。*/
			LookAtTarget();

			/* 攻撃間隔が残っているなら撃たない。*/
			if (fAttackTimer_ < fAttackInterval_)
				return;

			/* ダメージを与えてタイマーを戻す。*/
			pTarget_->ApplyDamage(iAttackPower_);
			fAttackTimer_ = 0.0f;
		}


		void CommonEnemy::ApplyModelTransform()
		{
			/* 位置をモデルへ反映する。*/
			stModelRender_.SetPosition(vPosition_);
			stModelRender_.Update();
		}


		void CommonEnemy::PlayAnimation(int iAnimationNumber)
		{
			/* 同じアニメなら再生し直さない。*/
			if (iPlayingAnimation_ == iAnimationNumber)
				return;

			/* 指定アニメを再生する。*/
			iPlayingAnimation_ = iAnimationNumber;
			stModelRender_.PlayAnimation(iAnimationNumber, 0.2f);
		}


		void CommonEnemy::PlayIdle()
		{
			/* 待機を再生する。*/
			PlayAnimation(0);
		}


		void CommonEnemy::PlayWalk()
		{
			/* 歩きを再生する。*/
			PlayAnimation(1);
		}


		bool CommonEnemy::IsDeathState()const
		{
			/* ステートマシーンが無ければ死亡ステートではない。*/
			if (pStateMachine_ == nullptr || pStateMachine_->GetCurrentState() == nullptr)
				return false;

			/* 現在のステートが死亡ステートかを判定する。*/
			return dynamic_cast<EnemyDeathState*>(pStateMachine_->GetCurrentState()) != nullptr;
		}


		const wchar_t* CommonEnemy::GetCurrentStateName() const
		{
			/* ステートマシーンが無ければNone。*/
			if (pStateMachine_ == nullptr || pStateMachine_->GetCurrentState() == nullptr)
				return L"None";

			/* 現在のステートを取得して名前を返す。*/
			nsState::IState<Actor>* pState = pStateMachine_->GetCurrentState();

			/* ステートの型を判定して名前を返す。*/
			/* ここではdynamic_castを使ってステートの型を判定している。*/
			/* @todo TSVファイルを用いて文字列化する。*/
			if (dynamic_cast<EnemyIdleState*>(pState) != nullptr)
				return L"Idle";
			if (dynamic_cast<EnemyChaseState*>(pState) != nullptr)
				return L"Chase";
			if (dynamic_cast<EnemyAttackState*>(pState) != nullptr)
				return L"Attack";
			if (dynamic_cast<EnemyDeathState*>(pState) != nullptr)
				return L"Death";
			return L"Unknown";
		}
	}
}