#pragma once

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
		class InGameHud : public IGameObject
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


		private:
			FontRender	stCrosshair_;	//! 画面中央のクロスヘア。
			FontRender	stHpText_;		//! HP表示。
			FontRender	stAmmoText_;	//! 弾数表示。
			FontRender	stObjective_;	//! 目標表示。
			FontRender	stItemText_;	//! アイテム所持数(回復/投擲)。
			FontRender	stStatusText_;	//! ダウン等の状態表示(中央・大)。
			FontRender	stPauseText_;	//! ポーズ中の中央表示。

			wchar_t		wcHp_[32] = L"";		//! HP文字列バッファ(描画まで保持)。
			wchar_t		wcAmmo_[48] = L"";		//! 弾数文字列バッファ。
			wchar_t		wcStatus_[64] = L"";	//! 状態文字列バッファ。
			wchar_t		wcItem_[48] = L"";		//! アイテム所持数バッファ。
		};
	}
}
