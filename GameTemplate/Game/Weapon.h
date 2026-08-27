#pragma once

namespace nsApp
{
	namespace nsWeapon
	{
		/**
		 * @enum EnWeaponType
		 * @brief 武器の種類。WeaponStatusテーブルの添字にもなる。
		 */
		enum class EnWeaponType : uint8_t
		{
			Handgun,		//! ハンドガン。
			AssaultRifle,	//! アサルトライフル。
			/* ↑ここに武器を追加したら、Weapon.cpp のステータステーブルにも1行追加する。*/
			Num,			//! 武器の数。
		};

		/**
		 * @enum  EnWeaponSlot
		 * @brief 武器の区分。メインは予備弾数に限りがあり、サブは弾切れで詰まないよう無限にする。
		 */
		enum class EnWeaponSlot : uint8_t
		{
			Main,	//! メイン武器(予備弾数に限りがある)。
			Sub,	//! サブ武器(予備弾数は無限。そのぶん火力は低い)。
		};

		/**
		 * @struct RecoilStep
		 * @brief  リコイルパターンの1発ぶん。撃つたびにこの順で視点が跳ねる。
		 *         値は倍率で、実際の角度は武器の反動量(fRecoilPitch_/fRecoilYaw_)を掛けて決まる。
		 */
		struct RecoilStep
		{
			float fPitch_;	//! 上方向へ跳ねる量の倍率。
			float fYaw_;	//! 左右へ跳ねる量の倍率(正で右)。
		};

		/**
		 * @struct WeaponStatus
		 * @brief  武器ごとの静的パラメータ(データ)。
		 *         挙動そのものは全武器共通で、この数値だけを差し替えて銃の違いを表現する。
		 */
		struct WeaponStatus
		{
			const char* pName_;			//! 武器名(UI表示用)。
			float fFireInterval_;		//! 発射間隔(秒)。小さいほど連射が速い。
			EnWeaponSlot enSlot_;		//! 武器の区分(メイン/サブ)。
			int iMaxAmmo_;				//! マガジン最大弾数。
			int iMaxReserveAmmo_;		//! 予備弾数の上限(サブ武器では使わない)。
			float fReloadTime_;			//! リロードにかかる時間(秒)。
			float fDeployTime_;			//! 持ち替えてから撃てるようになるまでの時間(秒)。
			int iAttackPower_;			//! 1発の威力。
			bool bIsFullAuto_;			//! 押しっぱなしで連射できるか(falseなら単発)。
			const char* pModelPath_;	//! 手に持つ武器モデルのファイルパス。
			float fModelScale_;			//! 表示サイズ倍率(自動サイズ合わせに対する倍率。1.0=標準)。
			float fViewModelForward_;	//! ビューモデルをカメラから前へ離す距離(大きい銃ほど離すとカメラにめり込みにくい)。
			float fRecoilPitch_;		//! 1発あたりに視点が跳ね上がる角度(ラジアン)。
			float fRecoilYaw_;			//! 1発あたりに視点が左右へブレる角度の最大値(ラジアン)。
			float fKickBack_;			//! 1発あたりに銃が手前へ下がる距離。
			float fSpreadHip_;			//! 腰だめ撃ちの弾の拡散角(ラジアン)。
			float fSpreadAds_;			//! 覗き込み(ADS)中の弾の拡散角(ラジアン)。
			float fSpreadPerShot_;		//! 1発撃つごとに増える拡散角(ラジアン)。連射でばらつく。
			float fAdsZoomRate_;		//! 覗き込み時の画角の倍率(小さいほど拡大される)。
			float fAdsSpeedRate_;		//! 覗き込み中の移動速度の倍率。
			const RecoilStep* pRecoilPattern_;	//! リコイルパターン(撃つ順に跳ねる方向が決まっている)。
			int iRecoilPatternCount_;			//! リコイルパターンの段数。
		};

		/**
		 * @file   Weapon.h
		 * @brief  データ駆動の武器クラス。
		 *         挙動(クールタイム・弾数・リロード)は全武器共通で、
		 *         EnWeaponTypeから引いたWeaponStatusのパラメータだけで銃の違いを表す。
		 *         銃を増やすときはEnWeaponTypeとステータステーブルに1行足すだけでよい。
		 * @author Izumida Kiryu
		 * @date   2026/08/19
		 */
		class Weapon
		{
		public:
			/* コンストラクタとデストラクタ。*/
			Weapon() = default;
			virtual ~Weapon() = default;


		public:
			/**
			 * @brief 武器の種類を指定して初期化する。
			 * @param enType 武器の種類。
			 */
			void Init(EnWeaponType enType);

			/**
			 * @brief 毎フレームの更新処理(発射クールタイム・リロードの経過)。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void Update(float fDeltaTime);

			/**
			 * @brief 発射を試みる。
			 * @param vPosition  発射位置(銃口)。
			 * @param vDirection 発射方向(正規化済み)。
			 * @return 発射できたらtrue。クールタイム中・リロード中・弾切れならfalse。
			 */
			bool Fire(const Vector3& vPosition, const Vector3& vDirection);

			/**
			 * @brief リロードを開始する。
			 */
			void Reload();

			/**
			 * @brief この武器に持ち替える(構える動作を始める)。
			 *        リロード中だった場合は中断する(Apexと同じく持ち替えでキャンセルされる)。
			 */
			void Deploy();


		/* ゲッター。*/
		public:
			/**
			 * @brief 武器名を取得する。
			 * @return 武器名。
			 */
			inline const char* GetName() const
			{
				return stStatus_.pName_;
			}

