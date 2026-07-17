#pragma once

#include "sound/SoundSource.h"
namespace nsK2EngineLow {

	// ==========================================================
	// サウンドID（SE / BGM の一元管理）。
	//   ・曲本体(動的に読み込む)以外の「静的WAV」をここで一括管理する。
	//   ・値は WaveFileBank の登録番号を兼ねる。曲本体=0 と衝突しない番号にすること。
	//   ・音を増やしたい時は、ここに1行足して SE::Init の登録に1行足すだけ。
	// ==========================================================
	enum EnSound {
		SND_HIT        = 1, // 判定音 (shyan.wav)
		SND_TITLE_CALL = 3, // タイトルコール
		SND_TITLE_BGM  = 4, // タイトルBGM
		SND_TAP        = 5, // タップ音（決定/カーソル）
		SND_SP_VOICE   = 6, // SP演出ボイス
	};

	// サウンドを一元管理するクラス。
	//   ・Init() で全静的WAVを一括登録（アプリ内で実登録は初回1回だけ）。
	//   ・Play(番号) で任意のSE/BGMを再生。ループ(BGM)は停止用に SoundSource* を返す。
	//   ・音量は自動で BGM音量(ループ) / SE音量(ワンショット) を掛けて再生する。
	class SE :public IGameObject {
	public:
		void Init();

		// 後方互換：タップ音を鳴らす（従来の m_tap->Play(false) 用）。
		void Play(bool isLoop);

		// サウンドIDで再生する。
		//   isLoop=true（BGM）の場合は、呼び出し側が Stop/DeleteGO できるよう SoundSource* を返す。
		//   isLoop=false（ワンショットSE）の場合は再生完了で自動解放されるので戻り値は保持不要。
		SoundSource* Play(int soundId, bool isLoop = false);
	};
}
