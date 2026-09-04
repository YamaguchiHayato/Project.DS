#include "stdafx.h"
#include "InGameHud.h"
#include "Player.h"
#include "Weapon.h"
#include "Src/System/GamePause.h"
#include "Src/Event/EventBus.h"
#include <cstdio>

namespace
{
	const Vector3 vCrosshairCenter_ = { -6.0f, 10.0f, 0.0f };	//! クロスヘアの中心(文字の見た目を合わせた微調整込み)。
	const float fCrosshairBaseGap_ = 10.0f;					//! 拡散が無いときの、中心から各線までの距離。
	const float fCrosshairSpreadScale_ = 900.0f;				//! 拡散角(ラジアン)を画面上の開き量へ変換する倍率。
	const float fCrosshairMaxGap_ = 90.0f;					//! クロスヘアが開く上限。
	const Vector3 vHpPos_ = { -900.0f, -470.0f, 0.0f };		//! HP(左下)。
	const Vector3 vAmmoPos_ = { 430.0f, -470.0f, 0.0f };		//! 弾数(右下)。予備弾も出すので長くなり、右端からはみ出さない位置にしている。
	const Vector3 vObjectivePos_ = { -420.0f, 500.0f, 0.0f };	//! 目標(上)。
	const Vector3 vItemPos_ = { -160.0f, -440.0f, 0.0f };		//! アイテム所持数(下・中央)。
	const Vector3 vStatusPos_ = { -230.0f, 140.0f, 0.0f };	//! 状態(中央やや上)。
	const float fHudFontScale_ = 0.8f;						//! 通常表示の大きさ。
	const float fStatusFontScale_ = 2.0f;					//! 状態表示の大きさ。
	const Vector3 vPausePos_ = { -340.0f, 40.0f, 0.0f };		//! ポーズ表示(中央やや上)。
	const float fPauseFontScale_ = 1.3f;					//! ポーズ表示の大きさ。
	const Vector3 vHitMarkerPos_ = { -14.0f, 18.0f, 0.0f };	//! ヒットマーカー(クロスヘアへ重ねる)。
	const Vector3 vDamagePos_ = { 40.0f, 60.0f, 0.0f };		//! ダメージ数値(クロスヘアの右上)。
	const float fHitMarkerScale_ = 1.4f;					//! ヒットマーカーの大きさ。
	const float fDamageScale_ = 1.0f;						//! ダメージ数値の大きさ。
	const float fHitMarkerLifeTime_ = 0.12f;				//! ヒットマーカーの表示時間(秒)。

	/*
	 * 弱点(頭)に当てたときは、普通の命中とはっきり差をつける。
	 * 数値は出さない方針なので、印の大きさ・形・色・残る長さで伝える。
	 */
	const wchar_t* sHitMarkerText_ = L"X";					//! 命中の印。
	/*
	 * 弱点に当てた印。普通の命中と形で区別する。
	 * ※フォントに無い文字を渡すと SpriteFont が例外を投げて落ちる。
	 *   新しい記号を使うときは、すでに画面に出ている文字に寄せること。
	 */
	const wchar_t* sCriticalMarkerText_ = L"[X]";				//! 弱点に当てた印。
	const Vector3 vCriticalMarkerPos_ = { -46.0f, 22.0f, 0.0f };	//! 弱点の印の位置(大きくなるぶん左へずらして中央に合わせる)。
	const float fCriticalMarkerScale_ = 2.1f;				//! 弱点の印の大きさ。
	const float fCriticalMarkerLifeTime_ = 0.35f;			//! 弱点の印の表示時間(秒)。普通の命中より長く残す。
	const float fDamageLifeTime_ = 0.6f;					//! ダメージ数値の表示時間(秒)。
	const char* sDamageOverlayPath_ = "Assets/sprite/white.dds";	//! 被弾時に重ねる幕(白い画像を赤く染めて使う)。
	const float fDamageFlashTime_ = 0.35f;					//! 被弾したときに赤い幕を出す時間(秒)。
	const float fDamageFlashAlpha_ = 0.45f;					//! 被弾した瞬間の赤い幕の濃さ。

