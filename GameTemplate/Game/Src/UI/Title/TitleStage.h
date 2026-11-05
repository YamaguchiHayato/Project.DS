#pragma once
/**
 * @file   TitleStage.h
 * @brief  メニュー画面に立たせるモデルと、そのためのカメラを用意する。
 * @author Izumida Kiryu
 * @date   2026/10/29
 * @note   メニューの文字などの2D表示は TitleUI が受け持つ。こちらは3D表示だけを受け持つ。
 *         モデルは PMX。tkm の ModelRender と違い、描画はエンジンの PMX 専用パスが行うので、
 *         このクラスの Render では描かない。出し入れは描画エンジンへの登録と解除で行う。
 *         カメラはシーンをまたいで共用なので、来たときの値を控えて、抜けるときに戻す。
 *         ライトは触らない。PMX は SceneLight を見ず、自前のシェーディングで描くため。
 */

namespace nsApp
{
	namespace nsUI
	{
		class TitleStage : public IGameObject
		{
		public:
			/* コンストラクタとデストラクタ。*/
			TitleStage() = default;
			virtual ~TitleStage();


		public:
			/* ライフサイクル。*/
			/**
			 * @brief 初期化処理。
			 * @return 初期化に成功したらtrue。
			 */
			bool Start() override;

			/**
			 * @brief 更新処理。
			 */
			void Update() override;

			/**
			 * @brief 描画処理。
			 * @param rc レンダリングコンテキスト。
			 * @details PMX はエンジンの専用パスが描くので、ここでは何もしない。
			 */
			void Render(RenderContext& rc) override {}


		public:
			/**
			 * @brief モデルを見せるかどうかを切り替える。
			 * @param bIsVisible 見せるなら true。
			 * @details 読み込みの前に呼ばれてもよい。実際の出し入れは Update で合わせる。
			 */
			inline void SetVisible(bool bIsVisible)
			{
				/* 出すかどうかを控えるだけにする。描画エンジンへの登録は Update で行う。*/
				bIsVisible_ = bIsVisible;
			}

			/**
			 * @brief モデルを読み込めたかどうかを返す。
			 * @return 読み込めていたら true。
			 */
			inline bool IsLoaded() const
			{
				/* 読み込みの結果を返す。*/
				return bIsLoaded_;
			}


		private:
			/**
			 * @brief PMXとモーションを読み込む。
			 * @return 読み込めたら true。
			 */
			bool LoadModel();

			/**
			 * @brief モデルが画面の右側に来るようにカメラを置く。
			 */
			void SetUpCamera();

			/**
			 * @brief 描画エンジンへの登録と解除を、見せるかどうかに合わせる。
			 * @param bIsRegister 登録するなら true。
			 */
			void RegisterModel(bool bIsRegister);

			/**
			 * @brief 待機モーションを実時間で進める。
			 */
			void UpdateMotion();


		private:
			PMXLoader stPmxLoader_;			//! PMXの読み込み。
			PMXModel stPmxModel_;			//! 読み込んだモデル。
			PMXRender stPmxRender_;			//! モデルの描画。
			VMDLoader stVmdLoader_;			//! 待機モーション。

			/*
			 * モーションは固定のdtを積むのではなく、立たせてからの実経過秒で進める。
			 * 積み上げだとフレーム落ちのたびに再生の速さがぶれて、カクついて見えるため。
			 */
			Stopwatch stMotionClock_;		//! モーションを進めるための時計。
			float fMotionEndSecond_ = 0.0f;	//! モーションの終わり(秒)。0より大きければこの周期で繰り返す。
			float fPrevMotionSecond_ = 0.0f;//! 前フレームのモーションの時刻(秒)。繰り返しの折り返しを見つけるのに使う。

			Vector3 vSavedCameraPosition_ = Vector3::Zero;	//! 来たときのカメラ位置。
			Vector3 vSavedCameraTarget_ = Vector3::Zero;	//! 来たときのカメラ注視点。
			float fSavedViewAngle_ = 0.0f;	//! 来たときの画角(ラジアン)。
			float fSavedNear_ = 0.0f;		//! 来たときのカメラの手前の面。
			float fSavedFar_ = 0.0f;		//! 来たときのカメラの奥の面。
			float fModelHeight_ = 0.0f;		//! 頂点から測ったモデルの背丈。カメラの寄り引きの基準にする。
			bool bIsLoaded_ = false;		//! モデルを読み込めたか。
			bool bIsRegistered_ = false;	//! 描画エンジンに登録しているか。
			bool bIsVisible_ = false;		//! モデルを見せるか。
		};
	}
}
