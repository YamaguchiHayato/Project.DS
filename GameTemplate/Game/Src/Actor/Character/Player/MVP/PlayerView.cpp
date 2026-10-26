#include "stdafx.h"
#include "Src/Actor/Character/Player/MVP/PlayerView.h"
#include "Src/Actor/Character/Player/Player.h"
#include "Src/System/ModelBounds.h"

namespace
{
	const float ANIM_INTERPOLATE_TIME = 0.2f;	//! アニメーション補間時間(秒)。
	const float VIEW_MODEL_RIGHT = 22.0f;		//! ビューモデル銃の右オフセット。
	const float VIEW_MODEL_DOWN = -26.0f;		//! ビューモデル銃の下オフセット(負で下)。
	const float TARGET_GUN_SIZE = 45.0f;		//! ビューモデル銃の目標サイズ(一番長い辺をこの長さに自動スケール)。
	const float HAND_HEIGHT_RATE = 0.55f;		//! 右手のボーンが見つからないときに銃を置く高さ(目線の高さに対する割合)。
	const float ADS_VIEW_MODEL_RIGHT = 6.0f;	//! 覗き込み時に銃を画面中央へ寄せた後の右オフセット。
	const float ADS_VIEW_MODEL_DOWN = -12.0f;	//! 覗き込み時の銃の下オフセット。
	const float SHOVE_MOTION_ANGLE = 0.25f;		//! 押したときに銃口が上を向く角度(ラジアン)。
	const int FLASH_LIGHT_INDEX = 0;			//! 手持ちライトに使うスポットライトの番号。
	const float FLASH_LIGHT_RANGE = 900.0f;		//! ライトが届く距離。
	const float FLASH_LIGHT_ANGLE = 0.5f;		//! ライトの広がり(ラジアン)。
	const float SPRINT_LOWER_ANGLE = 0.7f;		//! スプリント中に銃口を下げる角度(ラジアン)。走っている間は構えを解く。
	const float SPRINT_LOWER_DOWN = 18.0f;		//! スプリント中に銃を下げる距離。
	const float ADS_ROLL_ANGLE = 1.5708f;		//! 覗き込み時に銃を倒す角度(ラジアン。約90度)。視界を塞がないよう横倒しにする。
	const int HAND_BONE_NAME_MAX = 128;			//! ボーン名をワイド文字列へ変換するときの長さの上限。
}


namespace nsApp
{
	namespace nsActor
	{
		void PlayerView::Init(const nsData::PlayerStatus& stStatus, const Vector3& vPosition)
		{
			/* 扱うモデルの種類をセットする。*/
			animation_.Initialize(CharacterModelType::Survivor);
			/* アニメーションをロードする。*/
			animation_.LoadAnimation();
			/* モデルとアニメーションの種類を読み込む。*/
			model_.LoadCharacterModel(CharacterModelType::Survivor, animation_.GetAnimatiocClip(), animation_.GetAnimationClips());
			/* 座標を設定する。*/
			model_.SetPosition(vPosition);

			/*
			 * 体の大きさはステータス表で決める。
			 * 一人称では体を描かないので今まで見えていなかったが、三人称では他プレイヤーに見える。
			 * 敵や地面と大きさが揃わない場合は player.json の bodyModelScale で合わせる。
			 */
			bodyModelScale_ = stStatus.fBodyModelScale_;
			model_.SetCharacterScale(Vector3(bodyModelScale_, bodyModelScale_, bodyModelScale_));

			/* 実際の表示サイズを測っておく(三人称カメラの寄り引きに使う)。*/
			CalcBodyModelSize(stStatus);
			model_.Update();

			/*
			 * 三人称で銃を持たせるボーンを探しておく(毎フレーム名前で探さないため)。
			 * 名前は player.json の handBoneName。見つからなければ -1 になり、銃は腰のあたりへ置かれる。
			 */
			rightHandBoneId_ = FindHandBoneId(stStatus);
		}


