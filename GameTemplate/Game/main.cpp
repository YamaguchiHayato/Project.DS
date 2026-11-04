#include "stdafx.h"
#include "system/system.h"
#include "Src/Scene/GameFlow.h"
#include "Src/InputObserver.h"
#include "Game.h"
#include "../System/GraphicSetting/Src/GraphicsSetting.h"

// K2EngineLowのグローバルアクセスポイント。
K2EngineLow* g_k2EngineLow = nullptr;


// =====================================================================
// ★追加：カレントディレクトリ（作業フォルダ）を「Assets が存在する場所」に固定する。
//
// このゲームは "Assets/..." という相対パスでアセット読み込みやセーブ
// （ranking.json / highscores.json）を行っている。そのため実行時のCWDが
// Game/ 以外だと、アセットが読めずセーブも別の場所に書かれてしまう。
// PVLauncher やショートカットから起動されるとCWDがずれるため、
// 起動直後に exe の場所から上方向へ Assets フォルダを探し、CWDをそこへ固定する。
// これでどこから起動されても、アセット読込とセーブ先が必ず一致する。
// =====================================================================
static void FixWorkingDirectoryToAssetsRoot()
{
	wchar_t exePath[MAX_PATH] = { 0 };
	if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0) return;

	// exe のあるフォルダを起点に、最大8階層まで親をたどって "Assets" を探す。
	std::wstring dir(exePath);
	size_t slash = dir.find_last_of(L"\\/");
	if (slash == std::wstring::npos) return;
	dir = dir.substr(0, slash);

	for (int i = 0; i < 8; ++i) {
		std::wstring candidate = dir + L"\\Assets";
		DWORD attr = GetFileAttributesW(candidate.c_str());
		if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
			SetCurrentDirectoryW(dir.c_str());
			return;
		}
		// 1つ上の親フォルダへ。
		size_t up = dir.find_last_of(L"\\/");
		if (up == std::wstring::npos) break;
		dir = dir.substr(0, up);
	}
	// Assets が見つからなければCWDは変更しない（従来どおりの挙動を維持）。
}


/// <summary>
/// メイン関数
/// </summary>
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// ★アセット読み込み・セーブより前に作業フォルダを確定させる。
	FixWorkingDirectoryToAssetsRoot();

	// 拡張分。
	// グラフィックスの設定をロードする。
	nsApp::nsData::GraphicsSetting::Get().Load();

	// ★乱数シードを初期化する。
	// これが無いと rand()（クリティカル抽選・スキル発動抽選など）が毎起動まったく
	// 同じ出目になり、「確率」が固定パターンになってしまう。起動時に一度だけ種を蒔く。
	srand(static_cast<unsigned int>(time(nullptr)));

	// ゲームの初期化。
	InitGame(hInstance, hPrevInstance, lpCmdLine, nCmdShow, TEXT("RUST GUB BULLET REQUIEM_Window"));
	
	// k2EngineLowの初期化。
	g_k2EngineLow = new K2EngineLow();
	g_k2EngineLow->Init(g_hWnd, FRAME_BUFFER_W, FRAME_BUFFER_H);


	// ゲームフローを生成。
	NewGO<nsApp::nsScene::GameFlow>(0, "gameFlow");

	while (DispatchWindowMessage() && g_gameLoop.m_isLoop == true)
	{
		// フレームの開始時に呼び出す必要がある処理を実行
		g_k2EngineLow->BeginFrame();

		// 入力管理の更新処理を呼び出す。
		nsApp::nsData::InputObserver::Get().BeginFrame();

		// ゲームオブジェクトマネージャーの更新処理を呼び出す。
		g_k2EngineLow->ExecuteUpdate();

		/* 設定画面で変えた値を、その場で画面と音へ反映する。 */
		nsApp::nsData::GraphicsSetting& setting = nsApp::nsData::GraphicsSetting::Get();
		g_renderingEngine->SetSceneBrightness(setting.GetBrightness());
		g_renderingEngine->isMotionBlurEnabled = setting.GetVideo().IsON(nsApp::nsData::VideoItem::MotionBlur);

		// 音量設定を反映する。
		if (g_soundEngine != nullptr)
		{
			const float master = setting.GetSound().IsON(nsApp::nsData::SoundItem::Mute) ? 0.0f : setting.GetSound().Get(nsApp::nsData::SoundItem::Master);
			g_soundEngine->SetMasterVolume(master);
			g_soundEngine->SetBgmVolume(setting.GetSound().Get(nsApp::nsData::SoundItem::Bgm));
			g_soundEngine->SetSeVolume(setting.GetSound().Get(nsApp::nsData::SoundItem::Se));
		}

		/* 振動オフのときはモーターを止める。オンでも、今は振動を出す処理がゲーム側に無い。 */
		if (!setting.GetControl().IsON(nsApp::nsData::ControlItem::Vibration))
		{
			XINPUT_VIBRATION vibration = {};
			for (int pad = 0; pad < nsK2EngineLow::GamePad::CONNECT_PAD_MAX; pad++)
				XInputSetState(pad, &vibration);
		}

		// フレームレート設定を反映する。
		const int frameRate = setting.GetVideo().GetIndex(nsApp::nsData::VideoItem::FrameRate);
		if (frameRate == 0)
			g_k2EngineLow->SetFrameRateMode(K2EngineLow::enFrameRateMode_Fix, 30);
		else if (frameRate == 1)
			g_k2EngineLow->SetFrameRateMode(K2EngineLow::enFrameRateMode_Fix, 60);
		else
			g_k2EngineLow->SetFrameRateMode(K2EngineLow::enFrameRateMode_Variable, 240);

		// ゲームオブジェクトマネージャーの描画処理を呼び出す。
		g_k2EngineLow->ExecuteRender();

		// デバッグ描画処理を実行する。
		g_k2EngineLow->DebubDrawWorld();

		// フレームの終了時に呼び出す必要がある処理を実行。
		g_k2EngineLow->EndFrame();
	}

	delete g_k2EngineLow;

	return 0;
}

