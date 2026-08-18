#pragma once

#include "Src/Actor/Character/ICharacter.h"

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
		class CommonEnemy : public ICharacter
		{
		public:
			/* コンストラクタとデストラクタ。*/
			CommonEnemy() = default;
			virtual ~CommonEnemy() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;


		public:
			/**
			 * @brief キャラクターの位置を取得する。
			 * @return キャラクターの位置。
			 */
			Vector3& GetPosition() override
			{
				return vPosition_;
			}

			/**
			 * @brief キャラクターの位置を設定する。
			 * @param pTarget 追跡対象のキャラクター。
			 */
			inline void SetTarget(ICharacter* pTarget)
			{
				/* 追跡対象を受け取る。*/
				pTarget_ = pTarget;
			}


		private:
			ModelRender stModelRender_; //! 仮モデル。
			ICharacter* pTarget_ = nullptr; //! 追跡対象。
			Vector3 vPosition_ = { 50.0f, 0.0f, 0.0f }; //! 現在位置。
		};
	}
}