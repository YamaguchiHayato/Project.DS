#pragma once
#include "Src/Actor/Character/ICharacter.h"
#include "Src/Actor/Character/Common/CharacterMovement.h"
#include "Src/Actor/Character/Common/CharacterAnimation.h"
#include "Src/Actor/Character/Player/MVP/PlayerSystems.h"
#include "Src/Actor/Character/Player/Transition/EnPlayerState.h"
#include "Src/Actor/Character/Player/Transition/PlayerTransition.h"

#include "IPlayerController.h"
#include "WeaponInventory.h"
#include "Src/Event/GameEvent.h"
#include "Src/Data/PlayerStatusTable.h"
#include "Src/Combat/HitBoxSet.h"
#include "Src/StateMachine/StateMachine.h"

namespace nsApp
{
	namespace nsEvent { class EventBus; }	//! 前方宣言。
	namespace nsItem { class Pickup; }		//! 前方宣言。

	namespace nsActor
	{
		class CommonEnemy;	//! 前方宣言。

		/**
		 * @enum  EnViewMode
		 * @brief この Player をどう見せるか。
		 *        マルチを想定して、自分の画面と他プレイヤーの画面で見せ方を切り替えるための区分。
		 */
		enum class EnViewMode : uint8_t
		{
			FirstPerson,	//! 自分の視点。体は描かず、銃は画面手前のビューモデルとして置く。
			ThirdPerson,	//! 他人から見た姿。体を描き、銃を右手のボーンへ持たせる。
		};

		/**
		 * @enum  EnLifeState
		 * @brief プレイヤーの生命状態(L4D2風の行動不能→救助/死亡)。
		 */
		enum class EnLifeState : uint8_t
		{
			Alive,	//! 生存(通常行動可能)。
			Down,	//! ダウン(移動不能・出血中。サブ武器だけ撃てる。救助されれば復帰)。
			Dead,	//! 死亡。
		};

		/**
		 * @enum  EnQuickItem
		 * @brief 即効アイテム(本家の5番スロット)。どちらか1つだけ持てる。
		 */
		enum class EnQuickItem : uint8_t
		{
			None,		//! 持っていない。
			Pills,		//! 鎮痛剤。一時体力を大きく増やす。
			Adrenaline,	//! アドレナリン。一時体力を少し増やし、しばらく速く動ける。
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
			Player() = default;
			virtual ~Player();


		public:
			/* ライフサイクル。*/
			virtual bool Start() override;
			virtual void Update() override;
			virtual void Render(RenderContext& rc) override;

			/**
			 * @brief ダメージを受ける(L4D2式)。
			 *        一時体力から先に削り、残りを恒久HPから引く。
			 *        恒久HPが尽きるとダウンし、白黒(復帰回数を使い切った状態)ならそのまま死亡する。
			 *        ダウン中に受けたダメージは出血時間を縮める。
			 * @param iDamage ダメージ量。
			 */
			virtual void ApplyDamage(int iDamage) override;


			/* セッター。*/
		public:

			/**
			 * @brief アドレナリンの残り時間(秒)を設定する。
			 * @param fValue 設定する残り時間(秒)。
			 */
			inline void SetAdrenalineTimer(float fValue)
			{
				fAdrenalineTimer_ = fValue;
			}

			/**
			 * @brief カメラのピッチ角を設定する。マウスの上下移動で呼ばれる。
			 * @param pitch 設定するピッチ角(ラジアン)。
			 */
			inline void SetCameraPitch(float pitch)
			{
				fCameraPitch_ = pitch;
			}

			/**
			 * @brief しゃがみ中かを設定する。
			 * @param bValue 設定する値。trueでしゃがみ中、falseで立ち。
			 */
			inline void SetCrouching(bool bValue)
			{
				bIsCrouching_ = bValue;
			}

			/**
			 * @brief 装備中の武器の種類を設定する。
			 * @param enType 設定する装備中の武器の種類。
			 */
			inline void SetEquippedType(nsWeapon::EnWeaponType enType)
			{
				enEquippedType_ = enType;
			}

			/**
			 * @brief メディキットで回復中かを設定する。
			 * @param bValue 設定する値。trueで回復中、falseで回復していない。
			 */
			inline void SetHealing(bool bValue)
			{
				bIsHealing_ = bValue;
			}

			/**
			 * @brief メディキットの使用進行度(0〜1)を設定する。
			 * @param fValue 設定する進行度。
			 */
			inline void SetMedkitProgress(float fValue)
			{
				fMedkitProgress_ = fValue;
			}

			/**
			 * @brief 移動中かを設定する。
			 * @param bValue 設定する値。trueで移動中、falseで停止。
			 */
			inline void SetMoving(bool bValue)
			{
				bIsMoving_ = bValue;
			}

			/**
			 * @brief キャラクターの位置を設定する。
			 * @param vPosition 設定する位置。
			 */
			inline void SetPosition(const Vector3& vPosition)
			{
				vPosition_ = vPosition;
				stMovement_.SetPosition(vPosition_);
			}

			/**
			 * @brief 即効アイテム(鎮痛剤/アドレナリン)の所持状態を設定する(UI表示用)。
			 * @param enValue 設定する即効アイテムの種類。持っていなければ EnQuickItem::None。
			 */
			inline void SetQuickItem(EnQuickItem enValue)
			{
				enQuickItem_ = enValue;
			}

			/**
			 * @brief 復帰回数を設定する。復帰回数を使い切ると次にダウンしたとき死亡する。
			 * @param iValue 設定する復帰回数。
			 */
			inline void SetReviveCount(int iValue)
			{
				iReviveCount_ = iValue;
			}

