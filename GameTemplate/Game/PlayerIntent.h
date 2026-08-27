#pragma once

namespace nsApp
{
	namespace nsActor
	{
		/**
		 * @file   PlayerIntent.h
		 * @brief  プレイヤーの「やりたいこと」を表す入力意図(Intent)。
		 *         入力デバイス(キーボード/マウス/ネット受信)に依存しない中間表現で、
		 *         Player本体はこの構造体だけを見て動く。
		 *         ローカルは LocalPlayerController が実入力から充填し、
		 *         将来のネット対戦では RemotePlayerController が受信データから充填する。
		 *         → Player本体を無改造でネットワーク化できるのがこの分離の目的(Commandパターン)。
		 * @author Izumida Kiryu
		 * @date   2026/08/20
		 */
		struct PlayerIntent
		{
			Vector3 vMoveAxis_ = Vector3::Zero;	//! 移動入力(カメラ相対。x:左右, z:前後。長さが入力の強さ)。
			float fLookYawDelta_ = 0.0f;		//! 視点の旋回量(このフレーム、ラジアン)。デバイス感度は充填側で吸収済み。
			float fLookPitchDelta_ = 0.0f;	//! 視点の上下量(このフレーム、ラジアン)。ピッチ実装まで0。
			bool bFirePress_ = false;		//! 射撃(押しっぱなし=フルオート)。
			bool bFireTrigger_ = false;		//! 射撃(押した瞬間=単発)。
			bool bShoveTrigger_ = false;		//! 近接(shove。押した瞬間)。
			bool bAdsPress_ = false;			//! 覗き込み(ADS。押しっぱなし)。
			bool bReloadTrigger_ = false;	//! リロード(押した瞬間)。
			bool bUseTrigger_ = false;		//! 使用・インタラクト(押した瞬間。武器/アイテム/ドア等)。
			bool bUseHold_ = false;			//! 使用・蘇生ホールド(押しっぱなし。ダウン中の味方を起こす)。
			bool bWeaponNextTrigger_ = false;	//! 武器切替(次)。
			bool bWeaponPrevTrigger_ = false;	//! 武器切替(前)。
			bool bMainWeaponTrigger_ = false;	//! メイン武器へ直接持ち替え。
			bool bSubWeaponTrigger_ = false;	//! サブ武器へ直接持ち替え。
			bool bLightTrigger_ = false;		//! ライトON/OFF(押した瞬間)。
			bool bPauseTrigger_ = false;		//! ポーズ・メニュー(押した瞬間)。

			bool bHealTrigger_ = false;		//! 回復アイテム使用(押した瞬間)。
			bool bThrowTrigger_ = false;		//! 投擲アイテム(押した瞬間)。

			bool bSprintPress_ = false;		//! スプリント(Shift押しっぱなし)。
		};
	}
}
