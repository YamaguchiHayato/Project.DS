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
		 * @struct WeaponStatus
		 * @brief  武器ごとの静的パラメータ(データ)。
		 *         挙動そのものは全武器共通で、この数値だけを差し替えて銃の違いを表現する。
		 */
		struct WeaponStatus
		{
			const char* pName_;			//! 武器名(UI表示用)。
			float		fFireInterval_;	//! 発射間隔(秒)。小さいほど連射が速い。
			int			iMaxAmmo_;		//! マガジン最大弾数。
			float		fReloadTime_;	//! リロードにかかる時間(秒)。
			int			iAttackPower_;	//! 1発の威力。
			bool		bIsFullAuto_;	//! 押しっぱなしで連射できるか(falseなら単発)。
			const char* pModelPath_;		//! 手に持つ武器モデルのファイルパス。
			float		fModelScale_;		//! 表示サイズ倍率(自動サイズ合わせに対する倍率。1.0=標準)。
			float		fViewModelForward_;	//! ビューモデルをカメラから前へ離す距離(大きい銃ほど離すとカメラにめり込みにくい)。
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
			~Weapon() = default;


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
			float			fFireTimer_ = 0.0f;					//! 発射クールタイムの残り(秒)。
			float			fReloadTimer_ = 0.0f;				//! リロードの残り時間(秒)。
			bool			bIsReloading_ = false;				//! リロード中かどうか。
		};
	}
}
