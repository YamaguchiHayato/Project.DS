#include "stdafx.h"
#include "TitleUI.h"
#include "Src/InputObserver.h"

namespace
{
	/*
	 * 1枚目(PRESS ANY KEY)の配置。
	 * ロゴを画面の横中央に置き、その下に案内を出す。
	 * ロゴは画像ができるまで文字で置いておく。
	 * Assets/image/title/Title.DDS は画面全体の構成を描いたラフなので、ロゴとしては使わない。
	 */
	const float fLogoPositionY_ = 150.0f;						//! ロゴの縦の位置。
	const float fLogoFontScale_ = 2.6f;							//! ロゴの文字の大きさ。
	const float fPressAnyKeyPositionY_ = -140.0f;				//! PRESS ANY KEY の縦の位置(ロゴの下)。
	const float fPressAnyKeyScale_ = 1.1f;						//! PRESS ANY KEY の大きさ。
	const float fBlinkCycle_ = 1.4f;							//! 点滅の周期(秒)。
	const float fBlinkMinAlpha_ = 0.25f;						//! 点滅で一番薄くなるときの濃さ。
	const float fBlinkMaxAlpha_ = 1.0f;							//! 点滅で一番濃くなるときの濃さ。
	const float fPulseCenter_ = 0.5f;							//! sin波を0〜1へ均すための中心と振れ幅。

	/* 2枚目(メニュー)の配置。右に立つモデルとぶつからないよう、左下へ寄せる。*/
	const float fMenuLeft_ = -790.0f;							//! メニューの文字の左端。
	const float fMenuTopPositionY_ = -146.0f;					//! メニュー1行目の縦の位置。
	const float fMenuLineHeight_ = 58.0f;						//! メニューの行間。
	const float fMenuFontScale_ = 1.0f;							//! メニューの文字の大きさ。

	/* 選んでいる行に敷く帯。どこを選んでいるかを色だけに頼らずに示す。*/
	const char* sWhiteSpritePath_ = "Assets/sprite/white.dds";	//! 帯に使う白い画像(色を掛けて使う)。
	const float fMenuBarWidth_ = 700.0f;						//! 帯の幅。
	const float fMenuBarHeight_ = 50.0f;						//! 帯の高さ。
	const float fMenuBarLeft_ = -820.0f;						//! 帯の左端。文字より少し外へ出す。
	const Vector2 vMenuBarPivotLeft_ = { 0.0f, 0.5f };			//! 帯の基準点を左端に置く。

	/* クレジットは下の中央、バージョンは右下。*/
	const float fCreditPositionY_ = -470.0f;					//! クレジットの縦の位置。
	const float fVersionPositionY_ = -470.0f;					//! バージョンの縦の位置。
	const float fVersionRight_ = 890.0f;						//! バージョンの文字の右端。
	const float fCornerFontScale_ = 0.7f;						//! 下に出す小さい文字の大きさ。

	const float fInputGuardTime_ = 0.2f;						//! 表示が切り替わった直後に入力を受けない時間(秒)。

	/*
	 * 画面に出す文字。
	 * ※フォントに無い文字を渡すと SpriteFont が例外を投げて落ちる。
	 *   ほかの画面と同じくASCIIにそろえておくこと。
	 */
	const wchar_t* sLogoText_ = L"DEPARTED SOULTAKER";		//! ロゴの仮置き。@todo ロゴ画像ができたら消す。
	const wchar_t* sPressAnyKeyText_ = L"PRESS ANY KEY";		//! 最初の画面に出す案内。
	const wchar_t* aMenuText_[] = { L"START", L"OPTION", L"GITHUB", L"END" };	//! メニューの各行。
	const wchar_t* sCreditText_ = L"(C) 2026 Departed Soultaker";	//! クレジット。@todo 連名の表記が決まったら差し替える。
	const wchar_t* sVersionText_ = L"Ver 0.1.0";				//! バージョン。@todo 提出や配布のたびに更新する。

	/* メニューの行と行き先の対応。行の並びと同じ順に書くこと。*/
	const nsApp::nsUI::EnTitleCommand aMenuCommand_[] =
	{
		nsApp::nsUI::EnTitleCommand::StartGame,		//! START。
		nsApp::nsUI::EnTitleCommand::Option,		//! OPTION。
		nsApp::nsUI::EnTitleCommand::OpenGitHub,	//! GITHUB。
		nsApp::nsUI::EnTitleCommand::ExitGame,		//! END。
	};

