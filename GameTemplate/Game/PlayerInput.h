#pragma once
#include <Windows.h>
// TODO: 実際のインクルードパスに置き換えてください(k2EngineLowのMouseクラス)。
// #include "Src/.../Mouse.h"

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   PlayerInput.h
		 * @brief  キーボード＆マウスの生入力を、「移動したい方向」「撃ちたい」
		 *         「リロードしたい」といった論理的なコマンドに変換するクラス。
		 *         Player本体はこのクラスの結果だけを見て動くので、
		 *         キーコンフィグを変えたくなった時に直すのはここだけでよい。
		 *
		 *         対応キー: W/A/S/D=移動, 左クリック=射撃, ホイール=武器切替,
		 *                   R=リロード, E=インタラクト, F=ライト, Esc=メニュー・ポーズ画面。
		 *         マウスボタン・ホイールは nsK2EngineLow::Mouse(g_mouse) から取得する。
		 * @author Yamaguchi Hayato
		 * @date   2026/08/19
		 */
		class PlayerInput
		{
		public:
			PlayerInput() = default;
			virtual ~PlayerInput();


		public:
			/**
			 * @brief 毎フレーム呼ぶ更新処理。
			 */
			void Update();


			/* ゲッター。*/
		public:
			//! 移動方向(x:左右、z:前後。長さが入力の強さになる)。
			inline const Vector3& GetMoveAxis() const { return vMoveAxis_; }

			//! マウスカーソル位置(スプライト空間、中心原点)。照準方向の計算に使う。
			inline const Vector2& GetAimScreenPos() const { return vAimScreenPos_; }

			//! このフレームのマウス横移動量(カーソルロック済み。カメラ旋回に使う)。
			inline float GetMouseDeltaX() const { return fMouseDeltaX_; }

			//! このフレームのマウス縦移動量(カーソルロック済み。カメラの上下に使う)。
			inline float GetMouseDeltaY() const { return fMouseDeltaY_; }

			//! 左クリックが押されているか(フルオート武器の連射判定、押しっぱなし)。
			inline bool IsFirePress() const { return bFirePress_; }

			//! 左クリックを押した瞬間か(単発武器の発射判定)。
			inline bool IsFireTrigger() const { return bFireTrigger_; }

			//! 右クリックを押した瞬間か(近接=突き飛ばし)。
			inline bool IsShoveTrigger() const { return bShoveTrigger_; }

			//! ホイールを奥に回した瞬間か(次の武器へ切り替え)。
			inline bool IsWeaponSwitchNextTrigger() const { return bWeaponSwitchNextTrigger_; }

			//! ホイールを手前に回した瞬間か(前の武器へ切り替え)。
			inline bool IsWeaponSwitchPrevTrigger() const { return bWeaponSwitchPrevTrigger_; }

			//! Rキーが押された瞬間か(リロード)。
			inline bool IsReloadTrigger() const { return bReloadTrigger_; }

			//! Eキーが押された瞬間か(インタラクト)。
			inline bool IsInteractTrigger() const { return bInteractTrigger_; }

			//! Fキーが押された瞬間か(ライトのON/OFF)。
			inline bool IsLightTrigger() const { return bLightTrigger_; }

			//! Escキーが押された瞬間か(メニュー・ポーズ画面)。
			inline bool IsPauseTrigger() const { return bPauseTrigger_; }

			//! Hキーが押された瞬間か(回復アイテム使用)。
			inline bool IsHealTrigger() const { return bHealTrigger_; }

			//! Gキーが押された瞬間か(投擲アイテム)。
			inline bool IsThrowTrigger() const { return bThrowTrigger_; }

			//! Shiftキーを押しているか(スプリント。押しっぱなし判定)。
			inline bool IsSprintPress() const { return bSprintPress_; }


		private:
			/**
			 * @brief トリガー入力(押した瞬間のみtrue)を判定する。
			 * @param iVKey      判定する仮想キーコード(VK_XXXやアルファベット等)。
			 * @param bPrevPress 前回のフレームで押されていたかどうか(呼び出し側の変数を書き換える)。
			 * @return 今回のフレームで押された瞬間ならtrue。
			 */
			bool CheckTrigger(int iVKey, bool& bPrevPress);

			/**
			 * @brief カーソルをウィンドウ中央に固定し、その差分から横移動量を得る。
			 *        カーソルが画面端で止まらないので、無限にカメラを旋回できる。
			 */
			void UpdateMouseLook();


		private:
			Vector3 vMoveAxis_ = Vector3::Zero;			//! 移動方向。
			Vector2 vAimScreenPos_ = { 0.0f, 0.0f };			//! マウスカーソル位置(スプライト空間)。
			float fMouseDeltaX_ = 0.0f;						//! このフレームのマウス横移動量。
			float fMouseDeltaY_ = 0.0f;						//! このフレームのマウス縦移動量。
			bool bCursorLocked_ = false;						//! カーソルを非表示・固定中か。

			bool bFirePress_ = false;					//! 左クリック押下中か。
			bool bFireTrigger_ = false;					//! 左クリックを押した瞬間か。
			bool bShoveTrigger_ = false;					//! 右クリックを押した瞬間か(突き飛ばし)。
			bool bWeaponSwitchNextTrigger_ = false;		//! ホイール奥回転トリガー。
			bool bWeaponSwitchPrevTrigger_ = false;		//! ホイール手前回転トリガー。
			bool bReloadTrigger_ = false;				//! Rキートリガー。
			bool bInteractTrigger_ = false;				//! Eキートリガー。
			bool bLightTrigger_ = false;					//! Fキートリガー。
			bool bPauseTrigger_ = false;					//! Escキートリガー。
			bool bHealTrigger_ = false;					//! Hキートリガー(回復)。
			bool bThrowTrigger_ = false;					//! Gキートリガー(投擲)。
			bool bSprintPress_ = false;					//! Shift押下中か(スプリント)。

			bool bPrevReloadPress_ = false;		//! 前回フレームのRキー押下状態。
			bool bPrevInteractPress_ = false;	//! 前回フレームのEキー押下状態。
			bool bPrevLightPress_ = false;		//! 前回フレームのFキー押下状態。
			bool bPrevPausePress_ = false;		//! 前回フレームのEscキー押下状態。
			bool bPrevHealPress_ = false;		//! 前回フレームのHキー押下状態。
			bool bPrevThrowPress_ = false;		//! 前回フレームのGキー押下状態。
		};
	}
}