	/* 表示に使う色。RGBA。*/
	const Vector4 vWhiteColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };		//! 白(クロスヘア・弾数・通常の命中)。
	const Vector4 vHpColor_ = { 0.6f, 1.0f, 0.6f, 1.0f };			//! HP表示の薄緑。
	const Vector4 vObjectiveColor_ = { 1.0f, 0.9f, 0.4f, 1.0f };	//! 目標表示の黄。
	const Vector4 vItemColor_ = { 0.6f, 0.9f, 1.0f, 1.0f };			//! アイテム表示の水色。
	const Vector4 vStatusColor_ = { 1.0f, 0.3f, 0.3f, 1.0f };		//! ダウン表示の赤。
	const Vector4 vPauseColor_ = { 1.0f, 0.95f, 0.4f, 1.0f };		//! ポーズ表示の黄。
	const Vector4 vCriticalMarkerColor_ = { 1.0f, 0.8f, 0.1f, 1.0f };	//! 弱点に当てた印の濃い黄。
	const Vector4 vCriticalDamageColor_ = { 1.0f, 0.85f, 0.2f, 1.0f };	//! 弱点のダメージ数値の色。

	const float fCrosshairScale_ = 0.7f;				//! クロスヘアの線の大きさ。
	const float fOverlayWidth_ = 1920.0f;				//! 被弾の幕の幅(画面いっぱいに広げる)。
	const float fOverlayHeight_ = 1080.0f;				//! 被弾の幕の高さ。
	const float fPulseCenter_ = 0.5f;					//! sin波(-1〜1)を0〜1へ均すための中心と振れ幅。
	const Vector3 vOverlayColor_ = { 1.0f, 0.0f, 0.0f };	//! 被弾の幕の色(赤)。濃さは別に掛ける。
	const float fLowHpRate_ = 0.35f;						//! この割合を下回ると画面が脈打ち始める。
	const float fLowHpPulseSpeed_ = 4.0f;					//! 脈打つ速さ。
	const float fLowHpMaxAlpha_ = 0.30f;					//! 脈打つときの赤の濃さの上限。
	/*
	 * ダメージ数値を表示するか。
	 * 本作は L4D2 を基盤にしているため、数値表示は画面の雰囲気に合わない。
	 * 実装は残しつつ、今は表示しない方針にしている。
	 */
	const bool bShowDamageNumber_ = false;
}

namespace nsApp
{
	namespace nsUI
	{
		bool InGameHud::Start()
		{
			/* クロスヘアの4本線。位置は毎フレーム拡散に応じて動かす。*/
			const wchar_t* aCrosshairText[4] = { L"|", L"|", L"-", L"-" };
			for (int i = 0; i < 4; i++)
			{
				aCrosshair_[i].SetScale(fCrosshairScale_);
				aCrosshair_[i].SetColor(vWhiteColor_);
				aCrosshair_[i].SetText(aCrosshairText[i]);
			}

			/* HP(左下・緑寄り)。*/
			stHpText_.SetPosition(vHpPos_);
			stHpText_.SetScale(fHudFontScale_);
			stHpText_.SetColor(vHpColor_);
			stHpText_.SetText(wcHp_);

			/* 弾数(右下)。*/
			stAmmoText_.SetPosition(vAmmoPos_);
			stAmmoText_.SetScale(fHudFontScale_);
			stAmmoText_.SetColor(vWhiteColor_);
			stAmmoText_.SetText(wcAmmo_);

			/* 目標(上・固定文言)。*/
			stObjective_.SetPosition(vObjectivePos_);
			stObjective_.SetScale(fHudFontScale_);
			stObjective_.SetColor(vObjectiveColor_);
			stObjective_.SetText(L"OBJECTIVE: REACH THE SAFE ROOM");

			/* アイテム所持数(下・中央)。*/
			stItemText_.SetPosition(vItemPos_);
			stItemText_.SetScale(fHudFontScale_);
			stItemText_.SetColor(vItemColor_);
			stItemText_.SetText(wcItem_);

			/* 状態(ダウン等。通常時は空・赤)。*/
			stStatusText_.SetPosition(vStatusPos_);
			stStatusText_.SetScale(fStatusFontScale_);
			stStatusText_.SetColor(vStatusColor_);
			stStatusText_.SetText(wcStatus_);

			/* ポーズ表示(中央・通常時は空)。*/
			stPauseText_.SetPosition(vPausePos_);
			stPauseText_.SetScale(fPauseFontScale_);
			stPauseText_.SetColor(vPauseColor_);
			stPauseText_.SetText(L"");

			/* 被弾したときに画面へ重ねる赤い幕。最初は透明にしておく。*/
			stDamageOverlay_.Init(sDamageOverlayPath_, fOverlayWidth_, fOverlayHeight_);
			stDamageOverlay_.SetMulColor({ vOverlayColor_.x, vOverlayColor_.y, vOverlayColor_.z, 0.0f });
			stDamageOverlay_.Update();

			/* ヒットマーカー(通常時は空)。*/
			stHitMarker_.SetPosition(vHitMarkerPos_);
			stHitMarker_.SetScale(fHitMarkerScale_);
			stHitMarker_.SetText(L"");

			/* ダメージ数値(通常時は空)。*/
			stDamageText_.SetPosition(vDamagePos_);
			stDamageText_.SetScale(fDamageScale_);
			stDamageText_.SetText(wcDamage_);

			/* 命中の通知を受け取れるよう購読する。*/
			nsEvent::EventBus* pBus = FindGO<nsEvent::EventBus>("eventBus");
			if (pBus != nullptr)
				pBus->Subscribe(this);

			return true;
		}


