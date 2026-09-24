#pragma once
#include "Src/Actor/Character/ICharacter.h"
#include "Src/Actor/Character/Common/CharacterMovement.h"
#include "IPlayerController.h"
#include "WeaponInventory.h"
#include "Src/Event/GameEvent.h"
#include "Src/Data/PlayerStatusTable.h"
#include "Src/Combat/HitBoxSet.h"

namespace nsApp
{
	namespace nsEvent { class EventBus; }	//! 前方宣言。
	namespace nsItem { class Pickup; }		//! 前方宣言。

	namespace nsActor
	{
		class CommonEnemy;	//! 前方宣言。

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
			Player();
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
			 * @brief 見せ方(自分の視点か、他人から見た姿か)を設定する。
			 *        マルチでは自分の機体だけ FirstPerson、他プレイヤーは ThirdPerson になる。
			 * @param enViewMode 設定する見せ方。
			 */
			inline void SetViewMode(EnViewMode enViewMode)
			{
				enViewMode_ = enViewMode;
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
			 * @brief 見せ方を取得する。
			 * @return 見せ方。
			 */
			inline EnViewMode GetViewMode() const
			{
				return enViewMode_;
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
			 * @brief 体モデルの実際の大きさ(一番長い辺)を取得する。
			 *        モデルの頂点から測った値なので、想定の身長ではなく本当の表示サイズが返る。
			 *        三人称カメラの寄り引きなど、モデルの大きさに合わせたい処理で使う。
			 * @return 体モデルの大きさ。測れなかった場合は0。
			 */
			inline float GetBodyModelSize() const
			{
				return fBodyModelSize_;
			}

			/**
			 * @brief 銃を持たせる右手のボーンが見つかっているか。
			 * @return 見つかっていれば true。false のときは銃を腰の高さへ置いている。
			 */
			inline bool IsHandBoneFound() const
			{
				return iRightHandBoneId_ >= 0;
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
			 * @brief 白黒(復帰回数を使い切り、次にダウンしたら死亡する状態)か。
			 * @return 白黒なら true。
			 */
			inline bool IsBlackAndWhite() const
			{
				return iReviveCount_ >= stPlayerStatus_.stHealthRule_.iMaxReviveCount_;
			}

			/**
			 * @brief 足を引きずっている(合計HPが少なくて遅い)か。アドレナリン中は引きずらない。
			 * @return 引きずっていれば true。
			 */
			bool IsLimping() const;

			//! メディキットの使用の進み具合(0=使っていない, 1=使い切る直前)。UIの進行バーに使う。
			inline float GetHealProgress() const { return fMedkitProgress_; }

			//! メディキットを使っている途中か。
			inline bool IsHealing() const { return bIsHealing_; }

			//! アドレナリンの効果の残り時間(秒)。効いていなければ0。
			inline float GetAdrenalineRemain() const { return fAdrenalineTimer_; }

			//! アドレナリンが効いているか。
			inline bool IsAdrenalineActive() const { return fAdrenalineTimer_ > 0.0f; }

			//! 持っている即効アイテム(鎮痛剤/アドレナリン/無し)。
			inline EnQuickItem GetQuickItem() const { return enQuickItem_; }

			//! メディキットを使わずに復帰した回数(UI表示用)。
			inline int GetReviveCount() const { return iReviveCount_; }

			//! しゃがんでいるか。
			inline bool IsCrouching() const { return bIsCrouching_; }

			//! 押し返しの疲労(続けて押した回数)。上限に達すると次までの間が長くなる。
			inline int GetShoveFatigue() const { return iShoveStack_; }

			/**
			 * @brief 現在装備中の武器を取得する(UI表示用。無ければnullptr)。
			 * @return 現在の武器。
			 */
			inline nsWeapon::Weapon* GetEquippedWeapon()
			{
				return stWeaponInventory_.GetCurrentWeapon();
			}

			//! いまの弾の拡散角(ラジアン)。クロスヘアの開き具合に使う。
			float GetCurrentSpread() const;

			//! 覗き込みの度合い(0=腰だめ, 1=完全に覗き込み)。カメラの画角に使う。
			inline float GetAdsRate() const { return fAdsRate_; }

			//! 覗き込み時の画角の倍率(装備中の武器のもの。武器が無ければ1.0)。
			float GetAdsZoomRate();

			//! 所持している回復アイテム数(UI表示用)。
			inline int GetMedkitCount() const { return iMedkitCount_; }

			//! 画面に出している銃の位置(不具合を調べるための確認用)。
			inline const Vector3& GetWeaponViewPosition() const { return vWeaponViewPos_; }

			//! ライトが点いているか(UI表示用)。
			inline bool IsLightOn() const { return bIsLightOn_; }

			//! 所持しているパイプ爆弾の数(UI表示用)。
			inline int GetPipeBombCount() const { return iPipeBombCount_; }

			/**
			 * @brief ダウン中のプレイヤーを救助して復帰させる(将来の味方/BOT用)。
			 *        ダウン中以外は何もしない。
			 */
			void Revive();

			/**
			 * @brief デバッグ用。メディキット・パイプ爆弾・鎮痛剤を1つずつ持たせる。
			 *        射撃場で体力ルールを試し直すためのもので、本編からは呼ばない。
			 */
			void DebugRestockItems();


		private:
			/**
			 * @brief モデルとアニメーションを読み込む。
			 */
			void InitModel();

			/**
			 * @brief マウスの移動量で視点(ヨー・ピッチ)を回す。ダウン中でも視点だけは動かせる。
			 */
			void UpdateLook();

			/**
			 * @brief 入力結果を見て移動する。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateMove(float fDeltaTime);

			/**
			 * @brief いまの体の状態から移動速度の倍率を求める(負傷歩行・瀕死・アドレナリン)。
			 * @return 歩きの速度に掛ける倍率。
			 */
			float GetMoveSpeedRate() const;

			/**
			 * @brief 手の動き(リロード・回復)の速さの倍率を求める(アドレナリン中は速い)。
			 * @return 倍率。通常は1。
			 */
			float GetActionSpeedRate() const;

			/**
			 * @brief 体力まわりの時間経過を進める(一時体力の減少、アドレナリンの残り時間)。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateHealth(float fDeltaTime);

			/**
			 * @brief 目の高さを状態に合わせて動かす(ダウン中は低く、復帰すると戻る)。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateEyeHeight(float fDeltaTime);

			/**
			 * @brief メディキットの使用を進める。押し続けると進み、離す・動く・撃つと最初からになる。
			 *        使い切ると失った恒久HPの一定割合を回復し、一時体力と復帰回数はリセットされる。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateMedkit(float fDeltaTime);

			/**
			 * @brief 持っている即効アイテム(鎮痛剤/アドレナリン)を使う。
			 */
			void UseQuickItem();

			/**
			 * @brief 一時体力を足す。合計が最大HPを超えないよう切り詰める。
			 * @param iAmount 足す量。
			 */
			void AddTempHP(int iAmount);

			/**
			 * @brief ダウン状態へ移る。恒久HPを1に留め、出血を始め、サブ武器へ持ち替える。
			 */
			void EnterDown();

			/**
			 * @brief 死亡状態へ移る。
			 */
			void EnterDead();

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
			 * @brief 手持ちのライトを目線の位置から視線の先へ向ける。倒れていても点いたままなので毎フレーム呼ぶ。
			 */
			void UpdateFlashLight();

			/**
			 * @brief 突き飛ばし(近接)入力を処理する。前方近距離の敵を押し返す。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateShove(float fDeltaTime);

			/**
			 * @brief アイテム(メディキット/即効アイテム/投擲)の入力を処理する。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateItems(float fDeltaTime);

			/**
			 * @brief 足元に落ちている物資を拾う。
			 */
			void PickUpItem();

			/**
			 * @brief 落ちている銃と、同じ区分の手持ちの銃を入れ替える。手放した銃はその場に落とす。
			 * @param pPickup 落ちている銃。
			 * @return 入れ替えたら true。すでに同じ銃を持っていれば false。
			 */
			bool SwapWeaponWithPickup(nsItem::Pickup* pPickup);

			/**
			 * @brief 覗き込み(ADS)の度合いと、弾の拡散を更新する。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateAds(float fDeltaTime);

			/**
			 * @brief 姿勢(しゃがみ)と移動(歩き/走り)で決まる拡散の倍率を、目標へ滑らかに寄せる。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateStanceSpread(float fDeltaTime);

			/**
			 * @brief 歩きと視点移動から生まれる揺れを更新する。
			 *        ・視点を振ったときに銃が遅れてついてくるずれ(sway)。
			 *        ・歩きに合わせたカメラの上下動と傾き。
			 *        ・横移動に合わせたカメラの傾き(ストレイフロール)。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateViewSway(float fDeltaTime);

			/**
			 * @brief リロードの進み具合から、銃をどれだけ動かすかを求める。
			 *        「マガジンを抜く→挿す→構え直す」の3段階に分けて、
			 *        いま何をしているのかが見て分かるようにする。
			 * @param fReloadRate リロードの進み具合(0=始まったところ, 1=完了間際)。
			 * @param fOutLower   銃を下げている度合い(0〜1)。
			 * @param fOutInsert  マガジンを挿し込む動きの度合い(0〜1)。
			 * @param fOutSettle  構え直したときの行き過ぎの度合い(0〜1)。
			 */
			void CalcReloadMotion(float fReloadRate, float& fOutLower, float& fOutInsert, float& fOutSettle) const;

			/**
			 * @brief 射撃の反動を時間で元へ戻す(跳ね上がった視点と下がった銃を戻す)。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void UpdateRecoil(float fDeltaTime);

			/**
			 * @brief 拡散のぶんだけ照準をばらつかせた発射方向を作る。
			 * @param vAimDir もとの照準方向。
			 * @return ばらつかせた発射方向。
			 */
			Vector3 MakeSpreadDirection(const Vector3& vAimDir) const;

			/**
			 * @struct HitEnemy
			 * @brief  1本の弾道に当たった敵1体ぶんの記録。
			 */
			struct HitEnemy
			{
				CommonEnemy* pEnemy_ = nullptr;		//! 当たった敵。
				nsCombat::HitResult stResult_;		//! 当たった部位と位置。
			};

			/**
			 * @brief 弾道に当たった敵と部位を、手前から順に調べる(ヒットスキャン)。
			 *        貫通しない武器は1体、貫通する武器はその数だけ奥まで拾う。
			 * @param vRayStart     判定の起点(目の位置)。
			 * @param vRayDirection 弾の進む向き(正規化済み)。
			 * @param iMaxCount     拾う最大数(1＝手前の1体だけ)。
			 * @param vecOutHits    手前から順に並べた命中の一覧を受け取る。
			 */
			void FindHitEnemies(const Vector3& vRayStart, const Vector3& vRayDirection, int iMaxCount, std::vector<HitEnemy>& vecOutHits);

			/**
			 * @brief 弾1粒ぶんの弾道を飛ばし、当たった敵にダメージを与えてトレーサーを出す。
			 *        散弾はこれを粒の数だけ呼ぶ。
			 * @param pWeapon    撃った武器。
			 * @param vEyePos    判定の起点(目の位置)。
			 * @param vMuzzlePos トレーサーの始点(画面上の銃口)。
			 * @param vAimDir    照準の向き(ばらつかせる前)。
			 */
			void FireHitScan(nsWeapon::Weapon* pWeapon, const Vector3& vEyePos, const Vector3& vMuzzlePos, const Vector3& vAimDir);

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
			 * @brief 1発ぶんの反動を加える(発射した瞬間に呼ぶ)。
			 * @param pWeapon 発射した武器。
			 */
			void ApplyFireRecoil(nsWeapon::Weapon* pWeapon);

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
			 * @param iParam 付随する数値(命中ならダメージ量)。
			 * @param bIsCritical 弱点(頭)への命中か。
			 */
			void PublishGameEvent(nsEvent::EnGameEvent enType, const Vector3& vPosition = Vector3::Zero, const Vector3& vDirection = Vector3::Zero, int iParam = 0, bool bIsCritical = false);

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
			 * @brief 自分の視点用に、銃を画面手前のビューモデルとして配置する。
			 * @param pWeapon 現在装備中の武器。
			 * @param iType   武器種類(モデル配列の添字)。
			 */
			void UpdateViewWeaponModel(nsWeapon::Weapon* pWeapon, int iType);

			/**
			 * @brief 他人から見た姿用に、銃を体の右手のボーンへ持たせる。
			 * @param pWeapon 現在装備中の武器。
			 * @param iType   武器種類(モデル配列の添字)。
			 */
			void UpdateHandWeaponModel(nsWeapon::Weapon* pWeapon, int iType);

			/**
			 * @brief 武器モデルの中心が指定した位置へ来るよう、モデル原点のズレを打ち消して配置する。
			 * @param iType      武器種類(モデル配列の添字)。
			 * @param vCenterPos 銃の中心を置きたい位置。
			 * @param qBase      原点ズレの補正に使う基準の回転(視点の向きだけ。演出の回転は含めない)。
			 * @param qRotation  実際に銃へ与える回転(演出を含む)。
			 * @param fScale     表示スケール。
			 */
			void PlaceWeaponModel(int iType, const Vector3& vCenterPos, const Quaternion& qBase, const Quaternion& qRotation, float fScale);

			/**
			 * @brief 体モデルのローカルAABBから実際の表示サイズを測って保存する。
			 *        読み込み時に一度だけ呼ぶ。
			 */
			void CalcBodyModelSize();

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
			//! 各武器モデルのローカルAABB中心(原点ズレ吸収用)。読み込んだときに CalcWeaponModelFit が入れる。
			Vector3 aWeaponModelCenter_[static_cast<int>(nsWeapon::EnWeaponType::Num)];
			float aWeaponModelAutoScale_[static_cast<int>(nsWeapon::EnWeaponType::Num)] = {};//! 各武器モデルの自動サイズ合わせスケール。
			float aWeaponModelLongestEdge_[static_cast<int>(nsWeapon::EnWeaponType::Num)] = {};//! 各武器モデルのローカルAABBの一番長い辺。実寸を指定して置くときに使う。
			nsWeapon::EnWeaponType enEquippedType_ = nsWeapon::EnWeaponType::Handgun;			//! 現在装備中の武器の種類(描画対象)。
			EnViewMode enViewMode_ = EnViewMode::FirstPerson;	//! 見せ方(自分の視点か、他人から見た姿か)。
			int iRightHandBoneId_ = -1;							//! 銃を持たせる右手のボーン番号。見つからなければ-1。
			float fBodyModelSize_ = 0.0f;						//! 体モデルの実際の大きさ(頂点から測った一番長い辺)。

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
			EnPlayerAnimation enPlayingAnimation_ = EnPlayerAnimation::Num;	//! 再生中のアニメーション。

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
			nsEvent::EventBus* pEventBus_ = nullptr;				//! イベント発行先(生成時にFindGOで取得。無ければ発行しない)。
		};
	}
}