		int PlayerView::FindHandBoneId(const nsData::PlayerStatus& stStatus)
		{
			/* モデルをまだ読めていなければ探せない。*/
			ModelRender* pModelRender = model_.GetModelRender();
			if (pModelRender == nullptr)
				return -1;

			/* スケルトン(tks)を持たないモデルでは、そもそもボーンが無い。*/
			Skeleton& skeleton = pModelRender->GetSkeleton();
			if (!skeleton.IsInited())
				return -1;

			/*
			 * FindBoneID はワイド文字列を取るので、ステータス表の名前を変換してから渡す。
			 * ナロー文字列(const char*)をワイド書式へ流し込むときは %hs を使う
			 * (%s だと wchar_t* 扱いになり文字化けする)。
			 */
			wchar_t wcBoneName[HAND_BONE_NAME_MAX];
			swprintf_s(wcBoneName, L"%hs", stStatus.sHandBoneName_.c_str());

			return skeleton.FindBoneID(wcBoneName);
		}


		bool PlayerView::TryGetHandBonePosition(Vector3& vOutPosition)
		{
			/* 名前が見つからなかったモデルでは使えない。*/
			if (rightHandBoneId_ < 0)
				return false;

			ModelRender* pModelRender = model_.GetModelRender();
			if (pModelRender == nullptr)
				return false;

			/* モデルを差し替えてボーンが減っている場合に備えて、番号の範囲を確かめる。*/
			Skeleton& skeleton = pModelRender->GetSkeleton();
			if (rightHandBoneId_ >= skeleton.GetNumBones())
				return false;

			Bone* pBone = skeleton.GetBone(rightHandBoneId_);
			if (pBone == nullptr)
				return false;

			/* ワールド行列の平行移動成分が、その骨のワールド座標。*/
			const Matrix& mBoneWorld = pBone->GetWorldMatrix();
			vOutPosition.x = mBoneWorld.m[3][0];
			vOutPosition.y = mBoneWorld.m[3][1];
			vOutPosition.z = mBoneWorld.m[3][2];

			return true;
		}


		void PlayerView::Present()
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/*
			 * 他人から見た姿のときは、体を「狙っている向き」へ向ける。
			 * どこを狙っているかが味方から見て分かるようにするため(L4D2のサバイバーと同じ)。
			 * 一人称では体を描かないので、進行方向を向いたままでよい。
			 * TODO: 横歩き・後ろ歩きのアニメーションが無いので、真横へ進むと足が滑って見える。
			 *       アニメーションが増えたら移動方向で出し分ける。
			 */
			bodyAngle_ = pPlayer_->GetRotation();
			if (pPlayer_->GetViewMode() == EnViewMode::ThirdPerson)
				bodyAngle_.SetRotationY(pPlayer_->GetCameraYaw());

			/* 位置と回転をモデルへ反映して更新する。*/
			model_.SetPosition(pPlayer_->GetPosition());
			model_.SettRotation(bodyAngle_);
			model_.Update();
		}


		void PlayerView::PresentWeapon()
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 現在装備している武器の種類をセットする。*/
			if (pPlayer_->GetWeaponInventory().GetCurrentWeapon() == nullptr)
				return;

			/* 装備中の武器の種類をプレイヤーにセットする。*/
			pPlayer_->SetEquippedType(pPlayer_->GetWeaponInventory().GetCurrentWeapon()->GetType());
			LoadWeaponModelIfNeeded(pPlayer_->GetWeaponInventory().GetCurrentWeapon(), static_cast<int>(pPlayer_->GetEquippedType()));

