#pragma once

namespace nsApp
{
	/**
	 * @enum CharacterBasicAnimationList
	 * @brief キャラクターの基本動作用アニメーションを管理する列挙型。
	 */
	enum class CharacterBasicAnimationList : uint8_t
	{
		Idle,			//! 待機。
		Walk,			//! 歩き。
		Run,			//! 走り。
		Attack,			//! 攻撃。
		Death,			//! 死亡。
	};

	/**
 　　 * @file   CharacterAnimation.h
	 * @brief  キャラクターのアニメーションを管理するクラス。
	 * @author Yamaguchi Hayato。
	 * @date   2026/06/02: 最終更新日
	 */
	class CharacterAnimation
	{
	public:
		/* コンストラクタとデストラクタ。*/
		CharacterAnimation() = default;
		virtual ~CharacterAnimation() = default;


	public:
		/**
		 * @brief アニメーションを初期化する。
		 */
		void Initialize();

		/**
		 * @brief アニメーションをロードする。
		 */
		void LoadAnimation();


	/* ゲッター。*/
	public:
		/**
		 * @brief 基本動作用アニメーションのインデックスを取得する。
		 * @param state アニメーションの種類。
		 * @return 基本動作用アニメーションのインデックス。
		 */
		inline int GetBasicAnimationIndex(CharacterBasicAnimationList state)
		{
			return mapBasicIndexMap_.count(state) ? mapBasicIndexMap_[state] : 0;
		}

		/**
		 * @brief 基本動作用アニメーションのファイルパスを取得する。
		 * @param sFilePath ファイルパス。
		 * @return 基本動作用アニメーションのファイルパス。
		 */
		inline const std::string GetBasicAnimationFilePath(const std::string sFilePath)
		{
			const std::string sBasicAnimation = sBasicAnimationFilePath_ + sFilePath + sAnimationExtension_;
			return sBasicAnimation;
		}

		/**
		 * @brief アニメーションクリップを取得する。
		 * @return アニメーションクリップ。
		 */
		inline AnimationClip* GetAnimatiocClip()
		{
			return pAnimationClipList_.get();
		}

		/**
		 * @brief 読み込んだアニメーションの数を取得する。
		 * @return 読み込んだアニメーションの数。
		 */
		inline int GetAnimationClips() const
		{
			return iAnimationNum_;
		}


	/* セッター。*/
	public:
		/**
		 * @brief アニメーションクリップを設定する。	
		 * @param sFilePath ファイルパス。
		 * @param bIsLoop ループするかどうか。
		 * @return　設定したアニメーションクリップのインデックス。
		 */
		inline int SetAnimationClip(const std::string sFilePath, bool bIsLoop)
		{
			/* アニメーションをロード。*/
			pAnimationClipList_[iCurrentIndex_].Load(sFilePath.c_str());
			/* アニメーションをループするか設定。*/
			pAnimationClipList_[iCurrentIndex_].SetLoopFlag(bIsLoop);
			/* 配列を加算する。*/
			return iCurrentIndex_++;
		}


	private:
		std::unordered_map<CharacterBasicAnimationList, std::string> mapBasicAnimationFilePathList_; //! 基本動作用アニメーションのファイルパスを管理するマップ。

		/* 読み込んだアニメーションの要素数を代入する変数。*/
		std::unordered_map<CharacterBasicAnimationList, int> mapBasicIndexMap_; //! 基本動作用アニメーシを管理するマップ。
		std::unique_ptr<AnimationClip[]> pAnimationClipList_; //! 読み込んだアニメーションを管理する配列。

		/* ファイルパスを定数化するための変数群。*/
		const std::string sBasicAnimationFilePath_ = "Assets/animData/Infected/"; //! 基本動作用アニメーションのファイルパスの共通部分。
		const std::string sAnimationExtension_ = ".tka"; //! アニメーションファイルの拡張子。

		int iCurrentIndex_ = 0; //! 現在のアニメーションの再生数を管理。
		int iAnimationNum_ = 0;	//! 読み込んだアニメーションの数を管理する変数。
		bool bIsLoop_ = false;	//! アニメーションをループするか管理する変数。
	};
}