#pragma once


namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   Stage.h
		 * @brief  Stageの生成を行う。
		 */
		class Stage : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			Stage();
			 ~Stage();


		public:
			/* ライフサイクル。*/
			virtual bool Start();
			virtual void Update();
			virtual void Render(RenderContext& rc);


		private:
			/** ステージモデル　*/
			ModelRender stageModel_;
			/** 当たり判定用モデル */
			ModelRender collisionModel_;
			/** 地面用モデル */
			ModelRender groundModel_;

			/** ステージの静的コリジョン */
			PhysicsStaticObject stageColision_;
			/** 地面の静的コリジョン */
			PhysicsStaticObject groundCollision_;
		};
	}
}