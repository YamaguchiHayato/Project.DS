#pragma once
#include "Src/Actor/Character/ICharacter.h"
#include "IPlayerController.h"
#include "WeaponManager.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @enum EnPlayerAnimation
		 * @brief プレイヤーの基本アニメーション。ModelRenderに渡す配列の添字になる。
		 */
		enum class EnPlayerAnimation : uint8_t
		{
			Idle,	//! 待機。
			Walk,	//! 歩き。
			Run,	//! 走り。
			Num,	//! アニメーションの数。
		};

		/**
		 * @file   Player.h
		 * @brief  プレイヤーキャラクター。
		 *         PlayerInput(入力)とWeaponManager(武器)を持ち、
		 *         入力結果を見て移動と発射を実行する司令塔の役割に徹する。
		 *         操作は本家(L4D2)準拠のキーボード＆マウス(nsK2EngineLow::Mouse経由)。
		 *         射撃=左クリック、武器切り替え=マウスホイール、リロード=R。
		 * @author Izumida Kiryu
		 * @date   2026/08/19
		 */
		class Player : public ICharacter
		{
		public:
			/* コンストラクタとデストラクタ。*/
			Player();
			virtual ~Player();


		public:
			/* ライフサイクル。*/
			virtual bool Start() override;
			virtual void Update() override;
			virtual void Render(RenderContext& rc) override;


		/* ゲッター。*/
		public:
			/**
			 * @brief キャラクターの位置を取得する。
			 * @return キャラクターの位置。
			 */
			virtual Vector3& GetPosition() override
			{
				return vPosition_;
			}

			/**
			 * @brief 向いている方向(=移動方向)を取得する。
			 * @return 正面方向ベクトル。
			 */
			inline const Vector3& GetForward() const
			{
				return vForward_;
			}

			/**
			 * @brief カメラの旋回角(ラジアン)を取得する。
			 * @return カメラのヨー角。シーン側が追従カメラの向きに使う。
			 */
			inline float GetCameraYaw() const
			{
				return fCameraYaw_;
			}


		private:
			/**
			 * @brief モデルとアニメーションを読み込む。
			 */
			void InitModel();

			/**
			 * @brief 入力結果を見て移動する。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateMove(float fDeltaTime);

			/**
			 * @brief 入力結果を見て武器の更新・発射・切り替えを行う。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateWeapon(float fDeltaTime);

			/**
			 * @brief 入力結果を見てインタラクト・ライトなどその他の操作を処理する。
			 */
			void UpdateAction();

			/**
			 * @brief 移動状態に応じてモデルの位置・回転・アニメーションを更新する。
			 */
			void UpdateModel();

			/**
			 * @brief 手に持つ武器モデルを現在の武器に合わせて更新する
			 *        (武器切替時の読み込み直し＋手元への配置)。
			 */
			void UpdateWeaponModel();

			/**
			 * @brief 武器モデルのローカルAABBから中心と自動スケールを計算して保存する。
			 *        銃ごとに原点のズレや基準サイズが違うのを自動で吸収するため、読み込み時に一度だけ呼ぶ。
			 * @param iType 武器種類(配列の添字)。
			 */
			void CalcWeaponModelFit(int iType);

			/**
			 * @brief アニメーションを切り替える(同じものが指定されたら何もしない)。
			 * @param enAnimation 再生するアニメーション。
			 */
			void PlayAnimation(EnPlayerAnimation enAnimation);


		private:
			IPlayerController*			pController_ = nullptr;	//! 操作意図の供給源(ローカル/ネットで差し替え可能)。
			PlayerIntent				stIntent_;				//! このフレームの操作意図。
			nsWeapon::WeaponManager		stWeaponManager_;	//! 武器の所持・切り替え担当。

			ModelRender	stModelRender_;											//! プレイヤーモデル。
			AnimationClip aAnimationClip_[static_cast<int>(EnPlayerAnimation::Num)];	//! Idle/Walk/Run。

			ModelRender	aWeaponModels_[static_cast<int>(nsWeapon::EnWeaponType::Num)];			//! 武器ごとのモデル(種類で添字)。切替のたびに再Initすると壊れるので一度だけ読み込む。
			bool		aWeaponModelLoaded_[static_cast<int>(nsWeapon::EnWeaponType::Num)] = {};	//! 各武器モデルを読み込み済みか。
			//! 各武器モデルのローカルAABB中心(原点ズレ吸収用)。Vector3に既定コンストラクタが無いので要素数ぶん明示初期化する。
			Vector3		aWeaponModelCenter_[static_cast<int>(nsWeapon::EnWeaponType::Num)] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
			float		aWeaponModelAutoScale_[static_cast<int>(nsWeapon::EnWeaponType::Num)] = {};//! 各武器モデルの自動サイズ合わせスケール。
			nsWeapon::EnWeaponType	enEquippedType_ = nsWeapon::EnWeaponType::Handgun;			//! 現在装備中の武器の種類(描画対象)。

			Vector3		vPosition_ = { 0.0f, 0.0f, 0.0f };	//! 座標。
			Vector3		vForward_ = { 0.0f, 0.0f, 1.0f };	//! 向いている方向(=移動方向)。
			Quaternion	qRotation_ = Quaternion::Identity;	//! モデルの回転。

			float		fCameraYaw_ = 0.0f;					//! カメラの旋回角(ラジアン、マウスで操作)。

			float		fMoveSpeed_ = 200.0f;				//! 移動速度(単位/秒)。
			bool		bIsMoving_ = false;					//! 移動中か。
			bool		bIsLightOn_ = false;				//! ライトが点いているか。
			EnPlayerAnimation	enPlayingAnimation_ = EnPlayerAnimation::Num;	//! 再生中のアニメーション。
		};
	}
}
