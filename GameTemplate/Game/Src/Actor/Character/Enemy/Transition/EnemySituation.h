#pragma once

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @enum EnEnemyFact
		 * @brief 遷移樹の枝が要求する事実フラグ。
		 * @details 複数条件はビット OR回路 で表し、AND回路 で照合する。
		 */
		enum EnEnemyFact : uint32_t
		{
			enEnemyFact_None = 0,					//! 条件なし。
			enEnemyFact_SelfDead = 1u << 0,			//! 自身が死亡。
			enEnemyFact_TargetDead = 1u << 1,		//! 対象が死亡。
			enEnemyFact_InAggro = 1u << 2,			//! アグロ円内。
			enEnemyFact_Visible = 1u << 3,			//! 視線が通っている。
			enEnemyFact_InAttack = 1u << 4,			//! 攻撃範囲内。
			enEnemyFact_NotInAggro = 1u << 5,		//! アグロ円外。
			enEnemyFact_NotInAttack = 1u << 6,		//! 攻撃範囲外。
			enEnemyFact_NeedKnockBack = 1u << 7,	//! ノックバック開始が必要。
			enEnemyFact_KnockBackDone = 1u << 8,	//! ノックバックが終了した。
		};


		/**
		 * @file   EnemySituation.h
		 * @brief  1フレーム分の敵の状況（遷移判断用の事実）。
		 * @details State に遷移 if を書かず、ここに事実を集約する。
		 *          遷移樹はこのクラスが作るフラグを見て枝を選ぶ。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/26
		 */
		class EnemySituation
		{
		public:
			/* コンストラクタとデストラクタ。*/
			EnemySituation() = default;
			virtual ~EnemySituation() = default;


		public:
			/**
			 * @brief 事実をすべて初期状態に戻す。
			 */
			void Clear();

			/**
			 * @brief 保持している事実をビットマスクへ変換する。
			 * @return 照合用の事実マスク。
			 */
			uint32_t MakeFactMask() const;

			/**
			 * @brief 要求された事実をすべて満たしているか判定する。
			 * @param uNeed 枝が要求する事実マスク。
			 * @return すべて満たしていれば true。
			 */
			bool Matches(uint32_t uNeed) const;


		/* セッター。*/
		public:
			/**
			 * @brief 自身の死亡フラグを設定する。
			 * @param bSelfDead 死亡していれば true。
			 */
			inline void SetSelfDead(bool bSelfDead)
			{
				bSelfDead_ = bSelfDead;
			}

			/**
			 * @brief 対象の死亡フラグを設定する。
			 * @param bTargetDead 死亡していれば true。
			 */
			inline void SetTargetDead(bool bTargetDead)
			{
				bTargetDead_ = bTargetDead;
			}

			/**
			 * @brief アグロ円内フラグを設定する。
			 * @param bInAggro 円内なら true。
			 */
			inline void SetInAggro(bool bInAggro)
			{
				bInAggro_ = bInAggro;
			}

			/**
			 * @brief 視線フラグを設定する。
			 * @param bVisible 視線が通っていれば true。
			 */
			inline void SetVisible(bool bVisible)
			{
				bVisible_ = bVisible;
			}

			/**
			 * @brief 攻撃範囲内フラグを設定する。
			 * @param bInAttack 攻撃範囲内なら true。
			 */
			inline void SetInAttack(bool bInAttack)
			{
				bInAttack_ = bInAttack;
			}

			/**
			 * @brief ノックバック開始が必要かを設定する。
			 * @param bNeedKnockBack 必要なら true。
			*/
			inline void SetNeedKnockBack(bool bNeedKnockBack)
			{
				bNeedKnockBack_ = bNeedKnockBack;
			}

			/**
			 * @brief ノックバック終了を設定する。
			 * @param bKnockBackDone 終了していれば true。
			 */
			inline void SetKnockBackDone(bool bKnockBackDone)
			{
				bKnockBackDone_ = bKnockBackDone;
			}


		/* ゲッター。*/
		public:
			/**
			 * @brief 自身が死亡しているか。
			 * @return 死亡していれば true。
			 */
			inline bool IsSelfDead() const
			{
				return bSelfDead_;
			}

			/**
			 * @brief 対象が死亡しているか。
			 * @return 死亡していれば true。
			 */
			inline bool IsTargetDead() const
			{
				return bTargetDead_;
			}

			/**
			 * @brief アグロ円内か。
			 * @return 円内なら true。
			 */
			inline bool IsInAggro() const
			{
				return bInAggro_;
			}

			/**
			 * @brief 視線が通っているか。
			 * @return 通っていれば true。
			 */
			inline bool IsVisible() const
			{
				return bVisible_;
			}

			/**
			 * @brief 攻撃範囲内か。
			 * @return 範囲内なら true。
			 */
			inline bool IsInAttack() const
			{
				return bInAttack_;
			}

			/**
			 * @brief ノックバック開始が必要か。
			 * @return 必要なら true。
			 */
			inline bool IsNeedKnockBack() const
			{
				return bNeedKnockBack_;
			}

			/**
			 * @brief ノックバックが終了したか。
			 * @return 終了していれば true。
			 */
			inline bool IsKnockBackDone() const
			{
				return bKnockBackDone_;
			}


		private:
			bool bSelfDead_ = false; //! 自身が死亡しているか。
			bool bTargetDead_ = false; //! 対象が死亡しているか。
			bool bInAggro_ = false; //! アグロ円内に対象がいるか。
			bool bVisible_ = false; //! 対象への視線が通っているか。
			bool bInAttack_ = false; //! 攻撃範囲内に対象がいるか。
			bool bNeedKnockBack_ = false; //! ノックバック開始が必要か。
			bool bKnockBackDone_ = false; //! ノックバックが終了したか。
		};
	}
}