	static_assert(sizeof(aMenuText_) / sizeof(aMenuText_[0]) == nsApp::nsUI::TitleUI::iMenuCount_, "メニューの行数と文字の数を合わせること。");
	static_assert(sizeof(aMenuCommand_) / sizeof(aMenuCommand_[0]) == nsApp::nsUI::TitleUI::iMenuCount_, "メニューの行数と行き先の数を合わせること。");

	/* 表示に使う色。RGBA。*/
	const Vector4 vWhiteColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };			//! ロゴと PRESS ANY KEY の白。
	const Vector4 vMenuNormalColor_ = { 0.88f, 0.88f, 0.90f, 1.0f };	//! 選んでいない行の白。
	const Vector4 vMenuSelectedColor_ = { 0.10f, 0.10f, 0.12f, 1.0f };	//! 選んでいる行の黒。明るい帯の上に乗せる。
	const Vector4 vMenuBarColor_ = { 0.92f, 0.92f, 0.95f, 0.82f };		//! 選んでいる行に敷く帯の白。
	const Vector4 vCornerColor_ = { 0.80f, 0.80f, 0.82f, 1.0f };		//! クレジットとバージョンの灰。

	/**
	 * @struct VirtualKeyRange
	 * @brief 「何か押されたか」を見るときに走査する仮想キーの範囲。
	 */
	struct VirtualKeyRange
	{
		int iFirst;	//! 範囲の最初。
		int iLast;	//! 範囲の最後。
	};

	/*
	 * 走査する仮想キーの一覧。
	 * ※仮想キーを 0x01〜0xFE まで端から見てはいけない。
	 *   日本語キーボードでは 0xF2(VK_OEM_COPY) と 0xF4(VK_OEM_ENLW) が
	 *   何も押していなくても「押されている」と返り続ける。
	 *   そのまま見ると常に押されている扱いになり、「押した瞬間」が永久に来なくなる。
	 * 実際に押せるキーだけを挙げ、未割り当てと IME/OEM の領域(0xE0以降)は外している。
	 * マウスのボタンも外している。ウィンドウを前に出すためのクリックで先へ進んでしまうため。
	 */
	const VirtualKeyRange aScanKeyRange_[] =
	{
		{ VK_BACK, VK_TAB },		//! Backspace と Tab。
		{ VK_RETURN, VK_RETURN },	//! Enter。
		{ VK_SHIFT, VK_CAPITAL },	//! Shift / Ctrl / Alt / Pause / CapsLock。
		{ VK_ESCAPE, VK_ESCAPE },	//! Esc。
		{ VK_SPACE, VK_DELETE },	//! Space、PageUp/Down、End、Home、方向キー、Insert、Delete など。
		{ '0', '9' },				//! 数字。
		{ 'A', 'Z' },				//! 英字。
		{ VK_NUMPAD0, VK_DIVIDE },	//! テンキー。
		{ VK_F1, VK_F24 },			//! ファンクションキー。
		{ VK_LSHIFT, VK_RMENU },	//! 左右の Shift / Ctrl / Alt。
		{ VK_OEM_1, VK_OEM_3 },		//! 記号キー( ; = , - . / ` )。
		{ VK_OEM_4, VK_OEM_8 },		//! 記号キー( [ \ ] ' )。
	};

	/**
	 * @brief キーが1つでも押されているかを調べる。
	 * @return 1つでも押されていたら true。
	 */
	bool IsAnyKeyPress()
	{
		/* 押されているものが見つかった時点で打ち切る。*/
		for (const VirtualKeyRange& stRange : aScanKeyRange_)
		{
			for (int iKey = stRange.iFirst; iKey <= stRange.iLast; iKey++)
			{
				if ((GetAsyncKeyState(iKey) & 0x8000) != 0)
					return true;
			}
		}

		return false;
	}

	/**
	 * @brief 文字を画面の横中央に置く。
	 * @param stFont 位置を決める文字。文字と大きさを入れてから呼ぶこと。
	 * @param fPositionY 縦の位置。
	 * @details FontRender の位置は文字の左上なので、実寸の半分だけ左へずらすと中央に来る。
	 */
	void SetCenterPosition(FontRender& stFont, float fPositionY)
	{
		const Vector2 vSize = stFont.MeasureText();
		stFont.SetPosition({ -vSize.x * 0.5f, fPositionY, 0.0f });
	}

	/**
	 * @brief 文字の右端をそろえて置く。
	 * @param stFont 位置を決める文字。文字と大きさを入れてから呼ぶこと。
	 * @param fRight 右端の位置。
	 * @param fPositionY 縦の位置。
	 */
	void SetRightPosition(FontRender& stFont, float fRight, float fPositionY)
	{
		const Vector2 vSize = stFont.MeasureText();
		stFont.SetPosition({ fRight - vSize.x, fPositionY, 0.0f });
	}
}

