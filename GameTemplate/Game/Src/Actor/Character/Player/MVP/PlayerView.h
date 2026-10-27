#pragma once

#include "Weapon.h"
#include "Src/Actor/Character/Common/CharacterAnimation.h"
#include "Src/Actor/Character/Common/CharacterModel.h"
#include "Src/Data/PlayerStatusTable.h"

namespace nsApp
{
	namespace nsActor
	{
		class Player;

		/**
		 * @file   PlayerView.h
		 * @brief  操作キャラの見た目（MVP の V）。
		 * @details 体モデル・武器モデル・ライト・アニメ再生を担当する。
		 *          モデルパス登録は CharacterModel、アニメ一覧は CharacterAnimation / AnimBank。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/16
		 */
		class PlayerView
		{
		public:
			/* コンストラクタとデストラクタ。*/
			PlayerView() = default;
			virtual ~PlayerView() = default;


		public:
			/**
			 * @brief 所有者を結びつける。
			 * @param pPlayer 操作キャラ（非所有）。
			 */
			inline void Bind(Player* pPlayer)
			{
				pPlayer_ = pPlayer;
			}

			/**
			 * @brief 体モデルとアニメを初期化する。
			 * @param stStatus プレイヤーステータス（スケール・手ボーン名）。
			 * @param vPosition 初期座標。
			 * @details 移設元: Player::InitModel。
			 */
			void Init(const nsData::PlayerStatus& stStatus, const Vector3& vPosition);

			/**
			 * @brief 体の位置・回転をモデルへ反映する。
			 * @details 移設元: Player::UpdateModel の体部分。
			 */
			void Present();

			/**
			 * @brief 手に持つ武器モデルを更新する。
			 * @details 移設元: Player::UpdateWeaponModel。
			 */
			void PresentWeapon();

			/**
			 * @brief ライトの向きを更新する。
			 * @details 移設元: Player::UpdateFlashLight。
			 */
			void PresentFlashLight();

			/**
			 * @brief アニメーションを再生する。
			 * @param animState 対応するアニメーションの種類。
			 * @details 移設元: Player::PlayAnimation。
			 */
			void PlayAnimation(ANIM_LIST animState);

			/**
			 * @brief 体モデルを描画する（三人称のみ）。
			 * @param rc レンダリングコンテキスト。
			 */
			void RenderBody(RenderContext& rc);

			/**
			 * @brief 武器モデルを描画する。
			 * @param rc レンダリングコンテキスト。
			 */
			void RenderWeapon(RenderContext& rc);

			/**
			 * @brief 体モデルの表示サイズを取得する。
			 */
			inline float GetBodyModelSize() const
			{
				return bodyModelSize_;
			}

			/**
			 * @brief 右手ボーン ID を取得する。
			 */
			inline int GetRightHandBoneId() const
			{
				return rightHandBoneId_;
			}

			/**
			 * @brief 体の CharacterModel への参照。
			 */
			inline CharacterModel& GetModel()
			{
				return model_;
			}

			/**
			 * @brief 画面に出している銃の位置を取得する。
			 */
			inline const Vector3& GetWeaponViewPosition() const
			{
				return weaponViewPos_;
			}


		private:
			/**
			 * @brief 体モデルの実寸を測る。
			 * @details 移設元: Player::CalcBodyModelSize。
			 */
			void CalcBodyModelSize(const nsData::PlayerStatus& stStatus);

			/**
			 * @brief 銃を持たせるボーンの番号を、名前から一度だけ引く。
			 * @param sBoneName 探すボーンの名前（player.json の handBoneName / leftHandBoneName）。
			 * @return ボーン番号。モデルに無い名前なら -1。
			 */
			int FindHandBoneId(const std::string& sBoneName);

			/**
			 * @brief 銃を持たせるボーンのワールド座標を取り出す。
			 * @param iBoneId      取り出すボーンの番号（FindHandBoneId で引いたもの）。
			 * @param vOutPosition 取り出した座標の受け取り先（失敗時は触らない）。
			 * @return 取り出せたら true。ボーンが無いモデルでは false。
			 */
			bool TryGetHandBonePosition(int iBoneId, Vector3& vOutPosition);

			/**
			 * @brief 未ロードなら武器モデルを一度だけ読む。
			 */
			void LoadWeaponModelIfNeeded(nsWeapon::Weapon* pWeapon, int iType);

			/**
			 * @brief 未ロードなら二丁持ちの左手ぶんのモデルを一度だけ読む。
			 * @details 同じ武器種でも置き場所が2つ要るので、右手ぶんとは別の ModelRender を持つ。
			 */
			void LoadDualWeaponModelIfNeeded(nsWeapon::Weapon* pWeapon);

			/**
			 * @brief 自分の視点用に、銃を画面手前のビューモデルとして配置する。
			 * @details 移設元: Player::UpdateViewWeaponModel。
			 */
			void UpdateViewWeaponModel(nsWeapon::Weapon* pWeapon, int iType);

			/**
			 * @brief ビューモデル銃の中心位置を組み立てる。
			 * @param fSideSign 画面のどちら側へ置くか。右手なら+1、二丁持ちの左手なら-1。
			 */
			void BuildViewWeaponCenter(nsWeapon::Weapon* pWeapon, float fSideSign);

