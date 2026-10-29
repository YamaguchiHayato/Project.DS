#include "stdafx.h"
#include "TitleUI.h"
#include "Src/InputObserver.h"

namespace
{
	/*
	 * ロゴは画像ができるまで文字で置いておく。
	 * Assets/image/title/Title.DDS は画面全体の構成を描いたラフなので、ロゴとしては使わない。
	 */
	const Vector3 vLogoPosition_ = { -860.0f, 300.0f, 0.0f };	//! ロゴの位置(画面の左上寄り)。
	const float fLogoFontScale_ = 2.2f;							//! ロゴの文字の大きさ。

	const Vector3 vPressAnyKeyPosition_ = { -700.0f, 60.0f, 0.0f };	//! PRESS ANY KEY の位置(ロゴの下)。
	const float fPressAnyKeyScale_ = 1.2f;						//! PRESS ANY KEY の大きさ。
	const float fBlinkCycle_ = 1.4f;							//! 点滅の周期(秒)。
	const float fBlinkMinAlpha_ = 0.25f;						//! 点滅で一番薄くなるときの濃さ。
	const float fBlinkMaxAlpha_ = 1.0f;							//! 点滅で一番濃くなるときの濃さ。
	const float fPulseCenter_ = 0.5f;							//! sin波を0〜1へ均すための中心と振れ幅。

	const Vector3 vMenuTopPosition_ = { -700.0f, 60.0f, 0.0f };	//! メニュー1行目の位置。PRESS ANY KEY と同じ場所から並べる。
	const float fMenuLineHeight_ = 84.0f;						//! メニューの行間。
	const float fMenuFontScale_ = 1.2f;							//! メニューの文字の大きさ。

	const Vector3 vCreditPosition_ = { -900.0f, -480.0f, 0.0f };	//! クレジットの位置(左下)。
	const Vector3 vVersionPosition_ = { 700.0f, -480.0f, 0.0f };	//! バージョンの位置(右下)。
	const float fCornerFontScale_ = 0.7f;						//! 四隅に出す小さい文字の大きさ。

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
	const Vector4 vWhiteColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };			//! PRESS ANY KEY の白。
	const Vector4 vMenuNormalColor_ = { 0.85f, 0.85f, 0.85f, 1.0f };	//! 選んでいない行の白。
	const Vector4 vMenuSelectedColor_ = { 1.0f, 0.85f, 0.25f, 1.0f };	//! 選んでいる行の黄。キーコンフィグと同じ見せ方にそろえている。
	const Vector4 vCornerColor_ = { 0.7f, 0.7f, 0.7f, 1.0f };			//! クレジットとバージョンの灰。

	/*
	 * 「何か押されたか」を見るときに走査する仮想キーの範囲。
	 * 0x01 から見るので、マウスのボタンも「何か」に含まれる。
	 */
	const int iVirtualKeyMin_ = 0x01;	//! 走査する仮想キーの最初。
	const int iVirtualKeyMax_ = 0xFE;	//! 走査する仮想キーの最後。

	/**
	 * @brief キーかマウスのボタンが1つでも押されているかを調べる。
	 * @return 1つでも押されていたら true。
	 */
	bool IsAnyKeyPress()
	{
		/* 押されているものが見つかった時点で打ち切る。*/
		for (int iKey = iVirtualKeyMin_; iKey <= iVirtualKeyMax_; iKey++)
		{
			if ((GetAsyncKeyState(iKey) & 0x8000) != 0)
				return true;
		}

		return false;
	}
}

namespace nsApp
{
	namespace nsUI
	{
		bool TitleUI::Start()
		{
			/* ロゴを用意する。画像ができるまでは文字で置いておく。*/
			stLogoFont_.SetPosition(vLogoPosition_);
			stLogoFont_.SetScale(fLogoFontScale_);
			stLogoFont_.SetColor(vWhiteColor_);
			stLogoFont_.SetText(sLogoText_);

			/* PRESS ANY KEY を用意する。濃さは毎フレーム入れ直すので、ここでは位置と大きさだけ決める。*/
			stPressAnyKeyFont_.SetPosition(vPressAnyKeyPosition_);
			stPressAnyKeyFont_.SetScale(fPressAnyKeyScale_);
			stPressAnyKeyFont_.SetColor(vWhiteColor_);
			stPressAnyKeyFont_.SetText(sPressAnyKeyText_);

			/* メニューの各行を、上から行間ぶんずつ下げて並べる。*/
			for (int iIndex = 0; iIndex < iMenuCount_; iIndex++)
			{
				const Vector3 vPosition = { vMenuTopPosition_.x, vMenuTopPosition_.y - fMenuLineHeight_ * iIndex, vMenuTopPosition_.z };
				aMenuFont_[iIndex].SetPosition(vPosition);
				aMenuFont_[iIndex].SetScale(fMenuFontScale_);
				aMenuFont_[iIndex].SetColor(vMenuNormalColor_);
				aMenuFont_[iIndex].SetText(aMenuText_[iIndex]);
			}

			/* クレジットとバージョンを下の両端に置く。*/
			stCreditFont_.SetPosition(vCreditPosition_);
			stCreditFont_.SetScale(fCornerFontScale_);
			stCreditFont_.SetColor(vCornerColor_);
			stCreditFont_.SetText(sCreditText_);

			stVersionFont_.SetPosition(vVersionPosition_);
			stVersionFont_.SetScale(fCornerFontScale_);
			stVersionFont_.SetColor(vCornerColor_);
			stVersionFont_.SetText(sVersionText_);

			/*
			 * 動画を飛ばした決定キーが、まだ押されたままのことがある。
			 * そのまま「押した瞬間」と見てしまわないよう、今の状態を押されていた側として控える。
			 */
			bWasAnyKeyPress_ = IsAnyKeyPress();
			RefreshMenuColor();
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

			/* 次フレームの判定用に今の状態を残す。*/
			bWasAnyKeyPress_ = bIsAnyKeyPress;
			bWasPressUp_ = bIsPressUp;
			bWasPressDown_ = bIsPressDown;
		}


		void TitleUI::Render(RenderContext& rc)
		{
			/* ロゴはどちらの段階でも出す。*/
			stLogoFont_.Draw(rc);

			/* 段階に応じて、案内かメニューのどちらかを出す。*/
			if (!bIsMenuPhase_)
			{
				stPressAnyKeyFont_.Draw(rc);
			}
			else
			{
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

			RefreshMenuColor();

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
		}


		void TitleUI::RefreshMenuColor()
		{
			/* 選んでいる行だけ色を変えて、どこにいるか分かるようにする。*/
			for (int iIndex = 0; iIndex < iMenuCount_; iIndex++)
				aMenuFont_[iIndex].SetColor(iIndex == iSelectedIndex_ ? vMenuSelectedColor_ : vMenuNormalColor_);
		}
	}
}
