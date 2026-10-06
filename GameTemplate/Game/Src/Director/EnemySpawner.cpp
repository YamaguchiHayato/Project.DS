#include "stdafx.h"
#include "EnemySpawner.h"
#include "Src/Director/IEnemyPool.h"
#include "Src/Actor/Character/Common/CharacterModel.h"
#include "Src/Actor/Character/Enemy/CommonEnemy.h"
#include "Player.h"
#include "Src/System/GamePause.h"

namespace
{
	const float fTwoPi_ = 6.2831853f; //! 一周の角度(ラジアン)。

	/**
	 * @brief 0〜1の乱数を返す。
	 * @return 0〜1の乱数。
	 */
	float Random01()
	{
		return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	}

	/**
	 * @brief ゾンビの種類を、プールに渡すモデルの種類へ変換する。
	 * @param enEnemyType ゾンビの種類。
	 * @param outModelType 変換したモデルの種類。
	 * @return 変換できたらtrue。未設定(None)ならfalse。
	 */
	bool ToCharacterModelType(nsApp::nsActor::EnEnemyType enEnemyType, nsApp::CharacterModelType& outModelType)
	{
		switch (enEnemyType)
		{
		case nsApp::nsActor::EnEnemyType::Common:
			outModelType = nsApp::CharacterModelType::Common;
			return true;

			case nsApp::nsActor::EnEnemyType::Bile:
			outModelType = nsApp::CharacterModelType::Bile;
			return true;

			case nsApp::nsActor::EnEnemyType::Bomber:
			outModelType = nsApp::CharacterModelType::Bomber;
			return true;

		default:
			return false;
		}
	}
}


namespace nsApp
{
	namespace nsDirector
	{
		bool EnemySpawner::Start()
		{
			/* 種類が未設定なら何も湧かせない(設定漏れで勝手に雑魚が湧かないように)。*/
			if (stSetting_.enEnemyType_ == nsActor::EnEnemyType::None)
				bFinished_ = true;

			return true;
		}


		void EnemySpawner::Update()
		{
			/* ポーズ中は止める(間隔のタイマーも進めない)。*/
			if (nsSystem::IsGamePaused())
				return;

			/* もう湧かせない、または準備ができていなければ何もしない。*/
			if (bFinished_ || !WaitReady())
				return;

			/* 間隔が0なら、最初に1回だけまとめて湧かせて終わる。*/
			if (stSetting_.fInterval_ <= 0.0f)
			{
				SpawnWave();
				bFinished_ = true;
				return;
			}

			/* 間隔ごとに湧かせる。最初の1回は待たずに湧かせる。*/
			fIntervalTimer_ -= g_gameTime->GetFrameDeltaTime();
			if (fIntervalTimer_ > 0.0f)
				return;

			fIntervalTimer_ += stSetting_.fInterval_;
			SpawnWave();

			/* 合計に達したら終わる。*/
			if (IsReachedTotal())
				bFinished_ = true;
		}


		bool EnemySpawner::WaitReady()
		{
			/* 一度準備ができたら、以降は確認しない。*/
			if (bReady_)
				return true;

			/* ※プールがセットされていない場合は、何もしない。*/
			if (pPool_ == nullptr)
				return false;

			/* 対象を検索する。*/
			pTarget = FindGO<nsActor::Player>("player");
			if (pTarget == nullptr)
				return false;

			/* プールの敵がまだ無ければ待つ。*/
			const auto enemies = FindGOs<nsActor::CommonEnemy>("commonEnemy");
			if (enemies.empty())
				return false;

			/* 全員の初期化(Start)が終わるまで待つ。*/
			for (nsActor::CommonEnemy* pEnemy : enemies)
			{
				if (pEnemy == nullptr || !pEnemy->IsStart())
					return false;
			}

			bReady_ = true;

			return true;
		}


		void EnemySpawner::SpawnWave()
		{
			/* プールに渡すモデルの種類へ変換する。変換できなければ湧かせない。*/
			CharacterModelType enModelType = CharacterModelType::Common;
			if (!ToCharacterModelType(stSetting_.enEnemyType_, enModelType))
				return;

			for (int i = 0; i < stSetting_.iAmount_; ++i)
			{
				/* 合計の上限に達したら打ち切る。*/
				if (IsReachedTotal())
					return;

				/* プールが空なら打ち切る(間隔ありなら次の回で再挑戦する)。*/
				if (pPool_->Spawn(enModelType, MakeSpawnPosition(i), pTarget) == nullptr)
					return;

				++iSpawned_;
			}
		}


		Vector3 EnemySpawner::MakeSpawnPosition(int index) const
		{
			/* 湧かせる位置の初期値は中心。*/
			Vector3 vPos = stSetting_.vCenter_;

			/* 形に合わせて位置をずらす。*/
			switch (stSetting_.enShape_)
			{
			case EnSpawnShape::Circle:
			{
				/* 円の中に偏りなく散らす(半径に√を掛けないと中心に寄る)。*/
				const float fAngle = Random01() * fTwoPi_;
				const float fLength = sqrtf(Random01()) * stSetting_.fRadius_;
				vPos.x += cosf(fAngle) * fLength;
				vPos.z += sinf(fAngle) * fLength;
				break;
			}
			case EnSpawnShape::Ring:
			{
				/* 円周上に等間隔で並べる(放射状)。*/
				const float fAngle = fTwoPi_ * static_cast<float>(index) / static_cast<float>(stSetting_.iAmount_);
				vPos.x += cosf(fAngle) * stSetting_.fRadius_;
				vPos.z += sinf(fAngle) * stSetting_.fRadius_;
				break;
			}
			default:
				break;
			}

			return vPos;
		}
	}
}