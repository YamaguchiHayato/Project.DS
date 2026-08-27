#pragma once

namespace nsApp
{
	/**
	 * @file   CharacterTransition.h
	 * @brief  ステート遷移の共通インターフェース。
	 * @author Yamaguchi Hayato
	 * @date   2026/08/26
	 */
	class CharacterTransition
	{
	public:
		/* コンストラクタとデストラクタ。*/
		CharacterTransition() = default;
		virtual ~CharacterTransition() = default;

		/**
		 * @brief 必要ならステートを切り替える。
		 * @return 切り替えたら true。
		 */
		virtual bool TryChangeState() = 0;
	};
}