			/* 見せ方で銃の置き場所が変わる。自分の視点なら画面手前、他人から見た姿なら右手のボーン。*/
			if (pPlayer_->GetViewMode() == EnViewMode::ThirdPerson)
				UpdateHandWeaponModel(pPlayer_->GetWeaponInventory().GetCurrentWeapon(), static_cast<int>(pPlayer_->GetEquippedType()));
			else
				UpdateViewWeaponModel(pPlayer_->GetWeaponInventory().GetCurrentWeapon(), static_cast<int>(pPlayer_->GetEquippedType()));
		}


		void PlayerView::PresentFlashLight()
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/*
			 * 手持ちのライトを目線の位置から視線の先へ向ける。
			 * 消えているときは届く距離を0にして、光が出ないようにする。
			 */
			g_renderingEngine->SetSpotLight(FLASH_LIGHT_INDEX, pPlayer_->GetEyePosition(), { 1.0f, 0.95f, 0.85f }, pPlayer_->IsLightOn() ? FLASH_LIGHT_RANGE : 0.0f, pPlayer_->GetLookDirection(), FLASH_LIGHT_ANGLE);
		}


		void PlayerView::PlayAnimation(ANIM_LIST animState)
		{
			/* アニメーションが同じ場合は処理しない。*/
			if (playingAnimation_ == animState)
				return;

			/* アニメーションをセット。*/
			playingAnimation_ = animState;
			/* アニメーションを再生する。*/
			model_.PlayAnimation(animation_.GetBasicAnimationIndex(animState), ANIM_INTERPOLATE_TIME);
		}


		void PlayerView::RenderBody(RenderContext& rc)
		{
			/* playerが null の場合、処理を行わない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 3人称視点の場合、描画する。*/
			if (pPlayer_->GetViewMode() == EnViewMode::ThirdPerson)
				model_.DrawCharacterModel(rc);
		}


		void PlayerView::RenderWeapon(RenderContext& rc)
		{
			/* nullの場合、処理しない。*/
			if (pPlayer_ == nullptr)
				return;

			/* 装備中の武器の種類をプレイヤーにセットする。*/
			if (weaponModelLoaded_[static_cast<int>(pPlayer_->GetEquippedType())])
				weaponModels_[static_cast<int>(pPlayer_->GetEquippedType())].Draw(rc);

			/* 二丁持ちのときは左手ぶんも描く。*/
			if (bDrawDualWeapon_ && bDualWeaponModelLoaded_)
				dualWeaponModel_.Draw(rc);
		}


		void PlayerView::CalcBodyModelSize(const nsData::PlayerStatus& stStatus)
		{
			/* 頂点から実際の大きさを測る。取れなければ測れない。*/
			/* TODO: CharacterModel から Model が取れたら MeasureModelBounds する。*/
			bodyModelSize_ = 0.0f;
			(void)stStatus;
		}


		void PlayerView::LoadWeaponModelIfNeeded(nsWeapon::Weapon* pWeapon, int iType)
		{
			/* この武器のモデルをまだ読み込んでいなければ一度だけ読み込む(再Initは壊れるので避ける)。*/
			if (weaponModelLoaded_[iType])
				return;

			/* TODO: 銃モデルが横倒し等になる場合は enModelUpAxisY に変える。*/
			weaponModels_[iType].Init(pWeapon->GetModelPath(), nullptr, 0, enModelUpAxisZ);
			weaponModelLoaded_[iType] = true;

			/* 読み込み時に一度だけ、モデルの中心と自動サイズ合わせスケールを計算しておく。*/
			CalcWeaponModelFit(iType);
		}


		void PlayerView::LoadDualWeaponModelIfNeeded(nsWeapon::Weapon* pWeapon)
		{
			/* 左手ぶんは一度読めば使い回す(再Initは壊れるので避ける)。*/
			if (bDualWeaponModelLoaded_)
				return;

			dualWeaponModel_.Init(pWeapon->GetModelPath(), nullptr, 0, enModelUpAxisZ);
			bDualWeaponModelLoaded_ = true;
		}


		void PlayerView::UpdateViewWeaponModel(nsWeapon::Weapon* pWeapon, int iType)
		{
			/* FPSのビューモデルとして、カメラ(目)基準で画面手前(前・右・下)に銃を置く。*/
			BuildViewWeaponCenter(pWeapon, 1.0f);
			BuildViewWeaponRotation(pWeapon, 1.0f);
			PlaceWeaponModel(weaponModels_[iType], iType, CalcViewWeaponScale(pWeapon, iType), true);

			/* 1丁の銃はここまで。*/
			bDrawDualWeapon_ = pWeapon->IsDualWield();
			if (!bDrawDualWeapon_)
				return;

			/*
			 * 二丁持ちは左手ぶんをもう一度組み立てる。
			 * 横へずらす向きだけ反転させて同じ計算を通すので、
			 * 揺れ・反動・リロードの動きは左右で揃う。
			 */
			LoadDualWeaponModelIfNeeded(pWeapon);
			BuildViewWeaponCenter(pWeapon, -1.0f);
			BuildViewWeaponRotation(pWeapon, -1.0f);
			PlaceWeaponModel(dualWeaponModel_, iType, CalcViewWeaponScale(pWeapon, iType), false);
		}


		void PlayerView::BuildViewWeaponCenter(nsWeapon::Weapon* pWeapon, float fSideSign)
		{
			/* 銃の「中心」を置きたい位置(目の前・右・下)。目の位置は歩きの上下動を含む。*/
			workWeaponCenter_ = pPlayer_->GetEyePosition();
			workWeaponCenter_ += pPlayer_->GetLookDirection() * pWeapon->GetViewModelForward();	// 前(武器ごと)

			/* 覗き込むほど、銃を画面中央(照準の位置)へ寄せる。*/
			workWeaponCenter_ += Vector3(cosf(pPlayer_->GetCameraYaw()), 0.0f, -sinf(pPlayer_->GetCameraYaw())) * ((VIEW_MODEL_RIGHT + (ADS_VIEW_MODEL_RIGHT - VIEW_MODEL_RIGHT) * pPlayer_->GetAdsRate()) * fSideSign);	// 右(二丁持ちの左手ぶんは反転)
			workWeaponCenter_.y += VIEW_MODEL_DOWN + (ADS_VIEW_MODEL_DOWN - VIEW_MODEL_DOWN) * pPlayer_->GetAdsRate();	// 下(負)
			workWeaponCenter_.y -= SPRINT_LOWER_DOWN * pPlayer_->GetLowerRate();	// 走っている間はさらに下げる

			/*
			 * リロードは「マガジンを抜く→挿す→構え直す」の3段階で見せる。
			 * 銃を手前・下へ引いて傾け(抜く)、下で一度突き上げ(挿す)、
			 * 元へ戻して最後に少し行き過ぎる(構え直す)。
			 */
			workReloadLower_ = 0.0f;
			workReloadInsert_ = 0.0f;
			workReloadSettle_ = 0.0f;
			pPlayer_->CalcReloadMotion(pWeapon->GetReloadRate(), workReloadLower_, workReloadInsert_, workReloadSettle_);

			workWeaponCenter_.y -= pPlayer_->GetPlayerStatus().stReloadMotion_.fLowerDown_ * workReloadLower_;	// 抜く動作で沈める
			workWeaponCenter_ -= pPlayer_->GetLookDirection() * (pPlayer_->GetPlayerStatus().stReloadMotion_.fPullBack_ * workReloadLower_);	// 手前へ引き寄せる
			workWeaponCenter_ -= Vector3(cosf(pPlayer_->GetCameraYaw()), 0.0f, -sinf(pPlayer_->GetCameraYaw())) * (pPlayer_->GetPlayerStatus().stReloadMotion_.fPullRight_ * workReloadLower_ * fSideSign);	// 画面中央へ寄せる
			workWeaponCenter_.y += pPlayer_->GetPlayerStatus().stReloadMotion_.fInsertUp_ * workReloadInsert_;	// 挿し込みの突き上げ
			workWeaponCenter_.y += pPlayer_->GetPlayerStatus().stReloadMotion_.fSettleUp_ * workReloadSettle_;	// 構え直しの行き過ぎ

			workWeaponCenter_ -= pPlayer_->GetLookDirection() * pPlayer_->GetWeaponKickBack();	// 撃った直後は手前へ下がる

			/* 突き飛ばしは銃を前へ突き出して見せる。*/
			workWeaponCenter_ += pPlayer_->GetLookDirection() * (pPlayer_->GetPlayerStatus().fShoveMotionForward_ * pPlayer_->GetShoveMotion());
			workWeaponCenter_.y += pPlayer_->GetPlayerStatus().fShoveMotionUp_ * pPlayer_->GetShoveMotion();

			/* 視点を振ったときの遅れ。腕が視点に引きずられているように見せる。*/
			workWeaponCenter_ += Vector3(cosf(pPlayer_->GetCameraYaw()), 0.0f, -sinf(pPlayer_->GetCameraYaw())) * pPlayer_->GetSwayRight();
			workWeaponCenter_.y += pPlayer_->GetSwayUp();

			/*
			 * 歩行ボブ(銃ぶん)。カメラ自体の上下動は GetEyePosition に含まれているので、
			 * ここで足すのは「体の動きに対して銃がさらに遅れて振れる」ぶん。
			 * 位相をずらしてあるので、カメラと同時に動いて画面上で消えることはない。
			 * 横は1歩に1往復、縦は1歩に2往復＝8の字を描く。
			 * 覗き込み中は銃の揺れを完全に止める(カメラ側の揺れは adsSuppressRate ぶんだけ残る)。
			 */
			workWeaponCenter_ += Vector3(cosf(pPlayer_->GetCameraYaw()), 0.0f, -sinf(pPlayer_->GetCameraYaw())) * (sinf(pPlayer_->GetBobTimer() - pPlayer_->GetPlayerStatus().stViewShake_.fBobPhaseLag_) * ((pPlayer_->IsSprinting() ? pPlayer_->GetPlayerStatus().stViewShake_.fBobSprintAmp_ : pPlayer_->GetPlayerStatus().stViewShake_.fBobWalkAmp_) * pPlayer_->GetBobWeight() * (1.0f - pPlayer_->GetAdsRate())));
			workWeaponCenter_.y += sinf((pPlayer_->GetBobTimer() - pPlayer_->GetPlayerStatus().stViewShake_.fBobPhaseLag_) * 2.0f) * ((pPlayer_->IsSprinting() ? pPlayer_->GetPlayerStatus().stViewShake_.fBobSprintAmp_ : pPlayer_->GetPlayerStatus().stViewShake_.fBobWalkAmp_) * pPlayer_->GetBobWeight() * (1.0f - pPlayer_->GetAdsRate())) * pPlayer_->GetPlayerStatus().stViewShake_.fBobWeaponUpRate_;
		}


		void PlayerView::BuildViewWeaponRotation(nsWeapon::Weapon* pWeapon, float fSideSign)
		{
			(void)pWeapon;

			/* 原点ズレの補正に使う基準の回転(視点の向きだけ)。*/
			workWeaponBase_.SetRotationY(pPlayer_->GetCameraYaw());
			workWeaponBase_.AddRotationX(-pPlayer_->GetCameraPitch());

			/* 実際に銃へ与える回転。ここへ演出ぶんを重ねていく。*/
			workWeaponRot_ = workWeaponBase_;
			workWeaponRot_.AddRotationZ(ADS_ROLL_ANGLE * pPlayer_->GetAdsRate() * fSideSign);	// 覗き込むほど銃を横倒しにし、拡大時に視界を塞がないようにする(左右で逆へ倒す)。
			workWeaponRot_.AddRotationX(SPRINT_LOWER_ANGLE * pPlayer_->GetLowerRate());	// 走っている間は銃口を下げて構えを解く。
			workWeaponRot_.AddRotationX(pPlayer_->GetPlayerStatus().stReloadMotion_.fLowerAngle_ * workReloadLower_);	// マガジンを抜くあいだは銃口を下げる。
			workWeaponRot_.AddRotationZ(pPlayer_->GetPlayerStatus().stReloadMotion_.fRollAngle_ * workReloadLower_ * fSideSign);	// 差込口が見えるよう銃を傾ける(左右で逆へ倒す)。
			workWeaponRot_.AddRotationX(-pPlayer_->GetPlayerStatus().stReloadMotion_.fInsertAngle_ * workReloadInsert_);	// 挿し込む瞬間だけ銃口が持ち上がる。
			workWeaponRot_.AddRotationZ(pPlayer_->GetSwayRight() * pPlayer_->GetPlayerStatus().stViewShake_.fSwayRollRate_);	// 視点を振った遅れのぶん傾ける。
			workWeaponRot_.AddRotationX(-SHOVE_MOTION_ANGLE * pPlayer_->GetShoveMotion());	// 突き飛ばした瞬間は銃口が上を向く。
		}


		float PlayerView::CalcViewWeaponScale(nsWeapon::Weapon* pWeapon, int iType) const
		{
			/*ADS中は銃を少し小さくして、照準周辺の視界を確保する。*/
			return weaponModelAutoScale_[iType] * pWeapon->GetModelScale() * (1.0f - 0.25f * pPlayer_->GetAdsRate());
		}


		void PlayerView::UpdateHandWeaponModel(nsWeapon::Weapon* pWeapon, int iType)
		{
			/*
			 * 銃は視線の向きに構える。体の向きではなく視線を使うので、狙っている方向がそのまま出る。
			 * TODO: 二丁持ちでも三人称では1丁しか出ない。左手のボーンを引いてもう1丁置けるようにする。
			 */
			bDrawDualWeapon_ = false;
			BuildHandWeaponCenter(pWeapon);
			BuildHandWeaponRotation();
			PlaceWeaponModel(weaponModels_[iType], iType, CalcHandWeaponScale(pWeapon, iType), true);
		}


		void PlayerView::BuildHandWeaponCenter(nsWeapon::Weapon* pWeapon)
		{
			/*
			 * 銃を置く基準は体の右手のボーン。
			 * ボーンのワールド行列は ModelRender::Update が計算しているので、
			 * 体のモデルを更新したあとに呼ぶこと(Present → PresentWeapon の順で呼んでいる)。
			 */
			if (!TryGetHandBonePosition(workWeaponCenter_))
			{
				/* ボーンが見つからないモデルでは、目線の高さから割合で腰のあたりを出す。*/
				workWeaponCenter_ = pPlayer_->GetPosition();
				workWeaponCenter_.y += pPlayer_->GetPlayerStatus().fEyeHeight_ * HAND_HEIGHT_RATE;
			}

			/* 銃ごとにグリップの位置が違うので、武器データのぶんだけずらして手に収める。*/
			workWeaponCenter_ += pPlayer_->GetLookDirection() * pWeapon->GetHandForward();
			workWeaponCenter_ += Vector3(cosf(pPlayer_->GetCameraYaw()), 0.0f, -sinf(pPlayer_->GetCameraYaw())) * pWeapon->GetHandRight();
			workWeaponCenter_.y += pWeapon->GetHandUp();
		}


		void PlayerView::BuildHandWeaponRotation()
		{
			/* 銃口が視線を向くように回す(ビューモデルと同じく、ヨーを設定してからピッチを後乗せする)。*/
			workWeaponBase_.SetRotationY(pPlayer_->GetCameraYaw());
			workWeaponBase_.AddRotationX(-pPlayer_->GetCameraPitch());

			/* 手に持たせるときは演出の回転を重ねないので、補正の基準も同じ回転でよい。*/
			workWeaponRot_ = workWeaponBase_;
		}


		float PlayerView::CalcHandWeaponScale(nsWeapon::Weapon* pWeapon, int iType) const
		{
			/*
			 * 手に持たせる銃は「世界の中での実寸」で決める。
			 * ビューモデルは画面に大きく映すため誇張しているので、そちらの倍率は使わない。
			 * 体を大きくすると手のボーンの位置も一緒に動くが、銃の大きさは動かないので
			 * ここで体の大きさに見合う長さを指定しておく必要がある。
			 */
			return (weaponModelLongestEdge_[iType] > 0.0001f) ? (pWeapon->GetHandLength() / weaponModelLongestEdge_[iType]) : 1.0f;
		}


		void PlayerView::PlaceWeaponModel(ModelRender& stModel, int iType, float fScale, bool bRecordPosition)
		{
			/*
			 * モデル原点のズレを打ち消す。
			 * 補正には視点の向き(ヨー＋ピッチ)だけを使い、演出でつけた回転は含めない。
			 * 演出の回転まで含めると、リロードのように大きく回したときに銃の位置まで動いてしまう。
			 */
			weaponViewPos_ = weaponModelCenter_[iType] * fScale;
			workWeaponBase_.Apply(weaponViewPos_);

			weaponViewPos_ = workWeaponCenter_ - weaponViewPos_;

			/* 不具合を調べられるよう、実際に置いた位置を控えておく(二丁持ちでは右手ぶんだけ)。*/
			if (bRecordPosition)
				pPlayer_->SetWeaponViewPosition(weaponViewPos_);

			stModel.SetPosition(weaponViewPos_);
			stModel.SetRotation(workWeaponRot_);
			stModel.SetScale(Vector3(fScale, fScale, fScale));
			stModel.Update();
		}


		void PlayerView::CalcWeaponModelFit(int iType)
		{
			/* 頂点から実際の大きさを測る。取れなかった場合は無補正(中心0・スケール1)にする。*/
			if (!nsSystem::MeasureModelBounds(weaponModels_[iType].GetModel()).bIsValid_)
			{
				weaponModelCenter_[iType] = Vector3::Zero;
				weaponModelAutoScale_[iType] = 1.0f;
				weaponModelLongestEdge_[iType] = 0.0f;
				return;
			}

			/* 中心は原点ズレの打ち消しに使う。*/
			weaponModelCenter_[iType] = nsSystem::MeasureModelBounds(weaponModels_[iType].GetModel()).GetCenter();

			/* 一番長い辺を TARGET_GUN_SIZE に合わせる自動スケール。*/
			weaponModelAutoScale_[iType] = (nsSystem::MeasureModelBounds(weaponModels_[iType].GetModel()).GetLongestEdge() > 0.0001f) ? (TARGET_GUN_SIZE / nsSystem::MeasureModelBounds(weaponModels_[iType].GetModel()).GetLongestEdge()) : 1.0f;
			weaponModelLongestEdge_[iType] = nsSystem::MeasureModelBounds(weaponModels_[iType].GetModel()).GetLongestEdge();

			/* デバッグ: AABBに余分なジオメトリが混じっていないか確認する。*/
			DebugPrintW(L"[Player] weapon%d size(%.1f,%.1f,%.1f) center(%.1f,%.1f,%.1f) autoScale=%.4f\n", iType, nsSystem::MeasureModelBounds(weaponModels_[iType].GetModel()).GetSize().x, nsSystem::MeasureModelBounds(weaponModels_[iType].GetModel()).GetSize().y, nsSystem::MeasureModelBounds(weaponModels_[iType].GetModel()).GetSize().z, nsSystem::MeasureModelBounds(weaponModels_[iType].GetModel()).GetCenter().x, nsSystem::MeasureModelBounds(weaponModels_[iType].GetModel()).GetCenter().y, nsSystem::MeasureModelBounds(weaponModels_[iType].GetModel()).GetCenter().z, weaponModelAutoScale_[iType]);
		}
	}
}