namespace nsApp
{
	namespace nsUI
	{
		bool TitleUI::Start()
		{
			/* ロゴを用意する。画像ができるまでは文字で置いておく。*/
			stLogoFont_.SetScale(fLogoFontScale_);
			stLogoFont_.SetColor(vWhiteColor_);
			stLogoFont_.SetText(sLogoText_);
			SetCenterPosition(stLogoFont_, fLogoPositionY_);

			/* PRESS ANY KEY を用意する。濃さは毎フレーム入れ直すので、ここでは位置と大きさだけ決める。*/
			stPressAnyKeyFont_.SetScale(fPressAnyKeyScale_);
			stPressAnyKeyFont_.SetColor(vWhiteColor_);
			stPressAnyKeyFont_.SetText(sPressAnyKeyText_);
			SetCenterPosition(stPressAnyKeyFont_, fPressAnyKeyPositionY_);

			/* メニューの各行を、左端をそろえて上から行間ぶんずつ下げて並べる。*/
			for (int iIndex = 0; iIndex < iMenuCount_; iIndex++)
			{
				aMenuFont_[iIndex].SetPosition({ fMenuLeft_, fMenuTopPositionY_ - fMenuLineHeight_ * iIndex, 0.0f });
				aMenuFont_[iIndex].SetScale(fMenuFontScale_);
				aMenuFont_[iIndex].SetColor(vMenuNormalColor_);
				aMenuFont_[iIndex].SetText(aMenuText_[iIndex]);
			}

			/* 帯を行の高さに合わせるため、文字の実寸を控えておく。*/
			fMenuTextHeight_ = aMenuFont_[0].MeasureText().y;

			/* 選んでいる行に敷く帯を用意する。*/
			stMenuBarSprite_.Init(sWhiteSpritePath_, fMenuBarWidth_, fMenuBarHeight_);
			stMenuBarSprite_.SetPivot(vMenuBarPivotLeft_);
			stMenuBarSprite_.SetMulColor(vMenuBarColor_);

			/* クレジットは下の中央、バージョンは右下に置く。*/
			stCreditFont_.SetScale(fCornerFontScale_);
			stCreditFont_.SetColor(vCornerColor_);
			stCreditFont_.SetText(sCreditText_);
			SetCenterPosition(stCreditFont_, fCreditPositionY_);

			stVersionFont_.SetScale(fCornerFontScale_);
			stVersionFont_.SetColor(vCornerColor_);
			stVersionFont_.SetText(sVersionText_);
			SetRightPosition(stVersionFont_, fVersionRight_, fVersionPositionY_);

			/*
			 * 動画を飛ばした決定キーが、まだ押されたままのことがある。
			 * そのまま「押した瞬間」と見てしまわないよう、今の状態を押されていた側として控える。
			 */
			bWasAnyKeyPress_ = IsAnyKeyPress();
			RefreshMenuColor();
			UpdateMenuCursor();
			return true;
		}


		void TitleUI::Update()
		{
			const float fDeltaTime = g_gameTime->GetFrameDeltaTime();

			/* 点滅は入力を受けない間も止めない。*/
			fBlinkTimer_ += fDeltaTime;
			UpdateBlink();

			/* 今フレームのキー状態を先に取る。「押した瞬間」は前フレームとの差で見る。*/
			const bool bIsAnyKeyPress = IsAnyKeyPress();
			const bool bIsPressUp = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
			const bool bIsPressDown = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;

			/*
			 * 設定画面を開いている間と、表示が切り替わった直後は入力を受けない。
			 * 受けない間も前フレームの状態は残しておく。
			 * 残さないと、受け付けを再開した瞬間に押しっぱなしのキーが押した瞬間として入ってしまう。
			 */
			if (!bIsInputActive_ || fInputGuardTimer_ < fInputGuardTime_)
			{
				fInputGuardTimer_ += fDeltaTime;
			}
			else if (!bIsMenuPhase_)
			{
				UpdateAnyKeyPhase(bIsAnyKeyPress);
			}
			else
			{
				UpdateMenuPhase(bIsPressUp, bIsPressDown);
			}

			/* 入力を受けない間も出し続けるので、メニューの見た目は段階を見て毎フレーム作り直す。*/
			if (bIsMenuPhase_)
			{
				RefreshMenuColor();
				UpdateMenuCursor();
			}

			/* 次フレームの判定用に今の状態を残す。*/
			bWasAnyKeyPress_ = bIsAnyKeyPress;
			bWasPressUp_ = bIsPressUp;
			bWasPressDown_ = bIsPressDown;
		}


