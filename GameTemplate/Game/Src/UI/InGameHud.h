#pragma once
#include "Src/Event/GameEvent.h"

namespace nsApp
{
	namespace nsActor
	{
		/* 前方宣言。*/
		class Player;
	}

	namespace nsUI
	{
		/**
		 * @file   InGameHud.h
		 * @brief  本編のHUD。プレイヤーのHP・弾数・目標・ダウン状態とクロスヘアを表示する。
		 *         連続値(HP/弾)は Player を毎フレーム参照して反映する(ポーリング表示)。
		 * @author Izumida Kiryu
		 * @date   2026/08/21
		 */
		class InGameHud : public IGameObject, public nsEvent::IGameEventListener
		{
		public:
			/* コンストラクタとデストラクタ。*/
			InGameHud() = default;
			virtual ~InGameHud() = default;


		public:
			/* ライフサイクル。*/
			bool Start() override;
			void Update() override;
			void Render(RenderContext& rc) override;

			/**
			 * @brief 通知を受け取り、命中の手応え(ヒットマーカーとダメージ数値)を表示する。
			 * @param stEvent 受け取った通知。
			 */
			void OnGameEvent(const nsEvent::GameEvent& stEvent) override;


		private:
			FontRender aCrosshair_[4];	//! クロスヘアの4本線(上下左右)。拡散に応じて中心から離れる。
			FontRender stHpText_;		//! HP表示。
			FontRender stAmmoText_;	//! 弾数表示。
			FontRender stObjective_;	//! 目標表示。
			FontRender stItemText_;	//! アイテム所持数(回復/投擲)。
			FontRender stStatusText_;	//! ダウン等の状態表示(中央・大)。
			FontRender stPauseText_;	//! ポーズ中の中央表示。
			SpriteRender stDamageOverlay_;	//! 被弾したときに画面へ重ねる赤い幕。
			FontRender stHitMarker_;	//! 命中したときにクロスヘアへ重ねる印。
			FontRender stDamageText_;	//! 命中したダメージ量の表示。

			float fHitMarkerTimer_ = 0.0f;	//! ヒットマーカーの残り表示時間(秒)。
			float fDamageTimer_ = 0.0f;		//! ダメージ数値の残り表示時間(秒)。
			bool bLastHitCritical_ = false;	//! 直前の命中が弱点(頭)だったか。
			float fDamageFlashTimer_ = 0.0f;	//! 被弾したときの赤い幕の残り時間(秒)。
			float fLowHpPulse_ = 0.0f;		//! HPが少ないときに脈打たせるための位相。

			wchar_t wcHp_[32] = L"";		//! HP文字列バッファ(描画まで保持)。
			wchar_t wcAmmo_[48] = L"";		//! 弾数文字列バッファ。
			wchar_t wcStatus_[64] = L"";	//! 状態文字列バッファ。
			wchar_t wcItem_[48] = L"";		//! アイテム所持数バッファ。
			wchar_t wcDamage_[32] = L"";	//! ダメージ数値バッファ。
		};
	}
}
