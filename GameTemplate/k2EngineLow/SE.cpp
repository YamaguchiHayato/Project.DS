#include "k2EngineLowPreCompile.h"
#include "SE.h"
#include "sound/SoundEngine.h"


using namespace nsK2EngineLow;

// 全静的WAVを一元登録する。実際の登録はアプリ内で初回1回だけ（SoundEngineはアプリ寿命で常駐）。
void nsK2EngineLow::SE::Init() {
	static bool s_registered = false;
	if (s_registered) return;
	g_soundEngine->ResistWaveFileBank(SND_HIT,        "Assets/SE/shyan.wav");
	g_soundEngine->ResistWaveFileBank(SND_TITLE_CALL, "Assets/SE/TitleColl.wav");
	g_soundEngine->ResistWaveFileBank(SND_TITLE_BGM,  "Assets/SE/Title.wav");
	g_soundEngine->ResistWaveFileBank(SND_TAP,        "Assets/SE/Tap.wav");
	g_soundEngine->ResistWaveFileBank(SND_SP_VOICE,   "Assets/SE/SPEffectVoice.wav");
	s_registered = true;
}

// 後方互換：従来のタップ音。
void nsK2EngineLow::SE::Play(bool isLoop) {
	Play(SND_TAP, isLoop);
}

// サウンドIDで再生。
SoundSource* nsK2EngineLow::SE::Play(int soundId, bool isLoop) {
	// ワンショットは再生完了で自動解放される（ループはユーザーが保持・停止）。
	auto* s = NewGO<SoundSource>(0, "se");
	s->Init(soundId);
	// ループ=BGM扱いでBGM音量、ワンショット=SE扱いでSE音量を適用（メニューの音量調整が効く）。
	s->SetVolume(isLoop ? g_soundEngine->GetBgmVolume() : g_soundEngine->GetSeVolume());
	s->Play(isLoop);
	return s;
}