			/**
			 * @brief ビューモデル銃の回転を組み立てる。
			 * @param fSideSign 画面のどちら側へ置くか。傾ける向きが左右で反転する。
			 */
			void BuildViewWeaponRotation(nsWeapon::Weapon* pWeapon, float fSideSign);

			/**
			 * @brief ADS中の銃の縮小スケール込みの表示倍率。
			 */
			float CalcViewWeaponScale(nsWeapon::Weapon* pWeapon, int iType) const;

			/**
			 * @brief 他人から見た姿用に、銃を右手のボーンへ持たせる。二丁持ちなら左手のボーンにも持たせる。
			 * @details 移設元: Player::UpdateHandWeaponModel。
			 */
			void UpdateHandWeaponModel(nsWeapon::Weapon* pWeapon, int iType);

			/**
			 * @brief 手持ち銃の中心位置を組み立てる。
			 * @param iBoneId   基準にする手のボーンの番号。
			 * @param fSideSign 武器データの横のずらしをどちらへ向けるか。右手なら+1、二丁持ちの左手なら-1。
			 * @return 手のボーンを基準に置けたら true。ボーンが取れず腰のあたりへ置いたら false。
			 */
			bool BuildHandWeaponCenter(nsWeapon::Weapon* pWeapon, int iBoneId, float fSideSign);

			/**
			 * @brief 手持ち銃の回転を組み立てる。
			 */
			void BuildHandWeaponRotation();

			/**
			 * @brief 手持ち銃の実寸スケール。
			 */
			float CalcHandWeaponScale(nsWeapon::Weapon* pWeapon, int iType) const;

			/**
			 * @brief 武器モデルの中心が指定位置へ来るよう原点ズレを打ち消して配置する。
			 * @param stModel          配置するモデル。二丁持ちの左手ぶんもここへ渡す。
			 * @param iType            武器の種類(原点ズレと自動スケールの参照に使う)。
			 * @param fScale           表示倍率。
			 * @param bRecordPosition  置いた位置を控えるか。左右で2回呼ぶので、控えるのは右手ぶんだけ。
			 * @details 移設元: Player::PlaceWeaponModel。
			 */
			void PlaceWeaponModel(ModelRender& stModel, int iType, float fScale, bool bRecordPosition);

			/**
			 * @brief 武器モデルの AABB から中心と自動スケールを計算する。
			 * @details 移設元: Player::CalcWeaponModelFit。
			 */
			void CalcWeaponModelFit(int iType);


		private:
			Player* pPlayer_ = nullptr;				//! 所有者（非所有）。
			CharacterModel model_;					//! 体モデル。
			CharacterAnimation animation_;			//! アニメーションの再生とブレンド。
			ANIM_LIST playingAnimation_ = ANIM_LIST::Idle;	//! 再生中のアニメーション。
			Quaternion bodyAngle_ = Quaternion::Identity;	//! 体の向き。
			int rightHandBoneId_ = -1;				//! 銃を持たせる右手のボーン番号。
			int leftHandBoneId_ = -1;				//! 二丁持ちのとき左手の銃を持たせるボーン番号。
			float bodyModelSize_ = 0.0f;				//! 体モデルの実際の大きさ。
			float bodyModelScale_ = 1.0f;			//! Init 時に控えた表示スケール。
			ModelRender weaponModels_[static_cast<int>(nsWeapon::EnWeaponType::Num)];		//! 武器ごとのモデル。
			bool weaponModelLoaded_[static_cast<int>(nsWeapon::EnWeaponType::Num)] = {};	//! 読み込み済みか。
			Vector3 weaponModelCenter_[static_cast<int>(nsWeapon::EnWeaponType::Num)];		//! ローカルAABB中心。
			float weaponModelAutoScale_[static_cast<int>(nsWeapon::EnWeaponType::Num)] = {};	//! 自動スケール。
			float weaponModelLongestEdge_[static_cast<int>(nsWeapon::EnWeaponType::Num)] = {};	//! 一番長い辺。
			Vector3 weaponViewPos_ = Vector3::Zero;	//! 画面に出している銃の位置。
			Vector3 workWeaponCenter_ = Vector3::Zero;	//! 銃の置き位置の組み立て用。
			Quaternion workWeaponBase_ = Quaternion::Identity;	//! 原点ズレ補正用の基準回転。
			Quaternion workWeaponRot_ = Quaternion::Identity;	//! 銃へ与える回転。
			float workReloadLower_ = 0.0f;			//! リロードで銃を下げている度合い。
			float workReloadInsert_ = 0.0f;			//! リロードで挿し込む度合い。
			float workReloadSettle_ = 0.0f;			//! リロードで構え直す度合い。

			/*
			 * 二丁持ちの左手ぶん。weaponModels_ は武器の種類ごとに1つしか無く、
			 * 1つの ModelRender は1箇所にしか置けないので、同じモデルを2箇所へ出すために別口で持つ。
			 */
			ModelRender dualWeaponModel_;			//! 左手に持つ銃のモデル。
			bool bDualWeaponModelLoaded_ = false;	//! 左手ぶんを読み込み済みか。
			bool bDrawDualWeapon_ = false;			//! このフレーム、左手ぶんを描くか。
		};
	}
}