		void InGameHud::Update()
		{
			/* プレイヤーを取得する。いなければ表示を空にする。*/
			nsActor::Player* pPlayer = FindGO<nsActor::Player>("player");
			if (pPlayer == nullptr)
			{
				wcHp_[0] = L'\0';
				wcAmmo_[0] = L'\0';
				wcStatus_[0] = L'\0';
				stHpText_.SetText(wcHp_);
				stAmmoText_.SetText(wcAmmo_);
				stStatusText_.SetText(wcStatus_);
				return;
			}

			/* HP。*/
			swprintf_s(wcHp_, L"HP %d/%d", pPlayer->GetCurrentHP(), pPlayer->GetMaxHP());
			stHpText_.SetText(wcHp_);

			/* 弾数(リロード中は表示を切り替える)。武器名(narrow)は %hs で埋め込む。*/
			nsWeapon::Weapon* pWeapon = pPlayer->GetEquippedWeapon();
			if (pWeapon != nullptr)
			{
				if (pWeapon->IsReloading())
					swprintf_s(wcAmmo_, L"%hs  RELOADING", pWeapon->GetName());
				else
					/* サブ武器は予備弾が無限なので、数の代わりに印を出す。*/
					if (pWeapon->IsInfiniteReserve())
						swprintf_s(wcAmmo_, L"%hs  %d / --", pWeapon->GetName(), pWeapon->GetCurrentAmmo());
					else
						swprintf_s(wcAmmo_, L"%hs  %d / %d", pWeapon->GetName(), pWeapon->GetCurrentAmmo(), pWeapon->GetReserveAmmo());
			}
			else
			{
				wcAmmo_[0] = L'\0';
			}
			stAmmoText_.SetText(wcAmmo_);

			/* アイテム所持数(回復/投擲)。*/
			swprintf_s(wcItem_, L"MEDKIT x%d    GRENADE x%d   ( H / G )", pPlayer->GetMedkitCount(), pPlayer->GetGrenadeCount());
			stItemText_.SetText(wcItem_);

			/* 状態(ダウン中は出血残り秒を大きく表示。それ以外は空)。*/
			if (pPlayer->GetLifeState() == nsActor::EnLifeState::Down)
			{
				const int iRemain = static_cast<int>(pPlayer->GetBleedOutRemain()) + 1;
				swprintf_s(wcStatus_, L"DOWNED  -  %d", iRemain);
			}
			else
			{
				wcStatus_[0] = L'\0';
			}
			stStatusText_.SetText(wcStatus_);

			/* 拡散が大きいほどクロスヘアを広げ、いまの精度が見て分かるようにする。*/
			float fGap = fCrosshairBaseGap_ + pPlayer->GetCurrentSpread() * fCrosshairSpreadScale_;
			if (fGap > fCrosshairMaxGap_)
				fGap = fCrosshairMaxGap_;

			/* 上下左右へ振り分けて配置する。*/
			Vector3 vUp = vCrosshairCenter_;
			vUp.y += fGap;
			aCrosshair_[0].SetPosition(vUp);

			Vector3 vDown = vCrosshairCenter_;
			vDown.y -= fGap;
			aCrosshair_[1].SetPosition(vDown);

			Vector3 vLeft = vCrosshairCenter_;
			vLeft.x -= fGap;
			aCrosshair_[2].SetPosition(vLeft);

			Vector3 vRight = vCrosshairCenter_;
			vRight.x += fGap;
			aCrosshair_[3].SetPosition(vRight);

			/* 被弾した赤い幕を時間で薄くする。*/
			if (fDamageFlashTimer_ > 0.0f)
				fDamageFlashTimer_ -= g_gameTime->GetFrameDeltaTime();

			/* HPが少ないほど、ゆっくり脈打たせて危険を伝える。*/
			fLowHpPulse_ += g_gameTime->GetFrameDeltaTime() * fLowHpPulseSpeed_;

			float fOverlayAlpha = 0.0f;

			/* 被弾直後の幕。*/
			if (fDamageFlashTimer_ > 0.0f)
				fOverlayAlpha = fDamageFlashAlpha_ * (fDamageFlashTimer_ / fDamageFlashTime_);

			/* HPが少ないときの脈打ち。濃いほうを採用する。*/
			const int iMaxHP = pPlayer->GetMaxHP();
			if (iMaxHP > 0)
			{
				const float fHpRate = static_cast<float>(pPlayer->GetCurrentHP()) / static_cast<float>(iMaxHP);
				if (fHpRate < fLowHpRate_)
				{
					/* HPが低いほど濃く、sin波で明滅させる。*/
					const float fDanger = 1.0f - (fHpRate / fLowHpRate_);
					const float fPulse = (sinf(fLowHpPulse_) * fPulseCenter_ + fPulseCenter_);
					const float fLowHpAlpha = fLowHpMaxAlpha_ * fDanger * fPulse;
					if (fLowHpAlpha > fOverlayAlpha)
						fOverlayAlpha = fLowHpAlpha;
				}
			}

			stDamageOverlay_.SetMulColor({ vOverlayColor_.x, vOverlayColor_.y, vOverlayColor_.z, fOverlayAlpha });
			stDamageOverlay_.Update();

			/* 命中の手応えを時間で消す。*/
			const float fDeltaTime = g_gameTime->GetFrameDeltaTime();
			if (fHitMarkerTimer_ > 0.0f)
				fHitMarkerTimer_ -= fDeltaTime;
			if (fDamageTimer_ > 0.0f)
				fDamageTimer_ -= fDeltaTime;

			/*
			 * 表示中だけ命中の印を出す。
			 * 弱点(頭)は、形・大きさ・色・残る長さの4つを変えて普通の命中と区別する。
			 * 色だけだと一瞬すぎて気づけないため。
			 */
			if (fHitMarkerTimer_ > 0.0f)
			{
				if (bLastHitCritical_)
				{
					stHitMarker_.SetText(sCriticalMarkerText_);
					stHitMarker_.SetPosition(vCriticalMarkerPos_);
					stHitMarker_.SetScale(fCriticalMarkerScale_);
					stHitMarker_.SetColor(vCriticalMarkerColor_);
				}
				else
				{
					stHitMarker_.SetText(sHitMarkerText_);
					stHitMarker_.SetPosition(vHitMarkerPos_);
					stHitMarker_.SetScale(fHitMarkerScale_);
					stHitMarker_.SetColor(vWhiteColor_);
				}
			}
			else
			{
				stHitMarker_.SetText(L"");
			}

			if (fDamageTimer_ <= 0.0f)
			{
				stDamageText_.SetText(L"");
			}

			/* ポーズ中だけ中央にPAUSED表示。*/
			if (nsSystem::IsGamePaused())
				stPauseText_.SetText(L"- PAUSED -    [Esc] 再開    [Enter] タイトルへ");
			else
				stPauseText_.SetText(L"");
		}