			/**
			 * @brief 突き飛ばしのクールダウン時間を設定する。
			 * @param fValue 設定する時間(秒)。
			 */
			inline void SetShoveCooldown(float fValue)
			{
				fShoveCooldown_ = fValue;
			}

			/**
			 * @brief 突き飛ばしの疲労度を設定する。連続で突き飛ばすと疲労が溜まり、移動速度が落ちる。
			 * @param fValue 設定する量。
			 */
			inline void SetShoveMotion(float fValue)
			{
				fShoveMotion_ = fValue;
			}

			/**
			 * @brief 突き飛ばしの休止時間を設定する。
			 * @param fValue 設定する時間(秒)。
			 */
			inline void SetShoveRestTimer(float fValue)
			{
				fShoveRestTimer_ = fValue;
			}

			/**
			 * @brief 射撃の精度を設定する。0.0fで精密、1.0fで大雑把。
			 * @param fValue 設定する精度。
			 */
			inline void SetSpreadShot(float fValue)
			{
				fSpreadShot_ = fValue;
			}

			/**
			 * @brief スプリント中かを設定する。
			 * @param bValue 設定する値。trueでスプリント中、falseで通常歩き。
			 */
			inline void SetSprinting(bool bValue)
			{
				bIsSprinting_ = bValue;
			}

			/**
			 * @brief 歩きの揺れの左右成分を設定する。
			 * @param fValue 設定する量。
			 */
			inline void SetSwayRight(float fValue)
			{
				fSwayRight_ = fValue;
			}

			/**
			 * @brief 歩きの揺れの上下成分を設定する。
			 * @param fValue 設定する量。
			 */
			inline void SetSwayUp(float fValue)
			{
				fSwayUp_ = fValue;
			}

			/**
			 * @brief 一時体力を設定する(UI表示用)。時間で減っていくぶん。
			 * @param fValue 設定する一時体力。
			 */
			inline void SetTempHP(float fValue)
			{
				fTempHP_ = fValue;
			}

			/**
			 * @brief 歩きの揺れのタイマーを設定する。
			 * @param fValue 設定する量。
			 */
			inline void SetViewBobHeight(float fValue)
			{
				fViewBobHeight_ = fValue;
			}

			/**
			 * @brief 見せ方(自分の視点か、他人から見た姿か)を設定する。
			 *        マルチでは自分の機体だけ FirstPerson、他プレイヤーは ThirdPerson になる。
			 * @param enViewMode 設定する見せ方。
			 */
			inline void SetViewMode(EnViewMode enViewMode)
			{
				enViewMode_ = enViewMode;
			}

			/**
			 * @brief 歩きの揺れのタイマーを設定する。
			 * @param fValue 設定する量。
			 */
			inline void SetViewRoll(float fValue)
			{
				fViewRoll_ = fValue;
			}

			/**
			 * @brief 銃のキックバック量を設定する。
			 * @param fValue 設定する量。
			 */
			inline void SetWeaponKickBack(float fValue)
			{
				fWeaponKickBack_ = fValue;
			}

			/**
			 * @brief 武器のビューモデルの位置を設定する。
			 * @param vPos 設定する位置。
			 */
			inline void SetWeaponViewPosition(const Vector3& vPos)
			{
				vWeaponViewPos_ = vPos;
			}

			/* ゲッター。*/
		public:
		
			/**
			 * @brief 手の動き(リロード・回復)の速さの倍率を求める(アドレナリン中は速い)。
			 * @return 倍率。通常は1。
			 */
			inline float GetActionSpeedRate() const
			{
				return IsAdrenalineActive() ? stPlayerStatus_.stHealthRule_.fAdrenalineActionRate_ : 1.0f;
			}
		
			/**
			 * @brief アドレナリンの残り時間(秒)を取得する(UI表示用)。
			 * @return 残り時間(秒)。
			 */
			inline float GetAdrenalineRemain() const
			{
				return fAdrenalineTimer_;
			}
		
			/**
			 * @brief アドレナリンの残り時間(秒)を取得する。
			 * @return 残り時間(秒)。
			 */
			inline float GetAdrenalineTimer() const
			{
				return fAdrenalineTimer_;
			}
		
			/**
			 * @brief 覗き込み(ADS)の進行度を取得する。0=覗いていない、1=覗き切った状態。
			 * @return 進行度(0〜1)。
			 */
			inline float GetAdsRate() const
			{
				return fAdsRate_;
			}
		
			/**
			 * @brief 覗き込み(ADS)の倍率を取得する。武器のADS倍率と掛け合わせて使う。
			 * @return 倍率。1より小さいほどズームしている。
			 */
			float GetAdsZoomRate();
		
			/**
			 * @brief ダウン中の出血残り時間を取得する(UI表示用)。
			 * @return 残り時間(秒)。
			 */
			inline float GetBleedOutRemain() const
			{
				return fBleedOutTimer_;
			}
		
			/**
			 * @brief 歩きの揺れのタイマーを取得する。武器の揺れ倍率と掛け合わせて使う。
			 * @return タイマー。0〜1で1周期。歩きの揺れの位相を決める。
			 */
			inline float GetBobTimer() const
			{
				return fBobTimer_;
			}
	
			/**
			 * @brief 歩きの揺れの強さを取得する。武器の揺れ倍率と掛け合わせて使う。
			 * @return 強さ。0に近いほど揺れが小さく、1に近いほど揺れが大きい。
			 */
			inline float GetBobWeight() const
			{
				return fBobWeight_;
			}
	
