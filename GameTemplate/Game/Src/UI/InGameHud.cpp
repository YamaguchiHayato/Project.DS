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
	const Vector3 vHpPos_ = { -900.0f, -450.0f, 0.0f };		//! HP(左下)。
	const Vector3 vAmmoPos_ = { 430.0f, -470.0f, 0.0f };		//! 弾数(右下)。予備弾も出すので長くなり、右端からはみ出さない位置にしている。
	const Vector3 vObjectivePos_ = { -420.0f, 500.0f, 0.0f };	//! 目標(上)。
	const Vector3 vItemPos_ = { -330.0f, -440.0f, 0.0f };		//! アイテムスロット(下・中央)。
	const Vector3 vStatusPos_ = { -420.0f, 140.0f, 0.0f };	//! 状態(中央やや上)。
	const Vector3 vBuffPos_ = { -900.0f, -400.0f, 0.0f };		//! 効いている効果(HPの上)。
	const Vector3 vHealTextPos_ = { -110.0f, -60.0f, 0.0f };	//! 回復中の文字(クロスヘアの下)。
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
	const char* sWhiteSpritePath_ = "Assets/sprite/white.dds";	//! 幕やバーに使う白い画像(色を掛けて使う)。
	const float fDamageFlashTime_ = 0.35f;					//! 被弾したときに赤い幕を出す時間(秒)。
	const float fDamageFlashAlpha_ = 0.45f;					//! 被弾した瞬間の赤い幕の濃さ。

	/* 表示に使う色。RGBA。*/
	const Vector4 vWhiteColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };		//! 白(クロスヘア・弾数・通常の命中)。
	const Vector4 vHpColor_ = { 0.6f, 1.0f, 0.6f, 1.0f };			//! HP表示の薄緑。
	const Vector4 vHpLowColor_ = { 1.0f, 0.55f, 0.45f, 1.0f };		//! 足を引きずるほど減ったときのHP表示の赤。
	const Vector4 vObjectiveColor_ = { 1.0f, 0.9f, 0.4f, 1.0f };	//! 目標表示の黄。
	const Vector4 vItemColor_ = { 0.6f, 0.9f, 1.0f, 1.0f };			//! アイテム表示の水色。
	const Vector4 vStatusColor_ = { 1.0f, 0.3f, 0.3f, 1.0f };		//! ダウン表示の赤。
	const Vector4 vBuffColor_ = { 1.0f, 0.85f, 0.3f, 1.0f };		//! アドレナリン表示の橙。
	const Vector4 vHealTextColor_ = { 0.6f, 1.0f, 0.7f, 1.0f };		//! 回復中の文字の緑。
	const Vector4 vPauseColor_ = { 1.0f, 0.95f, 0.4f, 1.0f };		//! ポーズ表示の黄。
	const Vector4 vCriticalMarkerColor_ = { 1.0f, 0.8f, 0.1f, 1.0f };	//! 弱点に当てた印の濃い黄。
	const Vector4 vCriticalDamageColor_ = { 1.0f, 0.85f, 0.2f, 1.0f };	//! 弱点のダメージ数値の色。

	/* 体力バー。本家と同じく左下に横長で置き、恒久HPの右へ一時体力を続けて描く。*/
	const Vector3 vHealthBarPos_ = { -900.0f, -480.0f, 0.0f };	//! 体力バーの左端。
	const float fHealthBarWidth_ = 420.0f;						//! 体力バーの幅(最大HPぶん)。
	const float fHealthBarHeight_ = 18.0f;						//! 体力バーの高さ。
	const Vector2 vBarPivotLeft_ = { 0.0f, 0.5f };				//! バーの基準点を左端に置く(幅を縮めても左端が動かない)。
	const Vector4 vBarBackColor_ = { 0.05f, 0.05f, 0.05f, 0.6f };	//! バーの下地の黒。
	const Vector4 vHealthMainColor_ = { 0.35f, 0.85f, 0.35f, 0.95f };	//! 恒久HPの緑。
	const Vector4 vHealthLowColor_ = { 0.9f, 0.3f, 0.25f, 0.95f };		//! 足を引きずるほど減った恒久HPの赤。
	const Vector4 vHealthTempColor_ = { 0.9f, 0.9f, 0.75f, 0.85f };		//! 一時体力の薄い黄白。時間で減っていくのが見て分かる。

	/* メディキットの進行バー。クロスヘアの下に出す。*/
	const Vector3 vHealBarPos_ = { -150.0f, -90.0f, 0.0f };	//! 進行バーの左端。
	const float fHealBarWidth_ = 300.0f;						//! 進行バーの幅。
	const float fHealBarHeight_ = 12.0f;						//! 進行バーの高さ。
	const Vector4 vHealBarColor_ = { 0.5f, 1.0f, 0.6f, 0.95f };	//! 進行バーの中身の緑。

	const float fCrosshairScale_ = 0.7f;				//! クロスヘアの線の大きさ。
	const float fOverlayWidth_ = 1920.0f;				//! 幕の幅(画面いっぱいに広げる)。
	const float fOverlayHeight_ = 1080.0f;				//! 幕の高さ。
	const float fPulseCenter_ = 0.5f;					//! sin波(-1〜1)を0〜1へ均すための中心と振れ幅。
	const Vector3 vOverlayColor_ = { 1.0f, 0.0f, 0.0f };	//! 被弾の幕の色(赤)。濃さは別に掛ける。
	const Vector4 vGrayOverlayColor_ = { 0.45f, 0.45f, 0.45f, 0.35f };	//! 白黒のときに重ねる灰色の幕。色が抜けた感じを出す。
	const float fLowHpRate_ = 0.35f;						//! この割合を下回ると画面が脈打ち始める。
	const float fLowHpPulseSpeed_ = 4.0f;					//! 脈打つ速さ。
	const float fLowHpMaxAlpha_ = 0.30f;					//! 脈打つときの赤の濃さの上限。
	/*
	 * ダメージ数値を表示するか。
	 * 本作は L4D2 を基盤にしているため、数値表示は画面の雰囲気に合わない。
	 * 実装は残しつつ、今は表示しない方針にしている。
	 */
	const bool bShowDamageNumber_ = false;


	/**
	 * @brief 左端を基準にした横バーを用意する。
	 * @param stSprite 用意するスプライト。
	 * @param vPos     左端の位置。
	 * @param fWidth   幅(いっぱいのとき)。
	 * @param fHeight  高さ。
	 * @param vColor   色。
	 */
	void SetupBar(SpriteRender& stSprite, const Vector3& vPos, float fWidth, float fHeight, const Vector4& vColor)
	{
		stSprite.Init(sWhiteSpritePath_, fWidth, fHeight);
		stSprite.SetPivot(vBarPivotLeft_);
		stSprite.SetPosition(vPos);
		stSprite.SetMulColor(vColor);
		stSprite.Update();
	}


	/**
	 * @brief 横バーの長さを割合で決める。0で消える。
	 * @param stSprite 長さを変えるバー。
	 * @param fRate    0〜1の割合。
	 */
	void SetBarRate(SpriteRender& stSprite, float fRate)
	{
		if (fRate < 0.0f)
			fRate = 0.0f;

		if (fRate > 1.0f)
			fRate = 1.0f;

		stSprite.SetScale(Vector3(fRate, 1.0f, 1.0f));
		stSprite.Update();
	}


	/**
	 * @brief 即効アイテムの表示名を返す。
	 * @param enItem 即効アイテム。
	 * @return 表示名。持っていなければ "--"。
	 */
	const wchar_t* GetQuickItemName(nsApp::nsActor::EnQuickItem enItem)
	{
		switch (enItem)
		{
		case nsApp::nsActor::EnQuickItem::Pills:
			return L"PILLS";

		case nsApp::nsActor::EnQuickItem::Adrenaline:
			return L"ADRENALINE";

		default:
			return L"--";
		}
	}
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

			/* 体力バー(HPの文字の下)。下地→恒久HP→一時体力の順に重ねる。*/
			SetupBar(stHealthBarBack_, vHealthBarPos_, fHealthBarWidth_, fHealthBarHeight_, vBarBackColor_);
			SetupBar(stHealthBarMain_, vHealthBarPos_, fHealthBarWidth_, fHealthBarHeight_, vHealthMainColor_);
			SetupBar(stHealthBarTemp_, vHealthBarPos_, fHealthBarWidth_, fHealthBarHeight_, vHealthTempColor_);

			/* 効いている効果(アドレナリン)。通常時は空。*/
			stBuffText_.SetPosition(vBuffPos_);
			stBuffText_.SetScale(fHudFontScale_);
			stBuffText_.SetColor(vBuffColor_);
			stBuffText_.SetText(wcBuff_);

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

			/* アイテムスロット(下・中央)。*/
			stItemText_.SetPosition(vItemPos_);
			stItemText_.SetScale(fHudFontScale_);
			stItemText_.SetColor(vItemColor_);
			stItemText_.SetText(wcItem_);

			/* 状態(ダウン等。通常時は空・赤)。*/
			stStatusText_.SetPosition(vStatusPos_);
			stStatusText_.SetScale(fStatusFontScale_);
			stStatusText_.SetColor(vStatusColor_);
			stStatusText_.SetText(wcStatus_);

			/* メディキットを使っている間の文字と進行バー(通常時は空)。*/
			stHealText_.SetPosition(vHealTextPos_);
			stHealText_.SetScale(fHudFontScale_);
			stHealText_.SetColor(vHealTextColor_);
			stHealText_.SetText(L"");
			SetupBar(stHealBarBack_, vHealBarPos_, fHealBarWidth_, fHealBarHeight_, vBarBackColor_);
			SetupBar(stHealBarFill_, vHealBarPos_, fHealBarWidth_, fHealBarHeight_, vHealBarColor_);
			SetBarRate(stHealBarBack_, 0.0f);
			SetBarRate(stHealBarFill_, 0.0f);

			/* ポーズ表示(中央・通常時は空)。*/
			stPauseText_.SetPosition(vPausePos_);
			stPauseText_.SetScale(fPauseFontScale_);
			stPauseText_.SetColor(vPauseColor_);
			stPauseText_.SetText(L"");

			/* 被弾したときに画面へ重ねる赤い幕。最初は透明にしておく。*/
			stDamageOverlay_.Init(sWhiteSpritePath_, fOverlayWidth_, fOverlayHeight_);
			stDamageOverlay_.SetMulColor({ vOverlayColor_.x, vOverlayColor_.y, vOverlayColor_.z, 0.0f });
			stDamageOverlay_.Update();

			/* 白黒のときに重ねる灰色の幕。最初は透明。*/
			stGrayOverlay_.Init(sWhiteSpritePath_, fOverlayWidth_, fOverlayHeight_);
			stGrayOverlay_.SetMulColor({ vGrayOverlayColor_.x, vGrayOverlayColor_.y, vGrayOverlayColor_.z, 0.0f });
			stGrayOverlay_.Update();

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

			/* 体力。*/
			UpdateHealthBar(pPlayer);
			UpdateHealBar(pPlayer);
			UpdateStatusText(pPlayer);
			UpdateOverlay(pPlayer);

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

			/* アイテムスロット。本家の並び(3投擲/4メディキット/5即効)で出す。*/
			swprintf_s(wcItem_, L"[3] PIPE BOMB x%d    [4] MEDKIT x%d    [5] %s",
				pPlayer->GetPipeBombCount(), pPlayer->GetMedkitCount(), GetQuickItemName(pPlayer->GetQuickItem()));
			stItemText_.SetText(wcItem_);

			/* クロスヘア。*/
			UpdateCrosshair(pPlayer);

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


		void InGameHud::UpdateHealthBar(const nsActor::Player* pPlayer)
		{
			const int iMaxHP = pPlayer->GetMaxHP();
			const int iMainHP = pPlayer->GetCurrentHP();
			const int iTempHP = pPlayer->GetTempHP();

			/* 数字は合計で出す。一時体力があるときは内訳も添える。*/
			if (iTempHP > 0)
				swprintf_s(wcHp_, L"HP %d  (%d +%d)", iMainHP + iTempHP, iMainHP, iTempHP);
			else
				swprintf_s(wcHp_, L"HP %d", iMainHP);
			stHpText_.SetText(wcHp_);

			/* 足を引きずるほど減っていれば、文字とバーを赤くして危険を伝える。*/
			const bool bIsLow = pPlayer->IsLimping();
			stHpText_.SetColor(bIsLow ? vHpLowColor_ : vHpColor_);
			stHealthBarMain_.SetMulColor(bIsLow ? vHealthLowColor_ : vHealthMainColor_);

			if (iMaxHP <= 0)
				return;

			/* 恒久HPのぶんを左から描く。*/
			const float fMainRate = static_cast<float>(iMainHP) / static_cast<float>(iMaxHP);
			SetBarRate(stHealthBarMain_, fMainRate);

			/* 一時体力は恒久HPの右端から続けて描く。*/
			Vector3 vTempPos = vHealthBarPos_;
			vTempPos.x += fHealthBarWidth_ * fMainRate;
			stHealthBarTemp_.SetPosition(vTempPos);
			SetBarRate(stHealthBarTemp_, static_cast<float>(iTempHP) / static_cast<float>(iMaxHP));
		}


		void InGameHud::UpdateHealBar(const nsActor::Player* pPlayer)
		{
			/* 使っていないときは何も出さない。*/
			if (!pPlayer->IsHealing())
			{
				stHealText_.SetText(L"");
				SetBarRate(stHealBarBack_, 0.0f);
				SetBarRate(stHealBarFill_, 0.0f);
				return;
			}

			stHealText_.SetText(L"HEALING...  (HOLD 4)");
			SetBarRate(stHealBarBack_, 1.0f);
			SetBarRate(stHealBarFill_, pPlayer->GetHealProgress());
		}


		void InGameHud::UpdateStatusText(const nsActor::Player* pPlayer)
		{
			/* 状態(中央・大)。ダウン中は出血残り秒と、ピストルで撃てることを伝える。*/
			switch (pPlayer->GetLifeState())
			{
			case nsActor::EnLifeState::Down:
			{
				const int iRemain = static_cast<int>(pPlayer->GetBleedOutRemain()) + 1;
				swprintf_s(wcStatus_, L"INCAPACITATED  %d   (PISTOL ONLY)", iRemain);
				break;
			}

			case nsActor::EnLifeState::Dead:
				swprintf_s(wcStatus_, L"DEAD");
				break;

			default:
				/* 白黒(次にダウンしたら死亡)は立っている間ずっと警告する。*/
				if (pPlayer->IsBlackAndWhite())
					swprintf_s(wcStatus_, L"NEXT DOWN = DEATH  (USE MEDKIT)");
				else
					wcStatus_[0] = L'\0';
				break;
			}
			stStatusText_.SetText(wcStatus_);

			/* 効いている効果。*/
			if (pPlayer->IsAdrenalineActive())
				swprintf_s(wcBuff_, L"ADRENALINE  %d", static_cast<int>(pPlayer->GetAdrenalineRemain()) + 1);
			else
				wcBuff_[0] = L'\0';
			stBuffText_.SetText(wcBuff_);
		}


		void InGameHud::UpdateOverlay(const nsActor::Player* pPlayer)
		{
			/* 被弾した赤い幕を時間で薄くする。*/
			if (fDamageFlashTimer_ > 0.0f)
				fDamageFlashTimer_ -= g_gameTime->GetFrameDeltaTime();

			/* HPが少ないほど、ゆっくり脈打たせて危険を伝える。*/
			fLowHpPulse_ += g_gameTime->GetFrameDeltaTime() * fLowHpPulseSpeed_;

			float fOverlayAlpha = 0.0f;

			/* 被弾直後の幕。*/
			if (fDamageFlashTimer_ > 0.0f)
				fOverlayAlpha = fDamageFlashAlpha_ * (fDamageFlashTimer_ / fDamageFlashTime_);

			/* 合計HPが少ないときの脈打ち。濃いほうを採用する。*/
			const int iMaxHP = pPlayer->GetMaxHP();
			if (iMaxHP > 0)
			{
				const float fHpRate = static_cast<float>(pPlayer->GetTotalHP()) / static_cast<float>(iMaxHP);
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

			/* 白黒のときは色が抜けたように灰色を重ねる。*/
			const float fGrayAlpha = pPlayer->IsBlackAndWhite() ? vGrayOverlayColor_.w : 0.0f;
			stGrayOverlay_.SetMulColor({ vGrayOverlayColor_.x, vGrayOverlayColor_.y, vGrayOverlayColor_.z, fGrayAlpha });
			stGrayOverlay_.Update();
		}


		void InGameHud::UpdateCrosshair(const nsActor::Player* pPlayer)
		{
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
			/* 幕を最初に描いて、他のUIはその上に重ねる。*/
			stDamageOverlay_.Draw(rc);
			stGrayOverlay_.Draw(rc);

			/* 体力バー(下地→恒久→一時)。*/
			stHealthBarBack_.Draw(rc);
			stHealthBarMain_.Draw(rc);
			stHealthBarTemp_.Draw(rc);

			/* メディキットの進行バー。*/
			stHealBarBack_.Draw(rc);
			stHealBarFill_.Draw(rc);

			/* クロスヘアと各種テキストを描画する。*/
			for (int i = 0; i < 4; i++)
				aCrosshair_[i].Draw(rc);
			stHpText_.Draw(rc);
			stBuffText_.Draw(rc);
			stAmmoText_.Draw(rc);
			stObjective_.Draw(rc);
			stItemText_.Draw(rc);
			stStatusText_.Draw(rc);
			stHealText_.Draw(rc);
			stPauseText_.Draw(rc);
			stHitMarker_.Draw(rc);
			stDamageText_.Draw(rc);
		}
	}
}
