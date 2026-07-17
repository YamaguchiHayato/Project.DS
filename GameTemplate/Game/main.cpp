#include "stdafx.h"
#include "system/system.h"
#include "Game.h"
#include "Title.h"
#include "GameSettings.h"


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

	// ★乱数シードを初期化する。
	// これが無いと rand()（クリティカル抽選・スキル発動抽選など）が毎起動まったく
	// 同じ出目になり、「確率」が固定パターンになってしまう。起動時に一度だけ種を蒔く。
	srand(static_cast<unsigned int>(time(nullptr)));

	// ゲームの初期化。
	InitGame(hInstance, hPrevInstance, lpCmdLine, nCmdShow, TEXT("Game"));
	
	// k2EngineLowの初期化。
	g_k2EngineLow = new K2EngineLow();
	g_k2EngineLow->Init(g_hWnd, FRAME_BUFFER_W, FRAME_BUFFER_H);

	// ★ライブ設定・音量設定を読み込んで適用する（サウンドエンジン初期化後に行うこと）。
	GameSettings::Get().Load();
	g_camera3D->SetPosition({ 0.0f, 10.0f, -30.0f });
	g_camera3D->SetTarget({ 0.0f, 10.0f, 0.0f });
	g_gameTime->EnableFixedFrameDeltaTime(0.0166f);
	//NewGO<Game>(0, "game"); // ゲームオブジェクトを生成。
	NewGO<Title>(0, "title"); // タイトルオブジェクトを生成。
	// ここからゲームループ。
	while (DispatchWindowMessage()&&g_gameLoop.m_isLoop==true)
	{
		// フレームの開始時に呼び出す必要がある処理を実行
		g_k2EngineLow->BeginFrame();

		// ゲームオブジェクトマネージャーの更新処理を呼び出す。
		g_k2EngineLow->ExecuteUpdate();

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

