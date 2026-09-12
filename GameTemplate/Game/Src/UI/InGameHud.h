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
		 *         体力は本家(L4D2)と同じく、恒久HPと一時体力を1本のバーに分けて描く。
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
			/**
			 * @brief 体力バーを更新する。恒久HPを濃い色、その右に一時体力を薄い色で続けて描く。
			 * @param pPlayer 表示するプレイヤー。
			 */
			void UpdateHealthBar(const nsActor::Player* pPlayer);

			/**
			 * @brief メディキットの進行バーを更新する(使っている間だけ出す)。
			 * @param pPlayer 表示するプレイヤー。
			 */
			void UpdateHealBar(const nsActor::Player* pPlayer);

			/**
			 * @brief 状態表示(ダウン・白黒・アドレナリン)を更新する。
			 * @param pPlayer 表示するプレイヤー。
			 */
			void UpdateStatusText(const nsActor::Player* pPlayer);

			/**
			 * @brief 画面に重ねる幕(被弾の赤・瀕死の脈打ち・白黒の灰色)を更新する。
			 * @param pPlayer 表示するプレイヤー。
			 */
			void UpdateOverlay(const nsActor::Player* pPlayer);

			/**
			 * @brief クロスヘアの開き具合を拡散に合わせて更新する。
			 * @param pPlayer 表示するプレイヤー。
			 */
			void UpdateCrosshair(const nsActor::Player* pPlayer);


		private:
			FontRender aCrosshair_[4];	//! クロスヘアの4本線(上下左右)。拡散に応じて中心から離れる。
			FontRender stHpText_;		//! HP表示。
			FontRender stAmmoText_;	//! 弾数表示。
			FontRender stObjective_;	//! 目標表示。
			FontRender stItemText_;	//! アイテムスロット(3投擲/4メディキット/5即効)の所持状況。
			FontRender stStatusText_;	//! ダウン等の状態表示(中央・大)。
			FontRender stBuffText_;	//! アドレナリン等、効いている効果の表示。
			FontRender stHealText_;	//! メディキットを使っている間の表示。
			FontRender stPauseText_;	//! ポーズ中の中央表示。
			SpriteRender stDamageOverlay_;	//! 被弾したときに画面へ重ねる赤い幕。
			SpriteRender stGrayOverlay_;	//! 白黒(次にダウンしたら死亡)のときに画面へ重ねる灰色の幕。
			SpriteRender stHealthBarBack_;	//! 体力バーの下地。
			SpriteRender stHealthBarMain_;	//! 体力バーの恒久HPぶん。
			SpriteRender stHealthBarTemp_;	//! 体力バーの一時体力ぶん(恒久HPの右に続けて描く)。
			SpriteRender stHealBarBack_;	//! メディキットの進行バーの下地。
			SpriteRender stHealBarFill_;	//! メディキットの進行バーの中身。
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
			wchar_t wcBuff_[48] = L"";		//! 効果表示バッファ。
			wchar_t wcItem_[96] = L"";		//! アイテムスロットバッファ。
			wchar_t wcDamage_[32] = L"";	//! ダメージ数値バッファ。
		};
	}
}
