#include "stdafx.h"
#include "CharacterAnimBank.h"

namespace nsApp
{
	namespace
	{
		/** @def
		 * @brief アニメーション登録用のマクロ。
		 * @details ANIM_LIST::Idle, "Idle", true のように使う。
		 */
#define ANIM_ENTRY(Name, Loop) { ANIM_LIST::Name, #Name, Loop }

		/** @def
		 * @brief 登録値と違う場合に活用。
		 */
#define ANIM_ENTRY_AS(Name, Stem, Loop) { ANIM_LIST::Name, Stem, Loop }

		/* コモンゾンビ。 */
		const CharacterAnimEntry kCommonAnimEntries_[] =
		{
			/* true … 連続再生。　false … 一度再生 */
			ANIM_ENTRY(Idle, true),
			ANIM_ENTRY(Walk, true),	
			ANIM_ENTRY(Run, true),
			ANIM_ENTRY(Attack, true),
		};

		/* コモンゾンビ。*/
		const CharacterAnimBank kCommonAnimBank_ =
		{
			CharacterModelType::Common,
			"Assets/animData/zombie/",
			kCommonAnimEntries_,
			static_cast<int>(sizeof(kCommonAnimEntries_) / sizeof(kCommonAnimEntries_[0])),
		};

		/* 特殊ゾンビ（アニメ未用意) */
		const CharacterAnimBank kSpecialAnimBank_ =
		{
			CharacterModelType::Special,
			"Assets/animData/Infected/Special/",
			nullptr,
			0,
		};

		/*  第三種ゾンビ（アニメ未用意) */
		const CharacterAnimBank kThirdAnimBank_ =
		{
			CharacterModelType::Third,
			"Assets/animData/Infected/Third/",
			nullptr,
			0,
		};

		/*  操作キャラ（アニメ未用意) */
		const CharacterAnimBank kSurvivorAnimBank_ =
		{
			CharacterModelType::Survivor,
			"Assets/animData/Survivor/",
			nullptr,
			0,
		};

		/* すべてのアニメバンクを配列にまとめる。*/
		const CharacterAnimBank* kAllCharacterAnimBanks_[] =
		{
			&kCommonAnimBank_,
			&kSpecialAnimBank_,
			&kThirdAnimBank_,
			&kSurvivorAnimBank_,
		};

#undef ANIM_ENTRY
#undef ANIM_ENTRY_AS
	}


	const CharacterAnimBank* FindCharacterAnimBank(CharacterModelType characterType)
	{
		/* 初回だけ map を構築する。*/
		static const std::unordered_map<CharacterModelType, const CharacterAnimBank*> s_map = []
			{
				/* キャラクター種別からアニメバンクへのマップを作る。*/
				std::unordered_map<CharacterModelType, const CharacterAnimBank*> map;

				/* すべてのアニメバンクをマップに登録する。*/
				for (const CharacterAnimBank* pBank : kAllCharacterAnimBanks_)
					map[pBank->type] = pBank;

				/* マップを返す。*/
				return map;
			}();

		/* マップからキャラクター種別に対応するアニメバンクを探す。*/
		auto it = s_map.find(characterType);
		if (it == s_map.end())
			return nullptr;

		/* 見つかった場合はアニメバンクを返す。*/
		return it->second;
	}
}