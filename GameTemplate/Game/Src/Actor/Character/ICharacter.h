#pragma once
#include "Src/Actor/Actor.h"
#include "Src/Actor/Character/Common/CharacterModel.h"

namespace nsApp
{
	/**
	 * @struct AttackStatus
	 * @details 攻撃力のパラメータを定義。
	 */
	struct AttackStatus
	{
		float fNormalDamage_;		//! 通常の攻撃力。
		float fCriticalDamage_;		//! クリティカルダメージ。
		float fCriticalRate_;		//! クリティカル率。
	};

	/**
	 * @struct HPStatus
	 * @details HPのパラメータを定義。
	 */
	struct HPStatus
	{
		int iCurrentHP_;			//! 現在HP。
		int iMaxHP_;				//! 最大HP。
	};

	/**
	 * @struct CharacterStatus
	 * @details キャラクターのステータスをまとめた構造体。
	 */
	struct CharacterStatus
	{
		AttackStatus stAttack_;		//! 攻撃のステータス。
		HPStatus stHp_;				//! 体力のステータス。
	};

	namespace nsActor
	{
		/**
          * @file   ICharacter.h
          * @brief  実態のあるキャラクターが継承する基底クラス。
          *         ステータス管理もここで行います。
          * @author Yamaguchi Hayato
          * @date   2026/08/18
          */
		class ICharacter : public Actor
		{
		public:
			/* コンストラクタとデストラクタ*/
			ICharacter() = default;
			virtual ~ICharacter() = default;


		public:
			/* ライフサイクル。*/
			/* 初期化処理。*/
			virtual bool Start() override = 0;
			/* 更新処理。*/
			virtual void Update() override;
			/* 描画処理。*/
			virtual void Render(RenderContext& rc) override;


		/* ゲッター。*/
		public:
			/**
			 * @brief キャラクターの位置を取得する。
			 * @return キャラクターの位置。
			 */
			virtual Vector3& GetPosition() = 0;

			/**
			 * @brief 現在のHPを取得する。
			 * @return 現在のHP。
			 */
			inline int GetCurrentHP() const
			{
				return stCharacterStatus_.stHp_.iCurrentHP_;
			}

			/**
			 * @brief ダメージを与える。
			 * @param iDamage ダメージ量。
			 */
			inline void ApplyDamage(int iDamage)
			{
				/* 現在HPからダメージを引く。*/
				stCharacterStatus_.stHp_.iCurrentHP_ -= iDamage;

				/* 0未満にはしない。*/
				if (stCharacterStatus_.stHp_.iCurrentHP_ < 0)
					stCharacterStatus_.stHp_.iCurrentHP_ = 0;
			}

		protected:
			CharacterStatus stCharacterStatus_; //! キャラクターのステータス。
			CharacterModel stModel_; //! キャラクターモデル。
			int iHitStopFrame_; //! ヒットストップのフレーム数。
		};
	}
}