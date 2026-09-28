#pragma once

#include "CharacterModel.h"
#include "CharacterAnimation.h"

namespace nsApp
{
	/**
	 * @struct CharacterAnimEntry
	 * @brief 1本ぶんのアニメ登録情報。
	 */
	struct CharacterAnimEntry
	{
		ANIM_LIST state;		//! 基本アクション種別。
		const char* pFileStem;	//! 拡張子なしのファイル名（例: "Idle"）。
		bool bIsLoop;			//! ループ再生するか。
	};

	/**
	 * @struct CharacterAnimBank
	 * @brief キャラ種別ごとのアニメーション一式（バンク）。
	 */
	struct CharacterAnimBank
	{
		CharacterModelType type;			//! キャラクターの種別。
		const char* pBasePath;				//! animData フォルダ（末尾 / 付き）。
		const CharacterAnimEntry* pEntries;	//! 登録配列。
		int iEntryCount;					//! 配列数。
	};

	/**
	 * @brief キャラ種別 ID からアニメバンクを取得する。
	 * @param characterType キャラクターの種別。
	 * @return バンクが無い場合は nullptr。
	 */
	const CharacterAnimBank* FindCharacterAnimBank(CharacterModelType characterType);
}