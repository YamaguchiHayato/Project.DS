#include "stdafx.h"
#include "EffectList.h"
#include <sys/stat.h>

namespace
{
	const int kMaxEffectCount = 64;						//! 同時に再生できるエフェクト数。
	const std::u16string sEffectFolder_ = u"Assets/effect/";	//! エフェクト素材を置くフォルダ。

	/**
	 * @brief 存在確認のために、パスをナロー文字列へ変換する(ASCIIのパスのみ対応)。
	 * @param sPath 変換するパス。
	 * @return 変換後の文字列。
	 */
	std::string ToNarrowPath(const std::u16string& sPath)
	{
		/* 変換結果を組み立てる文字列。*/
		std::string sResult;

		/* 1文字ずつナロー文字へ落とし込む。*/
		for (char16_t c : sPath)
			sResult += static_cast<char>(c);

		/* 組み立てた文字列を返す。*/
		return sResult;
	}
}

namespace nsApp
{
	namespace nsEffect
	{
		/* 現在有効なリスト(所有者のシーンが設定する)。*/
		EffectList* EffectList::pActiveList_ = nullptr;


		EffectList::~EffectList()
		{
			/* 自分が有効なリストとして登録されたままなら解除する。*/
			if (pActiveList_ == this)
				pActiveList_ = nullptr;

			/* 再生中のエフェクトを破棄する。*/
			Clear();
		}


		void PlayEffect(EnEffectID enID, const Vector3& vPosition, const Quaternion& qRotation, const Vector3& vScale, float fLifeTime)
		{
			/* 有効なリストがあるときだけ再生する。*/
			EffectList* pList = EffectList::GetActiveList();
			if (pList == nullptr)
				return;

			pList->PlayEffect(enID, vPosition, qRotation, vScale, fLifeTime);
		}


		void EffectList::Init()
		{
			/* 二重登録を防ぐ。*/
			if (bInitialized_)
				return;

			bInitialized_ = true;

			/*
			 * エフェクト素材を登録する。ファイル名は拡張子込みで指定するので、
			 * .efk と .efkefc のどちらでも扱える。素材を差し替えるときはここを直す。
			 */
			RegisterEffect(EnEffectID::MuzzleFlash, u"gun/shoot.efkefc");	//! 発射の閃光。
			RegisterEffect(EnEffectID::Hit, u"gun/Blood.efkefc");			//! 着弾の血しぶき。
			RegisterEffect(EnEffectID::Explosion, u"gun/Blast.efkefc");		//! グレネードの爆発。
			RegisterEffect(EnEffectID::Heal, u"gun/Heal.efkefc");			//! 回復の輝き。
		}


		void EffectList::RegisterEffect(EnEffectID enID, const std::u16string& sFileName)
		{
			/* フォルダ名と繋いでファイルパスを組み立てる。*/
			const std::u16string sPath = sEffectFolder_ + sFileName;

			/*
			 * 素材が無い状態で登録すると不正終了する恐れがあるため、先に存在を確認する。
			 * 見つからない識別子は表に載せず、再生要求を黙って無視する。
			 */
			struct stat stFileInfo;
			const std::string sNarrowPath = ToNarrowPath(sPath);
			if (stat(sNarrowPath.c_str(), &stFileInfo) != 0)
			{
				DebugPrintW(L"[EffectList] エフェクト素材が見つかりません: %hs\n", sNarrowPath.c_str());
				return;
			}

			/* エンジンへ登録する。*/
			const uint8_t iKey = static_cast<uint8_t>(enID);
			EffectEngine::GetInstance()->ResistEffect(iKey, sPath.c_str());

			/* 登録済みかの判定に使うため、パス表にも控える。*/
			mapEffectPathList_[iKey] = sPath;
		}


		void EffectList::Update(float fDeltaTime)
		{
			/* 再生中の一覧を巡回し、寿命が切れたエフェクトを破棄する。*/
			for (auto iterator = vecPlayingEffects_.begin(); iterator != vecPlayingEffects_.end();)
			{
				/* 経過時間を進める。*/
				iterator->fCurrentTime_ += fDeltaTime;

				/* まだ寿命内なら次へ。*/
				if (iterator->fCurrentTime_ < iterator->fLifeTime_)
				{
					++iterator;
					continue;
				}

				/* 寿命が尽きたのでエミッタを破棄し、一覧から取り除く。*/
				DestroyEmitter(*iterator);
				iterator = vecPlayingEffects_.erase(iterator);
			}
		}


		void EffectList::Clear()
		{
			/* 再生中のエフェクトを全て破棄する。*/
			for (EffectInfo& stInfo : vecPlayingEffects_)
				DestroyEmitter(stInfo);

			/* 一覧を空にする。*/
			vecPlayingEffects_.clear();
		}


		void EffectList::DestroyEmitter(EffectInfo& stInfo)
		{
			/* 既に破棄されていれば何もしない。*/
			if (stInfo.pEmitter_ == nullptr)
				return;

			/*
			 * ゲームオブジェクトマネージャが生きている間だけ破棄できる。
			 * 終了処理の順番によっては先に解放されているため、その場合は参照だけ捨てる。
			 */
			if (GameObjectManager::GetInstance() != nullptr)
				DeleteGO(stInfo.pEmitter_);

			stInfo.pEmitter_ = nullptr;
		}


		EffectEmitter* EffectList::PlayEffect(EnEffectID enID, const Vector3& vPosition, const Quaternion& qRotation, const Vector3& vScale, float fLifeTime)
		{
			/* 登録されていない(素材が無い)識別子は再生しない。*/
			const uint8_t iKey = static_cast<uint8_t>(enID);
			if (mapEffectPathList_.find(iKey) == mapEffectPathList_.end())
				return nullptr;

			/* 同時再生数の上限を超える場合は再生しない。*/
			if (static_cast<int>(vecPlayingEffects_.size()) >= kMaxEffectCount)
				return nullptr;

			/* エミッタを生成して再生する。*/
			EffectEmitter* pEmitter = NewGO<EffectEmitter>(0, "effect");
			pEmitter->Init(iKey);
			pEmitter->SetPosition(vPosition);
			pEmitter->SetRotation(qRotation);
			pEmitter->SetScale(vScale);
			pEmitter->Play();

			/* 寿命を管理するため再生中の一覧へ登録する。*/
			EffectInfo stInfo;
			stInfo.pEmitter_ = pEmitter;
			stInfo.fLifeTime_ = fLifeTime;
			stInfo.fCurrentTime_ = 0.0f;
			vecPlayingEffects_.emplace_back(stInfo);

			return pEmitter;
		}


		void EffectList::StopEffect(EffectEmitter* pEffect)
		{
			/* 無効な指定なら何もしない。*/
			if (pEffect == nullptr)
				return;

			/* 一覧から探して破棄する。*/
			for (auto iterator = vecPlayingEffects_.begin(); iterator != vecPlayingEffects_.end(); ++iterator)
			{
				/* 目的のエミッタでなければ次へ。*/
				if (iterator->pEmitter_ != pEffect)
					continue;

				/* 見つかったので破棄して一覧から取り除く。*/
				DestroyEmitter(*iterator);
				vecPlayingEffects_.erase(iterator);
				return;
			}
		}
	}
}
