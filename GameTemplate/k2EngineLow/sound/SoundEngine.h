/// <summary>
/// サウンドエンジン。
/// </summary>
#pragma once
#include "WaveFileBank.h"

namespace nsK2EngineLow {
#define INPUTCHANNELS 2  // number of source channels
#define OUTPUTCHANNELS 8	//最大出力チャンネル数。

	class WaveFile;
	class SoundSource;
	/// <summary>
	/// サウンドエンジン。
	/// </summary>
	class SoundEngine : public Noncopyable {
	public:
		/// <summary>
		/// コンストラクタ。
		/// </summary>
		SoundEngine();
		/// <summary>
		/// デストラクタ。
		/// </summary>
		~SoundEngine();
		/// <summary>
		/// 初期化。
		/// </summary>
		void Init();
		/// <summary>
		/// 開放。
		/// </summary>
		void Release();
		/// <summary>
		/// 更新。
		/// </summary>
		void Update();
		/// <summary>
		/// 3Dサウンドソースを追加。
		/// </summary>
		/// <param name="s">3Dサウンドソース。</param>
		void Add3DSoundSource(SoundSource* s)
		{
			m_3dSoundSource.push_back(s);
		}
		/// <summary>
		/// 3Dサウンドソースを削除。
		/// </summary>
		/// <param name="s">3Dサウンドソース。</param>
		void Remove3DSoundSource(SoundSource* s)
		{
			auto it = std::find(m_3dSoundSource.begin(), m_3dSoundSource.end(), s);
			if (it != m_3dSoundSource.end()) {
				m_3dSoundSource.erase(it);
			}
		}
		/// <summary>
		/// サウンドリスナーの座標を設定。
		/// 3D音源を使っている時の計算に使用されます。
		/// </summary>
		/// <param name="pos">サウンドリスナーの座標。</param>
		void SetListenerPosition(const Vector3& pos)
		{
			m_listenerPosition = pos;
		}
		/// <summary>
		/// サウンドリスナーの前方向を設定。
		/// 3D音源を使っている時の計算に使用されます。
		/// </summary>
		/// <param name="front">サウンドリスナーの前方向。</param>
		void SetListenerFront(Vector3 front)
		{
			m_listener.OrientFront.x = front.x;
			m_listener.OrientFront.y = front.y;
			m_listener.OrientFront.z = front.z;
		}
		/// <summary>
		/// サウンドリスナーの上方向を設定。
		/// 3D音源を使っている時の計算に使用されます。
		/// </summary>
		/// <param name="up">サウンドリスナーの上方向。</param>
		void SetListenerUp(Vector3 up)
		{
			m_listener.OrientTop.x = up.x;
			m_listener.OrientTop.y = up.y;
			m_listener.OrientTop.z = up.z;
		}
		/// <summary>
		/// XAudio2のソースボイスを追加。
		/// </summary>
		/// <param name="waveFile">波形データ。</param>
		/// <param name="is3DSound">trueなら3Dサウンド。</param>
		/// <returns>XAudio2のソースボイス。</returns>
		IXAudio2SourceVoice* CreateXAudio2SourceVoice(WaveFile* waveFile, bool is3DSound);
		IXAudio2MasteringVoice* GetMasteringVoice()
		{
			return m_masteringVoice;
		}
		IXAudio2SubmixVoice* GetSubmixVoice()
		{
			return m_submixVoice;
		}
		/// <summary>
		/// 出力チャンネルの数を取得。
		/// </summary>
		/// <returns></returns>
		int GetNumChannel() const
		{
			return m_nChannels;
		}
		/// <summary>
		/// 波形データバンクを取得。
		/// </summary>
		/// <returns></returns>
		WaveFileBank& GetWaveFileBank()
		{
			return m_waveFileBank;
		}
		/// <summary>
		/// 波形データを登録する。
		/// </summary>
		void ResistWaveFileBank(int number, const char* filePath)
		{
			m_waveFileBank.Resist(number, filePath);
		}

		// ==========================================
		// 音量設定（マスター/BGM/SE）。0.0〜1.0。
		//   ・マスターはマスタリングボイスに即時反映（全音に効く）。
		//   ・BGM/SEは値を保持するだけ。各音源側が再生時に GetBgmVolume/GetSeVolume を
		//     掛けて使う想定（SEクラスは自動で適用）。
		// メニューの音量調整から使う。項目追加も容易。
		// ==========================================
		void SetMasterVolume(float v)
		{
			m_masterVolume = v;
			if (m_masteringVoice) {
				m_masteringVoice->SetVolume(v);
			}
		}
		float GetMasterVolume() const { return m_masterVolume; }
		void  SetBgmVolume(float v) { m_bgmVolume = v; }
		float GetBgmVolume() const { return m_bgmVolume; }
		void  SetSeVolume(float v) { m_seVolume = v; }
		float GetSeVolume() const { return m_seVolume; }
	private:
		float m_masterVolume = 1.0f;
		float m_bgmVolume = 1.0f;
		float m_seVolume = 1.0f;
		IXAudio2* m_xAudio2 = nullptr;
		IXAudio2MasteringVoice* m_masteringVoice = nullptr;
		X3DAUDIO_HANDLE				m_hx3DAudio;						//3Dオーディオのハンドル。
		IUnknown* m_reverbEffect = nullptr;			//リバープエフェクト。
		IXAudio2SubmixVoice* m_submixVoice = nullptr;			//サブミックスボイス。
		DWORD						m_channelMask = 0;					//オーディオチャンネルマスク。
		DWORD						m_nChannels = 0;					//チャンネル数。
		Vector3						m_listenerPosition = Vector3::Zero;	//リスナーの座標。
		float						m_fListenerAngle = 0.0f;
		bool						m_UseListenerCone = false;
		bool						m_fUseInnerRadius = false;
		bool						m_useRedirectToLFE = false;
		bool						m_isInited = false;
		X3DAUDIO_LISTENER			m_listener;							//サウンドリスナー。
		std::list<SoundSource*>		m_3dSoundSource;					//3Dサウンドのサウンドソース。
		X3DAUDIO_CONE				m_emitterCone;
		WaveFileBank				m_waveFileBank;						//波形データのバンク。
	};

	extern SoundEngine* g_soundEngine;	//TKエンジン。
}