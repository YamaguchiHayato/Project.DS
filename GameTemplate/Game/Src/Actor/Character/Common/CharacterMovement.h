#pragma once

namespace nsApp
{
	/**
	 * @file   CharacterMovement.h
	 * @brief  キャラクターの移動計算と壁との押し戻しを管理するクラス。
	 * @author Yamaguchi Hayato
	 * @date   2026/08/26
	 */
	class CharacterMovement
	{
	public:
		/* コンストラクタとデストラクタ。*/
		CharacterMovement() = default;
		virtual ~CharacterMovement() = default;


	public:
		/**
		 * @brief キャラクターコントローラーを初期化する。
		 * @param fRadius カプセルの半径。
		 * @param fHeight カプセルの高さ。
		 * @param vPosition 初期位置。
		 */
		void Init(float fRadius, float fHeight, const Vector3& vPosition);

		/**
		 * @brief 速度に応じて1フレーム分移動する。
		 * @param vSpeed 移動速度（秒速）。
		 * @param fDeltaTime 経過時間（秒）。
		 * @return 移動後の位置。
		 */
		Vector3 Execute(const Vector3& vSpeed, float fDeltaTime);

		/**
		 * @brief 希望方向へ最大速度で1フレーム分移動する。
		 * @param vDirection 希望方向（ゼロなら停止）。
		 * @param fMaxSpeed 最大速度（秒速）。
		 * @param fDeltaTime 経過時間（秒）。
		 * @return 移動後の位置。
		 */
		Vector3 MoveOnDirection(const Vector3& vDirection, float fMaxSpeed, float fDeltaTime);

		/**
		 * @brief 目標位置へ最大速度で1フレーム分移動する。
		 * @param vTarget 目標位置。
		 * @param fMaxSpeed 最大速度（秒速）。
		 * @param fDeltaTime 経過時間（秒）。
		 * @return 移動後の位置。
		 */
		Vector3 MoveToward(const Vector3& vTarget, float fMaxSpeed, float fDeltaTime);


		/* セッター。*/
	public:
		/**
		 * @brief 位置を設定する。
		 * @param vPosition 設定する位置。
		 */
		void SetPosition(const Vector3& vPosition);

		/**
		 * @brief 重力の有効化/無効化を設定する。
		 * @param bEnabled 有効化する場合はtrue。
		 */
		void SetGravityEnabled(bool bEnabled)
		{
			bUseGravity_ = bEnabled;
		}


		/* ゲッター。*/
	public:
		/**
		 * @brief 現在位置を取得する。
		 * @return 現在位置。
		 */
		inline const Vector3& GetPosition() const
		{
			return vPosition_;
		}


	private:
		/**
		 * @brief 水平方向の速度を組み立てる。
		 * @param vDirection 希望方向。
		 * @param fMaxSpeed 最大速度（秒速）。
		 * @return 水平速度。方向が短い場合はゼロ。
		 */
		Vector3 MakeHorizontalSpeed(const Vector3& vDirection, float fMaxSpeed) const;


	private:
		CharacterController stCharaCon_; //! 壁との押し戻し用キャラクターコントローラー。
		Vector3 vPosition_ = Vector3::Zero; //! 現在位置。
		bool bIsInited_ = false; //! 初期化済みか。
		bool bUseGravity_ = false; //! true のときだけ重力を適用する。
		float fFallSpeed_ = 0.0f;		//! 落下速度（秒速）。
	};
}