		void TitleUI::Render(RenderContext& rc)
		{
			/* 段階に応じて、ロゴと案内か、メニューのどちらかを出す。*/
			if (!bIsMenuPhase_)
			{
				stLogoFont_.Draw(rc);
				stPressAnyKeyFont_.Draw(rc);
			}
			else
			{
				/* 帯を先に描いて、その上に文字を重ねる。*/
				stMenuBarSprite_.Draw(rc);

				for (int iIndex = 0; iIndex < iMenuCount_; iIndex++)
					aMenuFont_[iIndex].Draw(rc);
			}

			/* クレジットとバージョンは常に出す。*/
			stCreditFont_.Draw(rc);
			stVersionFont_.Draw(rc);
		}


		EnTitleCommand TitleUI::PopDecidedCommand()
		{
			/* 同じ行き先を二度実行しないよう、取り出したら未決定へ戻す。*/
			const EnTitleCommand enCommand = enDecidedCommand_;
			enDecidedCommand_ = EnTitleCommand::None;
			return enCommand;
		}


		void TitleUI::SetInputActive(bool bIsActive)
		{
			bIsInputActive_ = bIsActive;

			/* 受け付けを再開した直後は、閉じるのに使ったキーで決定しないよう猶予を入れ直す。*/
			fInputGuardTimer_ = 0.0f;
		}


		void TitleUI::UpdateBlink()
		{
			/* メニューへ移ったあとは点滅する文字を出さないので、濃さも動かさない。*/
			if (bIsMenuPhase_)
				return;

			/*
			 * sin波(-1〜1)を0〜1へ均してから、濃さの幅へ広げる。
			 * FontRender は描画のたびに色へアルファを掛けるので、濃さは毎フレーム入れ直す。
			 */
			const float fPulse = sinf(fBlinkTimer_ * Math::PI2 / fBlinkCycle_) * fPulseCenter_ + fPulseCenter_;
			const float fAlpha = fBlinkMinAlpha_ + (fBlinkMaxAlpha_ - fBlinkMinAlpha_) * fPulse;
			stPressAnyKeyFont_.SetColor({ vWhiteColor_.x, vWhiteColor_.y, vWhiteColor_.z, fAlpha });
		}


		void TitleUI::UpdateAnyKeyPhase(bool bIsAnyKeyPress)
		{
			/* 何かを押した瞬間だけメニューへ移る。*/
			if (bIsAnyKeyPress && !bWasAnyKeyPress_)
				EnterMenuPhase();
		}


		void TitleUI::UpdateMenuPhase(bool bIsPressUp, bool bIsPressDown)
		{
			/* 上下キーを押した瞬間だけ、選んでいる行を動かす。端では止める。*/
			if (bIsPressUp && !bWasPressUp_ && iSelectedIndex_ > 0)
				iSelectedIndex_--;

			if (bIsPressDown && !bWasPressDown_ && iSelectedIndex_ < iMenuCount_ - 1)
				iSelectedIndex_++;

			/* 決定キーを押した瞬間に、選んでいる行を行き先として控える。*/
			if (nsData::InputObserver::Get().IsTrigger(nsData::EnAction::Confirm))
				enDecidedCommand_ = aMenuCommand_[iSelectedIndex_];
		}


		void TitleUI::EnterMenuPhase()
		{
			/* メニューを出す段階へ移り、一番上の行から選ばせる。*/
			bIsMenuPhase_ = true;
			iSelectedIndex_ = 0;

			/* 移るのに使ったキーでそのまま決定しないよう、猶予を入れる。*/
			fInputGuardTimer_ = 0.0f;
			RefreshMenuColor();
			UpdateMenuCursor();
		}


		void TitleUI::RefreshMenuColor()
		{
			/* 選んでいる行だけ色を変えて、どこにいるか分かるようにする。*/
			for (int iIndex = 0; iIndex < iMenuCount_; iIndex++)
				aMenuFont_[iIndex].SetColor(iIndex == iSelectedIndex_ ? vMenuSelectedColor_ : vMenuNormalColor_);
		}


		void TitleUI::UpdateMenuCursor()
		{
			/*
			 * 帯を、選んでいる行の文字の高さの中心に合わせる。
			 * 文字の位置は左上なので、実寸の半分だけ下げた所が中心になる。
			 */
			const float fRowPositionY = fMenuTopPositionY_ - fMenuLineHeight_ * iSelectedIndex_;
			stMenuBarSprite_.SetPosition({ fMenuBarLeft_, fRowPositionY - fMenuTextHeight_ * 0.5f, 0.0f });
			stMenuBarSprite_.Update();
		}
	}
}
