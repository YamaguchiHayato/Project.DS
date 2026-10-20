#pragma once
#include "IPlayerController.h"
#include "PlayerInput.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   LocalPlayerController.h
		 * @brief  ローカル実機(キーボード＆マウス)の入力から PlayerIntent を組み立てる
		 *         コントローラ。デバイス依存(感度・キー割当)はすべてここに閉じ込め、
		 *         Player本体にはデバイス非依存の Intent だけを渡す。
		 * @author Izumida Kiryu
		 * @date   2026/08/20
		 */
		class LocalPlayerController : public IPlayerController
		{
		public:
			/* コンストラクタとデストラクタ。*/
			LocalPlayerController() = default;
			virtual ~LocalPlayerController() = default;


		public:
			/**
			 * @brief 実入力を1フレーム分ポーリングし、PlayerIntent へ写す。
			 * @param stOut 充填先の PlayerIntent。
			 */
			void PollIntent(PlayerIntent& stOut) override;


		private:
			PlayerInput	stInput_;					//! キーボード＆マウスの生入力→論理コマンド変換。
			float		fLookSensitivity_ = 0.002f;		//! マウス横移動量(px)→ヨー角(ラジアン)の感度。
			float		fLookPitchSensitivity_ = 0.0018f;	//! マウス縦移動量(px)→ピッチ角(ラジアン)の感度(縦は少し控えめ)。
		};
	}
}
