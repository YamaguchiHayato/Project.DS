#pragma once
/**
 * @file   DummyPlayer.h
 * @brief  敵テスト用の仮プレイヤー。
 * @author Yamaguchi Hayato
 * @date   2026/08/18
 */

#include "Src/Actor/Character/ICharacter.h"

namespace nsApp
{
	namespace nsActor
	{
		class DummyPlayer : public ICharacter
		{
		public:
			/* コンストラクタとデストラクタ。*/
			DummyPlayer() = default;
			virtual ~DummyPlayer() = default;


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
			 * @brief 現在位置を取得する。
			 * @return 現在位置。
			 */
			Vector3& GetPosition() override
			{
				return vPosition_;
			}


		private:
			/**
			 * @brief 移動する。
			 */
			void Move();

			/**
			 * @brief HP表示を更新する。
			 */
			void UpdateHpFont();

			/**
			 * @brief アニメーションを再生する。
			 * @param iAnimationNumber 再生するアニメーション番号。
			 */
			void PlayAnimation(int iAnimationNumber);


		private:
			CharacterController stCharaCon_;	//! 壁との押し戻し用。
			ModelRender stModelRender_;						//! 仮モデル。
			AnimationClip aAnimationClip_[2];				//! IdleとWalk。
			FontRender stHpFont_;							//! HP表示。
			wchar_t aHpText_[32] = {};						//! HP表示用文字列。
			Vector3 vPosition_ = { 0.0f, 0.0f, 0.0f };	//! 現在位置。
			float fMoveSpeed_ = 200.0f;						//! 移動速度。
			bool bIsMoving_ = false;						//! 移動中か。
			int iPlayingAnimation_ = -1;					//! 再生中のアニメーション番号。
		};
	}
}