		void InGameHud::OnGameEvent(const nsEvent::GameEvent& stEvent)
		{
			/* 攻撃を受けたら画面を赤く光らせる。*/
			if (stEvent.enType_ == nsEvent::EnGameEvent::PlayerDamaged)
			{
				fDamageFlashTimer_ = fDamageFlashTime_;
				return;
			}

			/* 命中以外の通知では何もしない。*/
			if (stEvent.enType_ != nsEvent::EnGameEvent::BulletHit)
				return;

			/* 印を出し、弱点かどうかを覚えておく。*/
			bLastHitCritical_ = stEvent.bIsCritical_;

			/* 弱点は気づけるよう長めに残す。*/
			fHitMarkerTimer_ = bLastHitCritical_ ? fCriticalMarkerLifeTime_ : fHitMarkerLifeTime_;

			/* ダメージ量が乗っていれば数値も出す(表示しない方針のときは何もしない)。*/
			if (!bShowDamageNumber_ || stEvent.iParam_ <= 0)
				return;

			fDamageTimer_ = fDamageLifeTime_;
			swprintf_s(wcDamage_, L"%d", stEvent.iParam_);
			stDamageText_.SetText(wcDamage_);

			/* 弱点なら数値の色も変える。*/
			if (stEvent.bIsCritical_)
				stDamageText_.SetColor(vCriticalDamageColor_);
			else
				stDamageText_.SetColor(vWhiteColor_);
		}


		void InGameHud::Render(RenderContext& rc)
		{
			/* 被弾の幕を最初に描いて、他のUIはその上に重ねる。*/
			stDamageOverlay_.Draw(rc);

			/* クロスヘアと各種テキストを描画する。*/
			for (int i = 0; i < 4; i++)
				aCrosshair_[i].Draw(rc);
			stHpText_.Draw(rc);
			stAmmoText_.Draw(rc);
			stObjective_.Draw(rc);
			stItemText_.Draw(rc);
			stStatusText_.Draw(rc);
			stPauseText_.Draw(rc);
			stHitMarker_.Draw(rc);
			stDamageText_.Draw(rc);
		}
	}
}
