#pragma once
#include <unordered_map>
#include <vector>
#include <string>

namespace nsApp
{
	namespace nsEffect
	{
		/**
		 * @file   EffectList.h
		 * @brief  エフェクトの登録・再生・寿命管理をまとめて扱うリストクラス。
		 *         識別子(EnEffectID)で再生を要求し、寿命が切れたエミッタはこのクラスが破棄する。
		 *         EffectEmitter は自動で消えないため、再生中の一覧を持って面倒を見る。
		 * @author Izumida Kiryu
		 * @date   2026/08/21
		 */

		/**
		 * @enum  EnEffectID
		 * @brief エフェクトの識別子。Init() の登録と対応させること。
		 */
		enum class EnEffectID : uint8_t
		{
			MuzzleFlash,	//! 発射時の銃口の閃光。
			Hit,			//! 弾が命中した瞬間。
			Explosion,		//! グレネードの爆発。
			Heal,			//! 回復アイテムの使用。
		};

		/**
		 * @struct EffectInfo
		 * @brief  再生中のエフェクト1件ぶんの情報。
		 */
		struct EffectInfo
		{
			EffectEmitter* pEmitter_ = nullptr;	//! エフェクトのエミッタ。
			float fLifeTime_ = 0.0f;			//! 寿命(秒)。
			float fCurrentTime_ = 0.0f;			//! 経過時間(秒)。
		};

		class EffectList
		{
		public:
			/* コンストラクタとデストラクタ。*/
			EffectList() = default;
			virtual ~EffectList();


		public:
			/**
			 * @brief エフェクト素材をエンジンへ登録する。シーン開始時に一度だけ呼ぶ。
			 */
			void Init();

			/**
			 * @brief 再生中のエフェクトの寿命を進め、切れたものを破棄する。毎フレーム呼ぶ。
			 * @param fDeltaTime 1フレームの経過時間(秒)。
			 */
			void Update(float fDeltaTime);

			/**
			 * @brief 再生中のエフェクトを全て破棄する。
			 */
			void Clear();

			/**
			 * @brief 指定したエフェクトを停止して破棄する。
			 * @param pEffect 停止するエミッタ。
			 */
			void StopEffect(EffectEmitter* pEffect);

			/**
			 * @brief エフェクトを再生する。
			 * @param enID エフェクトの識別子。
			 * @param vPosition 出現位置。
			 * @param qRotation 回転。
			 * @param vScale 拡大率。
			 * @param fLifeTime 維持する時間(秒)。
			 * @return 再生したエミッタ。失敗時は nullptr。
			 */
			EffectEmitter* PlayEffect(EnEffectID enID, const Vector3& vPosition, const Quaternion& qRotation = Quaternion::Identity, const Vector3& vScale = Vector3::One, float fLifeTime = 2.0f);


		public:
			/**
			 * @brief 現在有効なエフェクトリストを設定する(所有者であるシーンが自身を登録する)。
			 * @param pList 有効にするリスト。破棄時は nullptr を渡す。
			 */
			static void SetActiveList(EffectList* pList)
			{
				/* どこからでも再生できるよう、有効なリストを覚えておく。*/
				pActiveList_ = pList;
			}

			/**
			 * @brief 現在有効なエフェクトリストを取得する。
			 * @return 有効なリスト。無ければ nullptr。
			 */
			static EffectList* GetActiveList()
			{
				return pActiveList_;
			}


		private:
			/**
			 * @brief 再生中の一覧から1件ぶんのエミッタを破棄する。
			 * @param stInfo 破棄する情報。
			 */
			void DestroyEmitter(EffectInfo& stInfo);

			/**
			 * @brief エフェクトファイルのパスを組み立てて登録する。
			 *        素材が見つからない識別子は登録せず、再生要求を無視する。
			 * @param enID エフェクトの識別子。
			 * @param sFileName 拡張子を含むファイル名(例: "hit.efk")。
			 */
			void RegisterEffect(EnEffectID enID, const std::u16string& sFileName);


		private:
			std::unordered_map<uint8_t, std::u16string> mapEffectPathList_;	//! 識別子とファイルパスの対応表。
			std::vector<EffectInfo> vecPlayingEffects_;						//! 再生中のエフェクト一覧。
			bool bInitialized_ = false;										//! 登録済みか。

			static EffectList* pActiveList_;								//! 現在有効なリスト(どこからでも再生するための窓口)。
		};

		/**
		 * @brief エフェクトを再生する簡易窓口。有効なリストが無ければ何もしない。
		 * @param enID エフェクトの識別子。
		 * @param vPosition 出現位置。
		 * @param qRotation 回転。
		 * @param vScale 拡大率。
		 * @param fLifeTime 維持する時間(秒)。
		 */
		void PlayEffect(EnEffectID enID, const Vector3& vPosition, const Quaternion& qRotation = Quaternion::Identity, const Vector3& vScale = Vector3::One, float fLifeTime = 2.0f);
	}
}