			/**
			 * @brief 体モデルの実際の大きさ(一番長い辺)を取得する。
			 * @return 大きさ。
			 */
			inline float GetBodyModelSize() const
			{
				return systems_.GetView().GetBodyModelSize();
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
			 * @brief カメラの旋回角(ラジアン)を取得する。
			 * @return カメラのヨー角。シーン側が追従カメラの向きに使う。
			 */
			inline float GetCameraYaw() const
			{
				return fCameraYaw_;
			}
	
			/**
			 * @brief 現在の銃の拡散度を取得する。ADS率と武器の拡散度を掛け合わせた値。
			 * @return 拡散度(0〜1)。0に近いほど正確で、1に近いほどブレる。
			 */
			inline float GetCurrentSpread() const
			{
				return systems_.GetCalc().GetCurrentSpread();
			}
	
			/**
			 * @brief 装備中の武器の種類を取得する。
			 * @return 装備中の武器の種類。
			 */
			inline nsWeapon::EnWeaponType GetEquippedType() const
			{
				return enEquippedType_;
			}
	
			/**
			 * @brief 現在装備中の武器を取得する(UI表示用。無ければnullptr)。
			 * @return 現在の武器。
			 */
			inline nsWeapon::Weapon* GetEquippedWeapon()
			{
				return stWeaponInventory_.GetCurrentWeapon();
			}
	
			/**
			 * @brief 目の高さを取得する。しゃがむと低くなる。
			 * @return 目の高さ。
			 */
			inline float GetEyeHeight() const
			{
				return fEyeHeight_;
			}
	
			/**
			 * @brief 目(カメラ)の位置を取得する。歩きの上下動を含む。
			 *        カメラの位置・射撃の起点・ビューモデルの基準を必ずこれで揃える。
			 *        別々に計算するとクロスヘアと着弾がズレるため。
			 * @return 目の位置。
			 */
			inline Vector3 GetEyePosition() const
			{
				Vector3 vEyePosition = vPosition_;
				vEyePosition.y += fEyeHeight_ + fViewBobHeight_;
	
				return vEyePosition;
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
			 * @brief メディキットの使用進行度(0〜1)。押し続けると進み、離す・動く・撃つと最初からになる。
			 * @return 進行度。
			 */
			inline float GetHealProgress() const
			{
				return fMedkitProgress_;
			}
	
			/**
			 * @brief プレイヤーの意図を取得する。移動・射撃・回復などの入力結果。
			 * @return プレイヤーの意図。
			 */
			inline const PlayerIntent& GetIntent() const
			{
				return stIntent_;
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
			 * @brief 視線方向(ヨー＋ピッチの3D単位ベクトル)を取得する。
			 *        カメラ追従と射撃の照準で同じ向きを使うための共通ソース。
			 * @return 視線方向。
			 */
			Vector3 GetLookDirection() const;
	
			/**
			 * @brief 武器のリコイル(反動)の強さを取得する。ADS率と武器のリコイルを掛け合わせた値。
			 * @return リコイルの強さ。0に近いほど反動が小さく、1に近いほど反動が大きい。
			 */
			inline float GetLowerRate() const
			{
				return fLowerRate_;
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
			 * @brief 所持しているメディキットの数(UI表示用)。
			 * @return 所持数。
			 */
			inline int GetMedkitCount() const
			{
				return iMedkitCount_;
			}
	
			/**
			 * @brief いまの体の状態から移動速度の倍率を求める。
			 */
			float GetMoveSpeedRate() const;
	
			/**
			 * @brief 所持しているパイプ爆弾の数(UI表示用)。
			 * @return 所持数。
			 */
			inline int GetPipeBombCount() const
			{
				return iPipeBombCount_;
			}
	
			/**
			 * @brief プレイヤーのステータスを取得する。
			 * @return プレイヤーのステータス。
			 */
			inline const nsData::PlayerStatus& GetPlayerStatus() const
			{
				return stPlayerStatus_;
			}
	
			/**
			 * @brief キャラクターの位置を取得する。
			 * @return キャラクターの位置。
			 */
			virtual Vector3& GetPosition() override
			{
				return vPosition_;
			}
	
			/**
			 * @brief 即効アイテム(鎮痛剤/アドレナリン)の所持状態を取得する(UI表示用)。
			 * @return 即効アイテムの種類。持っていなければ EnQuickItem::None。
			 */
			inline EnQuickItem GetQuickItem() const
			{
				return enQuickItem_;
			}
	
			/**
			 * @brief リコイル(反動)のピッチ角度を取得する。
			 * @return ピッチ角度。
			 */
			inline float GetRecoilPitch() const
			{
				return fRecoilPitch_;
			}
	
			/**
			 * @brief リコイル(反動)のヨー角度を取得する。
			 * @return ヨー角度。
			 */
			inline float GetRecoilYaw() const
			{
				return fRecoilYaw_;
			}
	
			/**
			 * @brief 復帰回数を取得する。復帰回数を使い切ると次にダウンしたとき死亡する。
			 * @return 復帰回数。
			 */
			inline int GetReviveCount() const
			{
				return iReviveCount_;
			}
	
			/**
			 * @brief キャラの角度を取得する。
			 * @return キャラの角度。
			 */
			inline const Quaternion& GetRotation() const
			{
				return qRotation_;
			}
	
			/**
			 * @brief 突き飛ばしのクールダウン時間を取得する。
					 連続で突き飛ばすと疲労が溜まり、移動速度が落ちる。
			 * @return クールダウン時間(秒)。
			 */
			inline float GetShoveCooldown() const
			{
				return fShoveCooldown_;
			}
	
			/**
			 * @brief 突き飛ばしの疲労度を取得する。突き飛ばしを連続で行うと疲労が溜まり、移動速度が落ちる。
			 * @return 疲労度。0に近いほど疲労が少なく、1に近いほど疲労が大きい。
			 */
			inline int GetShoveFatigue() const
			{
				return iShoveStack_;
			}
	
			/**
			 * @brief しゃがみ中の移動速度の倍率を取得する。武器の移動速度倍率と掛け合わせて使う。
			 * @return 倍率。1より小さいほど遅く、1より大きいほど速い。
			 */
			inline float GetShoveMotion() const
			{
				return fShoveMotion_;
			}
	
			/**
			 * @brief 突き飛ばしの休止時間を取得する。突き飛ばし後に少しの間、移動速度が落ちる。
			 * @return 休止時間(秒)。
			 */
			inline float GetShoveRestTimer() const
			{
				return fShoveRestTimer_;
			}
	
			/**
			 * @brief 射撃の精度を取得する。
			 * @return 射撃の精度。0.0fで精密、1.0fで大雑把。
			 */
			inline float GetSpreadShot() const
			{
				return fSpreadShot_;
			}
	
			/**
			 * @brief ADS(照準)の精度を取得する。
			 * @return ADSの精度。0.0fで精密、1.0fで大雑把。
			 */
			inline float GetStanceSpreadRate() const
			{
				return fStanceSpreadRate_;
			}
	
			/**
			 * @brief ステートマシーンを取得する。
			 * @return ステートマシーン（非所有）。
			 */
			inline nsState::StateMachine<Actor>* GetStateMachine()
			{
				return pStateMachine_;
			}
	
			/**
			 * @brief 横移動の入力値を取得する。左が負、右が正。
			 * @return 横移動の入力値。
			 */
			inline float GetStrafeAxis() const
			{
				return fStrafeAxis_;
			}
	
			/**
			 * @brief 歩きの揺れの左右成分を取得する。武器の揺れ倍率と掛け合わせて使う。
			 * @return 左右の揺れ。0に近いほど揺れが小さく、1に近いほど揺れが大きい。
			 */
			inline float GetSwayRight() const
			{
				return fSwayRight_;
			}
	
			/**
			 * @brief 歩きの揺れの上下成分を取得する。武器の揺れ倍率と掛け合わせて使う。
			 * @return 上下の揺れ。0に近いほど揺れが小さく、1に近いほど揺れが大きい。
			 */
			inline float GetSwayUp() const
			{
				return fSwayUp_;
			}
	
			/**
			 * @brief 一時体力を取得する(UI表示用)。時間で減っていくぶん。
			 * @return 一時体力(切り捨て)。
			 */
			inline int GetTempHP() const
			{
				return static_cast<int>(fTempHP_);
			}
	
			/**
			 * @brief 恒久HPと一時体力の合計を取得する。負傷歩行やUIの数字はこの合計で見る。
			 * @return 合計HP。
			 */
			inline int GetTotalHP() const
			{
				return GetCurrentHP() + GetTempHP();
			}
	
			/**
			 * @brief 見せ方を取得する。
			 * @return 見せ方。
			 */
			inline EnViewMode GetViewMode() const
			{
				return enViewMode_;
			}
	
			/**
			 * @brief カメラの傾き(ロール)を取得する。
			 *        横移動と歩行に合わせてわずかに傾け、歩いている感じを出す。
			 * @return 傾きの角度(ラジアン)。
			 */
			inline float GetViewRoll() const
			{
				return fViewRoll_;
			}
	
			/**
			 * @brief 武器所持リストを取得する。
			 * @return 武器所持リスト。
			 */
			inline nsWeapon::WeaponInventory& GetWeaponInventory()
			{
				return stWeaponInventory_;
			}
	
			/**
			 * @brief 武器の反動の強さを取得する。ADS率と武器の反動を掛け合わせた値。
			 * @return 反動の強さ。0に近いほど反動が小さく、1に近いほど反動が大きい。
			 */
			inline float GetWeaponKickBack() const
			{
				return fWeaponKickBack_;
			}
	
			/**
			 * @brief 銃のビューモデルの位置を取得する。ADS率や武器のリコイルで上下左右に揺れる。
			 * @return ビューモデルの位置。
			 */
			inline const Vector3& GetWeaponViewPosition() const
			{
				return vWeaponViewPos_;
			}
	
			/**
			 * @brief アドレナリンの効果中か。残り時間が0より大きければ効果中。
			 * @return 効果中なら true。
			 */
			inline bool IsAdrenalineActive() const
			{
				return fAdrenalineTimer_ > 0.0f;
			}
	
			/**
			 * @brief 白黒(復帰回数を使い切り、次にダウンしたら死亡する状態)か。
			 * @return 白黒なら true。
			 */
			inline bool IsBlackAndWhite() const
			{
				return iReviveCount_ >= stPlayerStatus_.stHealthRule_.iMaxReviveCount_;
			}
	
			/**
			 * @brief しゃがみ中か。しゃがむと移動速度が落ちる。
			 * @return しゃがみ中なら true。
			 */
			inline bool IsCrouching() const
			{
				return bIsCrouching_;
			}
	
			/**
			 * @brief 銃を持たせる右手のボーンが見つかっているか。
			 * @return 見つかっていれば true。
			 */
			inline bool IsHandBoneFound() const
			{
				return systems_.GetView().GetRightHandBoneId() >= 0;
			}
	
			/**
			 * @brief メディキットで回復中か。進行度が0より大きければ回復中。
			 * @return 回復中なら true。
			 */
			inline bool IsHealing() const
			{
				return bIsHealing_;
			}
	
			/**
			 * @brief 行動不能(ダウンまたは死亡)かどうか。
			 * @return 生存していなければ true。
			 */
			inline bool IsIncapacitated() const
			{
				return enLifeState_ != EnLifeState::Alive;
			}
	
			/**
			 * @brief ライトが点いているか。点灯中はバッテリーを消費する。
			 * @return
			 */
			inline bool IsLightOn() const
			{
				return bIsLightOn_;
			}
	
			/**
			 * @brief 足を引きずっている(合計HPが少なくて遅い)か。アドレナリン中は引きずらない。
			 * @return 引きずっていれば true。
			 */
			bool IsLimping() const
			{
				return !IsAdrenalineActive() && (GetTotalHP() < stPlayerStatus_.stHealthRule_.iLimpHP_);
			}
	
			/**
			 * @brief 移動中か。
			 * @return 移動中なら true。
			 */
			inline bool IsMoving() const
			{
				return bIsMoving_;
			}
	
			/**
			 * @brief スプリント中か。
			 * @return スプリント中なら true。
			 */
			inline bool IsSprinting() const
			{
				return bIsSprinting_;
			}
	
			/* Add系。*/
		public:
			/**
			 * @brief ADS(照準)の精度を加算する。正の値で精度が下がる。
			 * @param fDelta 加算する精度の変化量。正の値で精度が下がる。
			 */
			inline void AddAdsRate(float fDelta)
			{
				fAdsRate_ += fDelta;
			}
	
			/**
			 * @brief ダウン中の出血残り時間を加算する。正の値で残り時間が増える。
			 * @param fDelta 加算する時間(秒)。
			 */
			inline void AddBleedOutTimer(float fDelta)
			{
				fBleedOutTimer_ += fDelta;
			}
	
			/**
			 * @brief 歩きの揺れのタイマーを加算する。
			 * @param fDelta 加算する量。
			 */
			inline void AddBobTimer(float fDelta)
			{
				fBobTimer_ += fDelta;
			}
	
			/**
			 * @brief 歩きの揺れの強さを加算する。
			 * @param fDelta 加算する量。
			 */
			inline void AddBobWeight(float fDelta)
			{
				fBobWeight_ += fDelta;
			}
	
			/**
			 * @brief カメラのピッチ角を加算する。マウスの上下移動で呼ばれる。
			 * @param deltaTime 経過時間(秒)。
			 */
			inline void AddCameraPitch(float deltaTime)
			{
				fCameraPitch_ += deltaTime;
			}
	
			/**
			 * @brief カメラのヨー角を加算する。マウスの左右移動で呼ばれる。
			 * @param deltaYaw 加算するヨー角(ラジアン)。
			 */
			inline void AddCameraYaw(float deltaYaw)
			{
				fCameraYaw_ += deltaYaw;
			}
	
			/**
			 * @brief	 現在のHPを加算する。正の値で回復、負の値でダメージ。
			 * @param iDelta 加算するHPの変化量。
			 */
			inline void AddCurrentHP(int iDelta)
			{
				stCharacterStatus_.stHp_.iCurrentHP_ += iDelta;
			}
	
			/**
			 * @brief 目の高さを加算する。しゃがむと低くなる。
			 * @param fDelta 加算する量。
			 */
			inline void AddEyeHeight(float fDelta)
			{
				fEyeHeight_ += fDelta;
			}
	
			/**
			 * @brief 射撃の精度を取得する。
			 * @param fDelta 加算する精度の変化量。正の値で精度が下がる。
			 */
			inline void AddLowerRate(float fDelta)
			{
				fLowerRate_ += fDelta;
			}
	
			/**
			 * @brief 所持しているメディキットの数を加算する。
			 * @param iDelta 加算する数。正の値で増え、負の値で減る。
			 */
			inline void AddMedkitCount(int iDelta)
			{
				iMedkitCount_ += iDelta;
			}
	
			/**
			 * @brief メディキットの使用進行度(0〜1)を加算する。
			 * @param fDelta 加算する進行度の変化量。
			 */
			inline void AddMedkitProgress(float fDelta)
			{
				fMedkitProgress_ += fDelta;
			}
	
			/**
			 * @brief パイプ爆弾の所持数を加算する。
			 * @param iDelta
			 */
			inline void AddPipeBombCount(int iDelta)
			{
				iPipeBombCount_ += iDelta;
			}
	
			/**
			 * @brief リコイル(反動)のピッチ角度を加算する。
			 * @param fDelta 加算するピッチ角度。
			 */
			inline void AddRecoilPitch(float fDelta)
			{
				fRecoilPitch_ += fDelta;
			}
	
			/**
			 * @brief リコイル(反動)のヨー角度を加算する。
			 * @param fDelta 加算するヨー角度。
			 */
			inline void AddRecoilYaw(float fDelta)
			{
				fRecoilYaw_ += fDelta;
			}
	
			/**
			 * @brief 突き飛ばしのクールダウン時間を加算する。
			 * @param fDelta 加算する時間(秒)。
			 */
			inline void AddShoveCooldown(float fDelta)
			{
				fShoveCooldown_ += fDelta;
			}
	
			/**
			 * @brief 突き飛ばしの疲労度を加算する。連続で突き飛ばすと疲労が溜まり、移動速度が落ちる。
			 * @param fDelta 加算する量。
			 */
			inline void AddShoveMotion(float fDelta)
			{
				fShoveMotion_ += fDelta;
			}
	
			/**
			 * @brief 突き飛ばしの休止時間を加算する。
			 * @param fDelta 加算する時間(秒)。
			 */
			inline void AddShoveRestTimer(float fDelta)
			{
				fShoveRestTimer_ += fDelta;
			}
	
			/**
			 * @brief 突き飛ばしの疲労度を取得する。連続で突き飛ばすと疲労が溜まり、移動速度が落ちる。
			 * @param iDelta 加算する量。
			 */
			inline void AddShoveStack(int iDelta)
			{
				iShoveStack_ += iDelta;
			}
	
			/**
			 * @brief 射撃の精度を加算する。正の値で精度が下がる。
			 * @param fDelta 加算する精度の変化量。
			 */
			inline void AddSpreadShot(float fDelta)
			{
				fSpreadShot_ += fDelta;
			}
	
			/**
			 * @brief ADS(照準)の精度を加算する。正の値で精度が下がる。
			 * @param fDelta 加算する精度の変化量。正の値で精度が下がる。
			 */
			inline void AddStanceSpreadRate(float fDelta)
			{
				fStanceSpreadRate_ += fDelta;
			}
	
			/**
			 * @brief 横移動の入力値を加算する。左が負、右が正。
			 * @param fDelta 加算する値。
			 */
			inline void AddStrafeAxis(float fDelta)
			{
				fStrafeAxis_ += fDelta;
			}
	
			/**
			 * @brief 歩きの揺れの左右成分を加算する。
			 * @param fDelta 加算する量。
			 */
			inline void AddSwayRight(float fDelta)
			{
				fSwayRight_ += fDelta;
			}
	
			/**
			 * @brief 歩きの揺れの上下成分を加算する。
			 * @param fDelta 加算する量。
			 */
			inline void AddSwayUp(float fDelta)
			{
				fSwayUp_ += fDelta;
			}
	
			/**
			 * @brief 一時体力を足す。合計が最大HPを超えないよう切り詰める。
			 * @param iAmount 足す量。
			 */
			void AddTempHP(int iAmount);
	
			/**
			 * @brief 銃のキックバック量を加算する。
			 * @param fDelta 加算する量。
			 */
			inline void AddWeaponKickBack(float fDelta)
			{
				fWeaponKickBack_ += fDelta;
			}
	
			/* その他 public。*/
		public:
	
			/**
			 * @brief 1発ぶんの反動を加える(発射した瞬間に呼ぶ)。
			 * @param pWeapon 発射した武器。
			 */
			inline void ApplyFireRecoil(nsWeapon::Weapon* pWeapon)
			{
				systems_.GetCalc().ApplyFireRecoil(pWeapon);
			}
	
			/**
			 * @brief リロードの進み具合から、銃をどれだけ動かすかを求める。
			 *        「マガジンを抜く→挿す→構え直す」の3段階に分けて、
			 *        いま何をしているのかが見て分かるようにする。
			 * @param fReloadRate リロードの進み具合(0=始まったところ, 1=完了間際)。
			 * @param fOutLower   銃を下げている度合い(0〜1)。
			 * @param fOutInsert  マガジンを挿し込む動きの度合い(0〜1)。
			 * @param fOutSettle  構え直したときの行き過ぎの度合い(0〜1)。
			 */
			inline void CalcReloadMotion(float fReloadRate, float& fOutLower, float& fOutInsert, float& fOutSettle) const
			{
				systems_.GetCalc().CalcReloadMotion(fReloadRate, fOutLower, fOutInsert, fOutSettle);
			}
	
			/**
			 * @brief 敵にダメージを与え、命中と撃破の通知を出す。倒したら退場させる。
			 * @param pEnemy    当たった敵。
			 * @param iDamage   与えるダメージ。
			 * @param vHitPoint 当たった位置。
			 * @param vShotDir  弾の向き。
			 * @param bHeadShot 頭に当たったか。
			 */
			void DealBulletDamage(CommonEnemy* pEnemy, int iDamage, const Vector3& vHitPoint, const Vector3& vShotDir, bool bHeadShot);
	
			/**
			 * @brief デバッグ用。メディキット・パイプ爆弾・鎮痛剤を1つずつ持たせる。
			 *        射撃場で体力ルールを試し直すためのもので、本編からは呼ばない。
			 */
			void DebugRestockItems();
	
			/**
			 * @brief 死亡状態へ移る。
			 */
			void EnterDead();
	
			/**
			 * @brief ダウン状態へ移る。恒久HPを1に留め、出血を始め、サブ武器へ持ち替える。
			 */
			void EnterDown();
	
			/**
			 * @brief 進行方向を向く。移動方向が0ベクトルなら何もしない。
			 * @param vMoveDir 移動方向。
			 */
			void FaceMoveDirection(const Vector3& vMoveDir)
			{
				vForward_ = vMoveDir;
				qRotation_.SetRotationY(atan2f(vMoveDir.x, vMoveDir.z));
			}
	
			/**
			 * @brief 弾道に当たった敵と部位を、手前から順に調べる(ヒットスキャン)。
			 *        貫通しない武器は1体、貫通する武器はその数だけ奥まで拾う。
			 * @param vRayStart     判定の起点(目の位置)。
			 * @param vRayDirection 弾の進む向き(正規化済み)。
			 * @param iMaxCount     拾う最大数(1＝手前の1体だけ)。
			 * @param vecOutHits    手前から順に並べた命中の一覧を受け取る。
			 */
			inline void FindHitEnemies(const Vector3& vRayStart, const Vector3& vRayDirection, int iMaxCount, std::vector<HitEnemy>& vecOutHits)
			{
				systems_.GetCalc().FindHitEnemies(vRayStart, vRayDirection, iMaxCount, vecOutHits);
			}
	
			/**
			 * @brief 弾1粒ぶんの弾道を飛ばし、当たった敵にダメージを与えてトレーサーを出す。
			 *        散弾はこれを粒の数だけ呼ぶ。
			 * @param pWeapon    撃った武器。
			 * @param vEyePos    判定の起点(目の位置)。
			 * @param vMuzzlePos トレーサーの始点(画面上の銃口)。
			 * @param vAimDir    照準の向き(ばらつかせる前)。
			 */
			inline void FireHitScan(nsWeapon::Weapon* pWeapon, const Vector3& vEyePos, const Vector3& vMuzzlePos, const Vector3& vAimDir)
			{
				systems_.GetCalc().FireHitScan(pWeapon, vEyePos, vMuzzlePos, vAimDir);
			}
	
			/**
			 * @brief 拡散のぶんだけ照準をばらつかせた発射方向を作る。
			 * @param vAimDir もとの照準方向。
			 * @return ばらつかせた発射方向。
			 */
			inline Vector3 MakeSpreadDirection(const Vector3& vAimDir) const
			{
				return systems_.GetCalc().MakeSpreadDirection(vAimDir);
			}
	
			/**
			 * @brief 指定方向へ移動する。壁に押し戻される計算も含む。
			 * @param vDir 移動方向。
			 * @param fSpeed 移動速度。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void MoveOnDirection(const Vector3& vDir, float fSpeed, float fDeltaTime)
			{
				vPosition_ = stMovement_.MoveOnDirection(vDir, fSpeed, fDeltaTime);
			}
	
			/**
			 * @brief 現在のステート種別を遷移樹へ通知する。
			 * @param enState 入ったステート種別。
			 */
			inline void NotifyPlayerState(EnPlayerState enState)
			{
				stTransition_.SetCurrentState(enState);
			}
	
			/**
			 * @brief アイテムを拾う。武器やメディキットなど、拾えるアイテムはすべてこの関数で拾う。
			 */
			inline void PickUpItem()
			{
				systems_.GetItem().PickUpItem();
			}
	
			/**
			 * @brief アニメーションを再生する。再生中のアニメーションは上書きされる。
			 * @param animState 対応するアニメーションの種類。
			 */
			inline void PlayAnimation(ANIM_LIST animState)
			{
				systems_.GetView().PlayAnimation(animState);
			}
	
			/**
			 * @brief イベントバスへゲームイベントを発行する(バスが無ければ何もしない)。
			 * @param enType    発行する通知の種別。
			 * @param vPosition 出来事が起きた位置。
			 * @param vDirection 出来事の向き。
			 * @param iParam 付随する数値(命中ならダメージ量)。
			 * @param bIsCritical 弱点(頭)への命中か。
			 */
			void PublishGameEvent(nsEvent::EnGameEvent enType, const Vector3& vPosition = Vector3::Zero, const Vector3& vDirection = Vector3::Zero, int iParam = 0, bool bIsCritical = false);
	
			/**
			 * @brief ダウン中のプレイヤーを救助して復帰させる(将来の味方/BOT用)。
			 *        ダウン中以外は何もしない。
			 */
			void Revive();
	
			/**
			 * @brief 壁に押し戻される計算だけを進める。移動入力が無くても呼ぶ。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void StandStillMove(float fDeltaTime)
			{
				vPosition_ = stMovement_.Execute(Vector3::Zero, fDeltaTime);
			}
	
			/**
			 * @brief 武器を拾ったときに、現在装備中の武器と入れ替える。
			 * @param pPickup 入れ替える武器のピックアップ。
			 * @return 入れ替えに成功したら true。入れ替えに失敗した場合は false。
			 */
			inline bool SwapWeaponWithPickup(nsItem::Pickup* pPickup)
			{
				return systems_.GetItem().SwapWeaponWithPickup(pPickup);
			}
	
			/**
			 * @brief ライトの点灯状態を切り替える。点灯中はバッテリーを消費する。
			 */
			inline void ToggleLightOn()
			{
				bIsLightOn_ = !bIsLightOn_;
			}
	
			/**
			 * @brief 遷移樹に判定を依頼する。
			 * @return 切り替えたら true。
			 */
			inline bool TryChangeState()
			{
				return stTransition_.TryChangeState();
			}
	
			/**
			 * @brief 入力結果を見てインタラクト・ライトなどその他の操作を処理する。
			 */
			inline void UpdateAction()
			{
				systems_.GetCombat().UpdateAction();
			}
	
			/**
			 * @brief アイテムの更新処理を行う。メディキットの使用進行や即効アイテムの使用など。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			inline void UpdateItems(float fDeltaTime)
			{
				systems_.GetItem().UpdateItems(fDeltaTime);
			}
	
			/**
			 * @brief メディキットの使用進行度を更新する。押し続けると進み、離す・動く・撃つと最初からになる。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			inline void UpdateMedkit(float fDeltaTime)
			{
				systems_.GetItem().UpdateMedkit(fDeltaTime);
			}
	
			/**
			 * @brief 突き飛ばし(近接)入力を処理する。前方近距離の敵を押し返す。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			inline void UpdateShove(float fDeltaTime)
			{
				systems_.GetCombat().UpdateShove(fDeltaTime);
			}
	
			/**
			 * @brief 入力結果を見て武器の更新・発射・切り替えを行う。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			inline void UpdateWeapon(float fDeltaTime)
			{
				systems_.GetCombat().UpdateWeapon(fDeltaTime);
			}
	
			/**
			 * @brief 即効アイテム(鎮痛剤/アドレナリン)を使用する。使用中はメディキットのように進行度があるわけではなく、即座に効果が出る。
			 */
			inline void UseQuickItem()
			{
				systems_.GetItem().UseQuickItem();
			}
	
	
		/* メンバ変数。*/
		private:
			CharacterMovement stMovement_;			//! 移動計算と壁との押し戻し。
			IPlayerController* pController_ = nullptr;	//! 操作意図の供給源(ローカル/ネットで差し替え可能)。
			PlayerTransition stTransition_;	//! 操作キャラのステート遷移樹。
			PlayerIntent stIntent_;				//! このフレームの操作意図。
			PlayerSystems systems_;	//! 見た目/計算/更新/戦闘/アイテムの統括。
			nsWeapon::WeaponInventory stWeaponInventory_;	//! 武器の所持・切り替え担当。
			nsWeapon::EnWeaponType enEquippedType_ = nsWeapon::EnWeaponType::Handgun;			//! 現在装備中の武器の種類(描画対象)。
			EnViewMode enViewMode_ = EnViewMode::FirstPerson;	//! 見せ方(自分の視点か、他人から見た姿か)。
			Vector3 vPosition_ = Vector3::Zero;	//! 座標。
			Vector3 vForward_ = { 0.0f, 0.0f, 1.0f };	//! 向いている方向(=移動方向)。
			Quaternion qRotation_ = Quaternion::Identity;	//! モデルの回転。
			float fCameraYaw_ = 0.0f;					//! カメラの旋回角(ラジアン、マウスで操作)。
			float fCameraPitch_ = 0.0f;				//! カメラのピッチ角(上下、ラジアン。マウス操作、上下限あり)。
			float fRecoilPitch_ = 0.0f;				//! 射撃で跳ね上がった視点の角度(ラジアン)。時間で0へ戻る。
			float fRecoilYaw_ = 0.0f;				//! 射撃で左右へブレた視点の角度(ラジアン)。時間で0へ戻る。
			float fWeaponKickBack_ = 0.0f;			//! 射撃で銃が手前へ下がっている距離。時間で0へ戻る。
			float fAdsRate_ = 0.0f;					//! 覗き込みの度合い(0=腰だめ, 1=完全に覗き込み)。
			float fSpreadShot_ = 0.0f;				//! 連射で増えた拡散角(ラジアン)。時間で0へ戻る。
			float fStanceSpreadRate_ = 1.0f;		//! 姿勢と移動で決まる拡散の倍率(しゃがみで締まり、動くと広がる)。
			bool bIsCrouching_ = false;				//! しゃがんでいるか。
			float fLowerRate_ = 0.0f;				//! 銃を下げている度合い(0=構え, 1=完全に下げる)。走る・回復すると1へ近づく。
			float fEyeHeight_ = 0.0f;				//! いまの目の高さ(立ち/ダウンの目標へ滑らかに寄せる)。
			nsData::PlayerStatus stPlayerStatus_;	//! 調整用のステータス(player.jsonから読み込む)。
			bool bIsMoving_ = false;					//! 移動中か。
			bool bIsSprinting_ = false;				//! スプリント中か。
			float fBobTimer_ = 0.0f;					//! 歩行ボブの位相。
			float fBobWeight_ = 0.0f;					//! 歩行ボブの重み(移動→1/停止→0へ補間)。
			float fViewBobHeight_ = 0.0f;				//! 歩きでカメラが上下している量。目の位置に足す。
			float fViewRoll_ = 0.0f;					//! カメラの傾き(ラジアン)。歩行ボブと横移動の合計。
			float fStrafeAxis_ = 0.0f;				//! 横移動の入力を鈍らせたもの(-1〜1)。カメラの傾きに使う。
			float fSwayRight_ = 0.0f;				//! 視点を振ったときに銃が遅れて右へずれる量。
			float fSwayUp_ = 0.0f;					//! 同・上へずれる量。
			bool bIsLightOn_ = false;				//! ライトが点いているか。
			Vector3 vWeaponViewPos_ = Vector3::Zero;	//! 画面に出している銃の位置(確認用に控えておく)。
			EnLifeState enLifeState_ = EnLifeState::Alive;	//! 生命状態(生存/ダウン/死亡)。
			float fBleedOutTimer_ = 0.0f;				//! ダウン中の出血残り時間(秒)。0で死亡。
			float fTempHP_ = 0.0f;					//! 一時体力。時間で減るので小数で持つ(表示は切り捨て)。
			int iReviveCount_ = 0;					//! メディキットを使わずに復帰した回数。上限に達すると白黒。
			float fMedkitProgress_ = 0.0f;			//! メディキットの使用の進み具合(0〜1)。
			bool bIsHealing_ = false;				//! メディキットを使っている途中か。
			EnQuickItem enQuickItem_ = EnQuickItem::None;	//! 持っている即効アイテム。
			float fAdrenalineTimer_ = 0.0f;			//! アドレナリンの効果の残り時間(秒)。
			float fShoveCooldown_ = 0.0f;				//! 突き飛ばしのクールダウン残り(秒)。
			int iShoveStack_ = 0;						//! 続けて押した回数(疲労)。押さずにいると抜けていく。
			float fShoveRestTimer_ = 0.0f;			//! 最後に押してからの経過時間(秒)。疲労の回復に使う。
			float fShoveMotion_ = 0.0f;				//! 押したときに銃を突き出している度合い(1→0へ戻る)。
			int iMedkitCount_ = 0;					//! 所持回復アイテム数(開始時の数はステータス表から入れる)。
			int iPipeBombCount_ = 0;				//! 所持しているパイプ爆弾の数(開始時の数はステータス表から入れる)。
			nsEvent::EventBus* pEventBus_ = nullptr; //! イベント発行先(生成時にFindGOで取得。無ければ発行しない)。
		};
	}
}
