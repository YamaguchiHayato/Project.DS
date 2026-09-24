#pragma once

#include "CharacterModel.h"

using namespace std;

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

#define ANIM_LIST CharacterBasicAnimationList

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
		 * @param characterType キャラクターの種別。
		 */
		void Initialize(CharacterModelType characterType);

		/**
		 * @brief アニメーションをロードする。
		 */
		void LoadAnimation();

		/**
		 * @brief 基本アニメのファイルパスを登録する。
		 * @param state アニメーションの種類。
		 * @param pFileStem 拡張子なしのファイル名。
		 * @param bIsLoop ループ再生するか。
		 */
		void RegisterAnimation(ANIM_LIST state, const char* pFileStem, bool bIsLoop);


		/* ゲッター。*/
	public:
		/**
		 * @brief 基本動作用アニメーションのインデックスを取得する。
		 * @param state アニメーションの種類。
		 * @return 基本動作用アニメーションのインデックス。
		 */
		inline int GetBasicAnimationIndex(ANIM_LIST state)
		{
			return mapBasicIndexMap_.count(state) ? mapBasicIndexMap_[state] : 0;
		}

		/**
		 * @brief 基本動作用アニメーションのファイルパスを取得する。
		 * @param sFileStem 拡張子なしのファイル名。
		 * @return 基本動作用アニメーションのファイルパス。
		 */
		inline const string GetBasicAnimationFilePath(const string& sFileStem)
		{
			/* 拡張子 .tka は固定。*/
			const string sBasicAnimation = sBasicAnimationFilePath_ + sFileStem + ".tka";
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
		inline int SetAnimationClip(const string& sFilePath, bool bIsLoop)
		{
			/* アニメーションをロード。*/
			pAnimationClipList_[iCurrentIndex_].Load(sFilePath.c_str());
			/* アニメーションをループするか設定。*/
			pAnimationClipList_[iCurrentIndex_].SetLoopFlag(bIsLoop);
			/* 配列を加算する。*/
			return iCurrentIndex_++;
		}


	private:
		std::unordered_map<ANIM_LIST, std::string> mapBasicAnimationFilePathList_; //! 基本動作用アニメーションのファイルパスを管理するマップ。
		std::unordered_map<ANIM_LIST, bool> mapBasicLoopFlagList_; //! 基本動作用アニメーションのループ設定。
		std::unordered_map<ANIM_LIST, int> mapBasicIndexMap_; //! 基本動作用アニメーシを管理するマップ。
		std::unique_ptr<AnimationClip[]> pAnimationClipList_; //! 読み込んだアニメーションを管理する配列。
		std::string sBasicAnimationFilePath_; //! 初期化時に CharacterAnimBank からセットする animData フォルダ。
		int iCurrentIndex_ = 0; //! 現在のアニメーションの再生数を管理。
		int iAnimationNum_ = 0;	//! 読み込んだアニメーションの数を管理する変数。
	};
}