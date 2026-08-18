#include "stdafx.h"
#include "CommonEnemy.h"
#include "Src/Actor/Character/Enemy/State/EnemyIdleState.h"

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
			/* 仮モデルをロードする。*/
			stModelRender_.Init(sUnityChanModelPath_, nullptr, 0, enModelUpAxisZ);

			/* 初期位置をモデルへ反映する。*/
			ApplyModelTransform();

			/* 最初は待機ステートから始める。*/
			pStateMachine_->ChangeState(new EnemyIdleState());
			return true;
		}


		void CommonEnemy::Update()
		{
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


		void CommonEnemy::LookAtTarget()
		{
			/* 対象へのベクトルを更新する。*/
			UpdateToTargetVector();

			/* ベクトルが無ければ向きを変えない。*/
			if (vToTarget_.Length() <= 0.0f)
			{
				return;
			}

			/* 対象の方向を向く。*/
			qLook_.SetRotationY(atan2f(vToTarget_.x, vToTarget_.z));
			stModelRender_.SetRotation(qLook_);
		}


		void CommonEnemy::MoveToTarget()
		{
			/* 対象が無ければ動かない。*/
			if (pTarget_ == nullptr)
			{
				return;
			}

			/* 攻撃距離なら止まる。*/
			if (IsTargetInAttackRange())
			{
				LookAtTarget();
				return;
			}

			/* 対象方向へ移動する。*/
			UpdateToTargetVector();
			vToTarget_.Normalize();
			vPosition_ += vToTarget_ * fChaseSpeed_ * g_gameTime->GetFrameDeltaTime();

			/* 移動方向を向く。*/
			LookAtTarget();
		}


		void CommonEnemy::AttackTarget()
		{
			/* 対象が無ければ攻撃しない。*/
			if (pTarget_ == nullptr)
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
	}
}