#include "stdafx.h"
#include "PlayerTransition.h"
#include "Src/Actor/Character/Player/Player.h"
#include "Src/Actor/Character/Player/State/PlayerIdleState.h"
#include "Src/Actor/Character/Player/State/PlayerWalkState.h"
#include "Src/Actor/Character/Player/State/PlayerRunState.h"
#include "Src/Actor/Character/Player/State/PlayerWinPauseState.h"
#include "Src/Actor/Character/Player/State/PlayerAttackState.h"


namespace nsApp
{
	namespace nsActor
	{
		namespace
		{
			/**
			 * @brief Idle ステートを生成する。
			 * @return 生成したステート。
			 */
			nsState::IState<Actor>* CreateIdleState()
			{
				return new PlayerIdleState();
			}

			/**
			 * @brief Walk ステートを生成する。
			 * @return 生成したステート。
			 */
			nsState::IState<Actor>* CreateWalkState()
			{
				return new PlayerWalkState();
			}

			/**
			 * @brief Run ステートを生成する。
			 * @return 生成したステート。
			 */
			nsState::IState<Actor>* CreateRunState()
			{
				return new PlayerRunState();
			}

			/**
			 * @brief WinPause ステートを生成する。
			 * @return 生成したステート。
			 */
			nsState::IState<Actor>* CreateWinPauseState()
			{
				return new PlayerWinPauseState();
			}

			/**
			 * @brief Attack ステートを生成する。
			 * @return 生成したステート。
			 */
			nsState::IState<Actor>* CreateAttackState()
			{
				return new PlayerAttackState();
			}
		}


		void PlayerTransition::Bind(Player* pPlayer)
		{
			/* ChangeState に使う所有者を保持する。*/
			pPlayer_ = pPlayer;
		}


		void PlayerTransition::RegisterState(EnPlayerState enState, PlayerStateCreator fnCreate)
		{
			/* 種別を添え字にして生成関数を登録する。*/
			creators_[static_cast<size_t>(enState)] = fnCreate;
		}


		void PlayerTransition::BuildTree()
		{
			/* 樹を組み直す。同じ From では先に書いた枝を優先する。*/
			edges_.clear();

			/*
			 * 移動系：Idle。
			 * スプリント付き移動を先に見て Run へ、それ以外の移動は Walk へ。
			 */
			edges_.push_back({ EnPlayerState::Idle, enPlayerFact_Moving | enPlayerFact_Sprinting, EnPlayerState::Run });
			edges_.push_back({ EnPlayerState::Idle, enPlayerFact_Moving | enPlayerFact_NotSprinting, EnPlayerState::Walk });

			/*
			 * 移動系：Walk。
			 * 止まったら Idle。歩きながらスプリントしたら Run。
			 */
			edges_.push_back({ EnPlayerState::Walk, enPlayerFact_NotMoving, EnPlayerState::Idle });
			edges_.push_back({ EnPlayerState::Walk, enPlayerFact_Moving | enPlayerFact_Sprinting, EnPlayerState::Run });

			/*
			 * 移動系：Run。
			 * 止まったら Idle。スプリントをやめて移動中なら Walk。
			 */
			edges_.push_back({ EnPlayerState::Run, enPlayerFact_NotMoving, EnPlayerState::Idle });
			edges_.push_back({ EnPlayerState::Run, enPlayerFact_Moving | enPlayerFact_NotSprinting, EnPlayerState::Walk });

			/*
			 * WinPause / Attack への枝は後で足す。
			 * いまは生成関数だけ登録しておき、遷移はまだ張らない。
			 */

			 /* 派生の new は構築時に閉じる。実行時の TryChangeState では switch しない。*/
			RegisterState(EnPlayerState::Idle, CreateIdleState);
			RegisterState(EnPlayerState::Walk, CreateWalkState);
			RegisterState(EnPlayerState::Run, CreateRunState);
			RegisterState(EnPlayerState::WinPause, CreateWinPauseState);
			RegisterState(EnPlayerState::Attack, CreateAttackState);
		}


		void PlayerTransition::SetCurrentState(EnPlayerState enState)
		{
			/* 各 State の Enter から呼ばれ、樹の「今の節」を同期する。*/
			enCurrentState_ = enState;
		}


		bool PlayerTransition::TryChangeState()
		{
			/* 所有者が無ければ遷移できない。*/
			if (pPlayer_ == nullptr)
				return false;

			/* 事実を集め、今の節から出る枝を上から順に見る。*/
			const PlayerSituation stSituation = MakeSituation();

			for (const PlayerTransitionEdge& stEdge : edges_)
			{
				/* 出発節が今のステートと違う枝はスキップする。*/
				if (stEdge.enFrom_ != enCurrentState_)
					continue;

				/* 必要な事実が揃っていなければ次の枝へ。*/
				if (!stSituation.Matches(stEdge.needFacts_))
					continue;

				/* 行き先が今と同じなら切り替えない。*/
				if (stEdge.enTo_ == enCurrentState_)
					return false;

				/* 登録テーブルから生成関数を取り出す。*/
				PlayerStateCreator fnCreate = creators_[static_cast<size_t>(stEdge.enTo_)];
				if (fnCreate == nullptr)
					return false;

				/* 節を更新してからステート実体を切り替える。*/
				enCurrentState_ = stEdge.enTo_;
				pPlayer_->GetStateMachine()->ChangeState(fnCreate());
				return true;
			}

			/* 条件に合う枝が無ければ現状維持。*/
			return false;
		}


		PlayerSituation PlayerTransition::MakeSituation() const
		{
			/* 所有者から事実だけを読む。遷移ルールはここに書かない。*/
			PlayerSituation stSituation;
			stSituation.Clear();
			stSituation.SetMoving(pPlayer_->IsMoving());
			stSituation.SetSprinting(pPlayer_->IsSprinting());
			/* WantWinPause / WantAttack は後で Player 側にフラグを用意してから読む。*/
			stSituation.SetWantWinPause(false);
			stSituation.SetWantAttack(false);

			return stSituation;
		}
	}
}