#include "stdafx.h"
#include "Stage.h"


namespace
{
	const float STAGE_SCALE = 150.0f;		//ステージ大きさ
}

namespace nsApp
{
	namespace nsActor
	{
		Stage::Stage()
		{
			/** ステージモデル読み込み */
			stageModel_.Init("Assets/modelData/stage/FirstStage/firstStage.tkm", nullptr, 0, enModelUpAxisZ);
			stageModel_.SetPosition(Vector3::Zero);
			stageModel_.SetRotation(Quaternion::Identity);
			stageModel_.SetScale(Vector3(STAGE_SCALE, STAGE_SCALE, STAGE_SCALE));
			stageModel_.Update();

			/** 当たり判定用モデル読み込み */
			collisionModel_.Init("Assets/modelData/stage/FirstStage/firstStageCollision.tkm", nullptr, 0, enModelUpAxisZ);
			collisionModel_.SetPosition(Vector3::Zero);
			collisionModel_.SetRotation(Quaternion::Identity);
			collisionModel_.SetScale(Vector3(STAGE_SCALE, STAGE_SCALE, STAGE_SCALE));
			collisionModel_	.Update();

			/** 地面用モデル読み込み */
			groundModel_.Init("Assets/modelData/stage/FirstStage/Ground.tkm", nullptr, 0, enModelUpAxisZ);
			groundModel_.SetPosition(Vector3::Zero);
			groundModel_.SetRotation(Quaternion::Identity);
			groundModel_.SetScale(Vector3(STAGE_SCALE, STAGE_SCALE, STAGE_SCALE));
			groundModel_.Update();


			/** 静的な当たり判定を作る */
			/** 当たり判定用モデル */
			stageColision_.Release();
			stageColision_.CreateFromModel(collisionModel_.GetModel(), collisionModel_.GetModel().GetWorldMatrix());
			/** 地面用モデル */
			groundCollision_.Release();
			groundCollision_.CreateFromModel(groundModel_.GetModel(), groundModel_.GetModel().GetWorldMatrix());
		}


		Stage::~Stage()
		{
			/** 静的コリジョンを解放 */
			stageColision_.Release();
			groundCollision_.Release();
		}


		bool Stage::Start()
		{
			return true;
		}


		void Stage::Update()
		{
		}


		void Stage::Render(RenderContext& rc)
		{
			/** コリジョンモデル以外を描画 */
			stageModel_.Draw(rc);
			groundModel_.Draw(rc);
		}
	}
}