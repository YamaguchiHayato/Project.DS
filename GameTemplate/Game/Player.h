#pragma once
#include "Src/Actor/Character/ICharacter.h"
#include "Src/Actor/Character/Common/CharacterMovement.h"
#include "IPlayerController.h"
#include "WeaponInventory.h"
#include "Src/Event/GameEvent.h"

namespace nsApp
{
	namespace nsEvent { class EventBus; }	//! 前方宣言。

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
		 * @enum  EnLifeState
		 * @brief プレイヤーの生命状態(L4D2風の行動不能→救助/死亡)。
		 */
		enum class EnLifeState : uint8_t
		{
			Alive,	//! 生存(通常行動可能)。
			Down,	//! ダウン(行動不能・出血中。救助されれば復帰)。
			Dead,	//! 死亡。
		};

		/**
		 * @file   Player.h
		 * @brief  プレイヤーキャラクター。
		 *         PlayerInput(入力)とWeaponInventory(武器)を持ち、
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

		/* セッター。*/
		public:
			/**
			 * @brief キャラクターの位置を設定する。
			 * @param vPosition 設定する位置。
			 */
			inline void SetPosition(const Vector3& vPosition)
			{
				vPosition_ = vPosition;
				stMovement_.SetPosition(vPosition_);
			}


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

			/**
			 * @brief カメラのピッチ角(上下、ラジアン)を取得する。
			 * @return ピッチ角。
			 */
			inline float GetCameraPitch() const
			{
				return fCameraPitch_;
			}

			/**
			 * @brief 視線方向(ヨー＋ピッチの3D単位ベクトル)を取得する。
			 *        カメラ追従と射撃の照準で同じ向きを使うための共通ソース。
			 * @return 視線方向。
			 */
			Vector3 GetLookDirection() const;

			/**
			 * @brief 行動不能(ダウンまたは死亡)かどうか。
			 * @return 生存していなければ true。
			 */
			inline bool IsIncapacitated() const
			{
				return enLifeState_ != EnLifeState::Alive;
			}

			/**
			 * @brief 現在の生命状態を取得する。
			 * @return 生命状態。
			 */
			inline EnLifeState GetLifeState() const
			{
				return enLifeState_;
			}

			/**
			 * @brief 最大HPを取得する(UI表示用)。
			 * @return 最大HP。
			 */
			inline int GetMaxHP() const
			{
				return stCharacterStatus_.stHp_.iMaxHP_;
			}

			/**
			 * @brief ダウン中の出血残り時間を取得する(UI表示用)。
			 * @return 残り時間(秒)。
			 */
			inline float GetBleedOutRemain() const
			{
				return fBleedOutTimer_;
			}

			/**
			 * @brief 現在装備中の武器を取得する(UI表示用。無ければnullptr)。
			 * @return 現在の武器。
			 */
			inline nsWeapon::Weapon* GetEquippedWeapon()
			{
				return stWeaponInventory_.GetCurrentWeapon();
			}

			//! 所持している回復アイテム数(UI表示用)。
			inline int GetMedkitCount() const { return iMedkitCount_; }

			//! 所持している投擲アイテム数(UI表示用)。
			inline int GetGrenadeCount() const { return iGrenadeCount_; }

			/**
			 * @brief ダウン中のプレイヤーを救助して復帰させる(将来の味方/BOT用)。
			 *        ダウン中以外は何もしない。
			 */
			void Revive();


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
			 * @brief 突き飛ばし(近接)入力を処理する。前方近距離の敵を押し返す。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateShove(float fDeltaTime);

			/**
			 * @brief アイテム(回復/投擲)の入力を処理する。
			 */
			void UpdateItems();

			/**
			 * @brief 生命状態(生存/ダウン/死亡)を更新する。HP0でダウン、出血タイマー切れで死亡。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateLifeState(float fDeltaTime);

			/**
			 * @brief イベントバスへゲームイベントを発行する(バスが無ければ何もしない)。
			 * @param enType    発行する通知の種別。
			 * @param vPosition 出来事が起きた位置。
			 * @param vDirection 出来事の向き。
			 */
			void PublishGameEvent(nsEvent::EnGameEvent enType, const Vector3& vPosition = Vector3::Zero, const Vector3& vDirection = Vector3::Zero);

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
			CharacterMovement stMovement_;			//! 移動計算と壁との押し戻し。
			IPlayerController* pController_ = nullptr;	//! 操作意図の供給源(ローカル/ネットで差し替え可能)。
			PlayerIntent stIntent_;				//! このフレームの操作意図。
			nsWeapon::WeaponInventory stWeaponInventory_;	//! 武器の所持・切り替え担当。

			ModelRender stModelRender_;											//! プレイヤーモデル。
			AnimationClip aAnimationClip_[static_cast<int>(EnPlayerAnimation::Num)];	//! Idle/Walk/Run。

			ModelRender aWeaponModels_[static_cast<int>(nsWeapon::EnWeaponType::Num)];			//! 武器ごとのモデル(種類で添字)。切替のたびに再Initすると壊れるので一度だけ読み込む。
			bool aWeaponModelLoaded_[static_cast<int>(nsWeapon::EnWeaponType::Num)] = {};	//! 各武器モデルを読み込み済みか。
			//! 各武器モデルのローカルAABB中心(原点ズレ吸収用)。Vector3に既定コンストラクタが無いので要素数ぶん明示初期化する。
			Vector3 aWeaponModelCenter_[static_cast<int>(nsWeapon::EnWeaponType::Num)] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
			float aWeaponModelAutoScale_[static_cast<int>(nsWeapon::EnWeaponType::Num)] = {};//! 各武器モデルの自動サイズ合わせスケール。
			nsWeapon::EnWeaponType enEquippedType_ = nsWeapon::EnWeaponType::Handgun;			//! 現在装備中の武器の種類(描画対象)。

			Vector3 vPosition_ = Vector3::Zero;	//! 座標。
			Vector3 vForward_ = { 0.0f, 0.0f, 1.0f };	//! 向いている方向(=移動方向)。
			Quaternion qRotation_ = Quaternion::Identity;	//! モデルの回転。

			float fCameraYaw_ = 0.0f;					//! カメラの旋回角(ラジアン、マウスで操作)。
			float fCameraPitch_ = 0.0f;				//! カメラのピッチ角(上下、ラジアン。マウス操作、上下限あり)。

			float fMoveSpeed_ = 200.0f;				//! 移動速度(単位/秒)。
			bool bIsMoving_ = false;					//! 移動中か。
			bool bIsSprinting_ = false;				//! スプリント中か。
			float fBobTimer_ = 0.0f;					//! 歩行ボブの位相。
			float fBobWeight_ = 0.0f;					//! 歩行ボブの重み(移動→1/停止→0へ補間)。
			bool bIsLightOn_ = false;				//! ライトが点いているか。
			EnPlayerAnimation enPlayingAnimation_ = EnPlayerAnimation::Num;	//! 再生中のアニメーション。

			EnLifeState enLifeState_ = EnLifeState::Alive;	//! 生命状態(生存/ダウン/死亡)。
			float fBleedOutTimer_ = 0.0f;				//! ダウン中の出血残り時間(秒)。0で死亡。
			float fShoveCooldown_ = 0.0f;				//! 突き飛ばしのクールダウン残り(秒)。
			int iMedkitCount_ = 1;					//! 所持回復アイテム数。
			int iGrenadeCount_ = 2;					//! 所持投擲アイテム数。
			nsEvent::EventBus* pEventBus_ = nullptr;				//! イベント発行先(生成時にFindGOで取得。無ければ発行しない)。
		};
	}
}
