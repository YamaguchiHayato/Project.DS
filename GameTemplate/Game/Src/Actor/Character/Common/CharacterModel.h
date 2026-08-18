#pragma once

namespace nsApp
{
	/**
	 * @enum CharacterModelType
	 * @brief キャラクターモデルの種別。
	 */
	enum class CharacterModelType
	{
		Infected,			//! 雑魚ゾンビ。
		Survivor,			//! サバイバー。
	};

	/**
	 * @file   CharacterModel.h
      * @brief  キャラクターモデル/アニメーションを管理するクラス。
	 * @author Yamaguchi Hayato。
	 * @date   2026/08/18
      */
	class CharacterModel
	{
	public:
		/* コンストラクタとデストラクタ。*/
		CharacterModel();
		virtual ~CharacterModel() = default;


	public:
		/**
		 * @brief キャラクターモデルをロードする。
		 * @param characterType キャラクターモデルの種別。
		 * @param pAnimationClip アニメーションクリップのポインタ。
		 * @param iNumClips アニメーションクリップの数。
		 * @return ロードに成功した場合はtrue、失敗した場合はfalse。
		 */
		bool LoadCharacterModel(CharacterModelType characterType, AnimationClip* pAnimationClip, int iNumClips);

		/**
		 * @brief キャラクターモデルのアニメーションを再生する。
		 * @param iAnimationNumber 再生するアニメーションの番号。
		 * @param fInterpolateTime アニメーションの補間時間。
		 */
		inline void PlayAnimation(int iAnimationNumber, float fInterpolateTime)
		{
			/* キャラモデルが確認できない場合、処理を終了させる。*/
			if (pCharacterModelRender_ != nullptr)
			{
				pCharacterModelRender_->PlayAnimation(iAnimationNumber, fInterpolateTime);
			}
		}

		/**
		 * @brief キャラクターモデルを描画する。
		 * @param rc レンダリングコンテキスト。
		 */
		void DrawCharacterModel(RenderContext& rc);

		/* 更新処理。*/
		void Update();

		/* アニメーションが再生終了したか。*/
		bool IsPlayAnimation()
		{
			/* nullチェック。*/
			if (pCharacterModelRender_ != nullptr)
				/* アニメーションが再生終了しているかを返す。*/
				return pCharacterModelRender_->IsPlayingAnimation();

			return false;
		}

	
	/* セッター。*/
	public:
		/**
		 * @brief キャラクターモデルの位置を設定する。
		 * @param position キャラクターモデルの位置。
		 */
		inline void SetPosition(const Vector3& position)
		{
			if (pCharacterModelRender_)
				pCharacterModelRender_->SetPosition(position);
		}

		/**
		 * @brief キャラクターモデルの回転を設定する。
		 * @param rotation キャラクターモデルの回転。
		 */
		inline void SettRotation(const Quaternion& rotation)
		{
			/* nullチェック。*/
			if (pCharacterModelRender_)
				pCharacterModelRender_->SetRotation(rotation);
		}

		/**
		 * @brief キャラクターモデルのスケールを設定する。
		 * @param scale キャラクターモデルのスケール。
		 */
		inline void SetCharacterScale(const Vector3& scale)
		{
			/* nullチェック。*/
			if (pCharacterModelRender_)
				pCharacterModelRender_->SetScale(scale);
		}


	/* ゲッター。*/
	public:
		/**
		 * @brief キャラクターモデルのファイルパスを取得する。
		 * @param sFilePath キャラクターモデルのファイルパス。
		 * @return キャラクターモデルのファイルパス。
		 */
		inline const std::string GetCharacterModelFilePath(std::string sFilePath)
		{
			std::string sModelPath = sCharacterModelFilePath_ + sFilePath + sModelExtension_;
			return sModelPath;
		}


	private:
		std::unordered_map<CharacterModelType, std::string> mapFilePathList_; //! モデルIDからファイルパスを文字列に変化。
		std::unique_ptr<ModelRender> pCharacterModelRender_; //! モデルを管理。
		std::string sModelFilePath_; //! モデルのファイルパスを格納。
		std::string sCharacterModelFilePath_ = "Assets/modelData/Character/CharacterModel/"; //! プレイヤー/NPCモデルのファイルパスを格納。
		std::string sModelExtension_ = ".tkm"; //! プレイヤー/NPCモデルの拡張子を格納。
	};
}