			/**
			 * @brief 武器の種類を取得する(武器ごとのモデル配列の添字などに使う)。
			 * @return 武器の種類。
			 */
			inline EnWeaponType GetType() const
			{
				return enType_;
			}

			/**
			 * @brief 手に持つ武器モデルのファイルパスを取得する。
			 * @return 武器モデルのパス。
			 */
			inline const char* GetModelPath() const
			{
				return stStatus_.pModelPath_;
			}

			/**
			 * @brief 武器モデルの表示スケールを取得する。
			 * @return 表示スケール。
			 */
			inline float GetModelScale() const
			{
				return stStatus_.fModelScale_;
			}

			/**
			 * @brief ビューモデルをカメラから前へ離す距離を取得する。
			 * @return 前方距離。
			 */
			inline float GetViewModelForward() const
			{
				return stStatus_.fViewModelForward_;
			}

			/**
			 * @brief 現在の残弾数を取得する。
			 * @return 現在の残弾数。
			 */
			inline int GetCurrentAmmo() const
			{
				return iCurrentAmmo_;
			}

			//! リロードの進み具合(0=始まったところ, 1=完了間際)。演出に使う。
			inline float GetReloadRate() const
			{
				/* リロード中でなければ0。*/
				if (!bIsReloading_ || stStatus_.fReloadTime_ <= 0.0f)
					return 0.0f;

				return 1.0f - (fReloadTimer_ / stStatus_.fReloadTime_);
			}

			//! 構えている途中か(この間は撃てない)。
			inline bool IsDeploying() const { return fDeployTimer_ > 0.0f; }

			//! 武器の区分(メイン/サブ)。
			inline EnWeaponSlot GetSlot() const { return stStatus_.enSlot_; }

			//! 予備弾数が無限か(サブ武器なら true)。
			inline bool IsInfiniteReserve() const { return stStatus_.enSlot_ == EnWeaponSlot::Sub; }

			//! 現在の予備弾数(無限の武器では常に上限を返す)。
			inline int GetReserveAmmo() const { return IsInfiniteReserve() ? stStatus_.iMaxReserveAmmo_ : iReserveAmmo_; }

			//! 予備弾数の上限。
			inline int GetMaxReserveAmmo() const { return stStatus_.iMaxReserveAmmo_; }

			/**
			 * @brief マガジンの最大弾数を取得する。
			 * @return マガジンの最大弾数。
			 */
			inline int GetMaxAmmo() const
			{
				return stStatus_.iMaxAmmo_;
			}

			/**
			 * @brief 1発の威力を取得する。
			 * @return 1発の威力。
			 */
			inline int GetAttackPower() const
			{
				return stStatus_.iAttackPower_;
			}

			/**
			 * @brief フルオート武器か。
			 * @return 押しっぱなしで連射できるならtrue。
			 */
			inline bool IsFullAuto() const
			{
				return stStatus_.bIsFullAuto_;
			}

			//! 1発あたりに視点が跳ね上がる角度(ラジアン)。
			inline float GetRecoilPitch() const { return stStatus_.fRecoilPitch_; }

			//! 1発あたりに視点が左右へブレる角度の最大値(ラジアン)。
			inline float GetRecoilYaw() const { return stStatus_.fRecoilYaw_; }

			//! 1発あたりに銃が手前へ下がる距離。
			inline float GetKickBack() const { return stStatus_.fKickBack_; }

			//! 腰だめ撃ちの弾の拡散角(ラジアン)。
			inline float GetSpreadHip() const { return stStatus_.fSpreadHip_; }

			//! 覗き込み(ADS)中の弾の拡散角(ラジアン)。
			inline float GetSpreadAds() const { return stStatus_.fSpreadAds_; }

			//! 1発撃つごとに増える拡散角(ラジアン)。
			inline float GetSpreadPerShot() const { return stStatus_.fSpreadPerShot_; }

			//! 覗き込み時の画角の倍率(小さいほど拡大される)。
			inline float GetAdsZoomRate() const { return stStatus_.fAdsZoomRate_; }

			//! 覗き込み中の移動速度の倍率。
			inline float GetAdsSpeedRate() const { return stStatus_.fAdsSpeedRate_; }

			//! 直前に撃った1発ぶんのリコイル(パターンから取り出した跳ね方)。
			inline const RecoilStep& GetLastRecoilStep() const { return stLastRecoilStep_; }

			/**
			 * @brief リロード中か。
			 * @return リロード中ならtrue。
			 */
			inline bool IsReloading() const
			{
				return bIsReloading_;
			}


		private:
			WeaponStatus	stStatus_ = {};						//! この武器のパラメータ。
			EnWeaponType	enType_ = EnWeaponType::Handgun;	//! 武器の種類。
			int				iCurrentAmmo_ = 0;					//! 現在の残弾数。
			int				iReserveAmmo_ = 0;					//! 現在の予備弾数(メイン武器のみ減る)。
			float			fFireTimer_ = 0.0f;					//! 発射クールタイムの残り(秒)。
			float			fReloadTimer_ = 0.0f;				//! リロードの残り時間(秒)。
			bool			bIsReloading_ = false;				//! リロード中かどうか。
			int				iRecoilIndex_ = 0;					//! リコイルパターンの現在位置(撃つたびに進む)。
			float			fDeployTimer_ = 0.0f;				//! 構え終わるまでの残り時間(秒)。
			float			fRecoilResetTimer_ = 0.0f;			//! 撃つのをやめてからの経過時間(秒)。一定時間でパターンが最初へ戻る。
			RecoilStep		stLastRecoilStep_ = { 0.0f, 0.0f };	//! 直前に撃った1発ぶんの跳ね方。
		};
	}
}
