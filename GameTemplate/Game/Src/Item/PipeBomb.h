#pragma once
#include "Src/Actor/Character/ICharacter.h"

namespace nsApp
{
	namespace nsItem
	{
		/**
		 * @file   PipeBomb.h
		 * @brief  パイプ爆弾(投擲アイテム)。
		 *         視線方向へ山なりに飛んで着地し、数秒間ピカピカ光りながら周りの雑魚を
		 *         おとりとして引き寄せ、信管が切れると爆発して周囲にまとめてダメージを与える。
		 *         本家(L4D2)と同じく「群れを1か所に集めて始末する」ための投擲物。
		 * @note   ICharacter を継承しているのは、敵の追跡対象(CommonEnemy::SetTarget)が ICharacter 型で、
		 *         おとりとして敵に狙わせるためにその型が要るから。キャラクターとして動くわけではない。
		 *         敵に殴られても尽きないよう、HPは大きく持たせている。
		 *         敵側の追跡はアグロ距離の内側でしか始まらないので、引き寄せは着地点の近くの敵に限られる。
		 *         @todo 敵側で「おとりへは距離を問わず向かう」ができると本家に近づく(山口さんと相談)。
		 * @author Izumida Kiryu
		 * @date   2026/09/12
		 */
		class PipeBomb : public nsActor::ICharacter
		{
		public:
			/* コンストラクタとデストラクタ。*/
			PipeBomb() = default;
			virtual ~PipeBomb() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;

			/**
			 * @brief 投擲開始の位置と方向を設定する。NewGO直後(Startより前)に呼ぶ。
			 * @param vPos 投擲開始位置。
			 * @param vDir 投擲方向(正規化前でも可)。
			 */
			void Setup(const Vector3& vPos, const Vector3& vDir);


		/* ゲッター。*/
		public:
			/**
			 * @brief 現在位置を取得する。敵はここへ向かって来る。
			 * @return 現在位置。
			 */
			Vector3& GetPosition() override
			{
				return vPosition_;
			}


		private:
			/**
			 * @brief 飛んでいる間の簡易物理(重力で放物線)。着地したら止まる。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateFlight(float fDeltaTime);

			/**
			 * @brief 近くの雑魚の追跡対象を自分に向ける(おとり)。一定間隔で見直す。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateAttract(float fDeltaTime);

			/**
			 * @brief 信管が切れるほど速く明滅させる(見た目)。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateBlink(float fDeltaTime);

			/**
			 * @brief 爆発して周囲の敵にダメージを与え、敵の追跡対象をプレイヤーへ戻す。
			 */
			void Explode();

			/**
			 * @brief 全ての雑魚の追跡対象をプレイヤーへ戻す。自分が消える前に必ず呼ぶ(消えた後を指されないため)。
			 */
			void RestoreEnemyTargets();


		private:
			ModelRender stModelRender_;				//! 表示モデル(仮の発光球)。
			Vector3 vPosition_ = Vector3::Zero;		//! 現在位置。
			Vector3 vVelocity_ = Vector3::Zero;		//! 速度。着地すると0。
			bool bIsLanded_ = false;				//! 着地したか。
			float fFuse_ = 0.0f;					//! 起爆までの残り時間(秒)。
			float fAttractTimer_ = 0.0f;			//! 次に引き寄せを見直すまでの時間(秒)。
			float fBlinkPhase_ = 0.0f;				//! 明滅の位相。
		};
	}
}
