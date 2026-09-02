#pragma once
#include <vector>
#include "Src/Actor/Character/Common/CharacterModel.h"

namespace nsApp
{
	namespace nsCombat
	{
		/**
		 * @enum  EnHitPart
		 * @brief 弾が当たった体の部位。ダメージ倍率と演出の出し分けに使う。
		 */
		enum class EnHitPart : uint8_t
		{
			Head,	//! 頭。弱点。
			Body,	//! 胴と腕。
			Leg,	//! 脚。
			/* ↑部位を増やしたら、HitBoxSet.cpp の部位名一覧にも1件追加する。*/
			Num,	//! 部位の数。
			None,	//! どこにも当たっていない。
		};

		/**
		 * @struct HitPartShape
		 * @brief  部位1つぶんの当たり判定の形。
		 *         人型を「縦に積んだ円柱」で近似する。高さは足元(y=0)からの値で持つので、
		 *         キャラクターがどちらを向いていても同じ判定になる。
		 *         TODO: 腕を左右で分けるなど向きに依存する部位を足すときは、
		 *               対象キャラクターから向き(ヨー角)を受け取れるようにしてから追加する。
		 */
		struct HitPartShape
		{
			EnHitPart enPart_ = EnHitPart::Body;	//! この形が表す部位。
			float fBottom_ = 0.0f;					//! 足元から測った下端の高さ。
			float fTop_ = 0.0f;						//! 足元から測った上端の高さ。
			float fRadius_ = 0.0f;					//! 水平方向の被弾半径。
		};

		/**
		 * @struct HitResult
		 * @brief  当たり判定の結果。
		 */
		struct HitResult
		{
			bool bIsHit_ = false;						//! 当たったか。
			EnHitPart enPart_ = EnHitPart::None;		//! 当たった部位。
			float fDistance_ = 0.0f;					//! 判定の起点から命中点までの距離。
			Vector3 vHitPoint_ = Vector3::Zero;			//! 命中点。
			float fDamageRate_ = 1.0f;					//! 当たった部位のダメージ倍率。
		};

		/**
		 * @file   HitBoxSet.h
		 * @brief  キャラクター1体ぶんの部位別当たり判定。
		 *         人型を縦に積んだ円柱の集まりで表し、レイ(弾)がどの部位に当たったかを返す。
		 *         形と部位ごとのダメージ倍率は Assets/data/hitbox.json から読み込むので、
		 *         ヘッドショット倍率などの調整にリビルドは要らない。
		 * @author Izumida Kiryu
		 * @date   2026/09/02
		 */
		class HitBoxSet
		{
		public:
			/* コンストラクタとデストラクタ。*/
			HitBoxSet() = default;
			virtual ~HitBoxSet() = default;


		public:
			/**
			 * @brief レイ(弾)がこのキャラクターのどの部位に当たったかを調べる。
			 *        当たった部位が複数あれば、一番手前のものを返す。
			 * @param vRayStart     判定の起点(目の位置)。
			 * @param vRayDirection 弾の進む向き(正規化済み)。
			 * @param fMaxDistance  調べる最大距離(射程)。
			 * @param vFootPosition 対象キャラクターの足元の座標。
			 * @param stOutResult   結果を受け取る。
			 * @return 当たっていたらtrue。
			 */
			bool RayTest(const Vector3& vRayStart, const Vector3& vRayDirection, float fMaxDistance, const Vector3& vFootPosition, HitResult& stOutResult) const;

			/**
			 * @brief 部位ごとのダメージ倍率を取得する。
			 * @param enPart 部位。
			 * @return ダメージ倍率。
			 */
			float GetDamageRate(EnHitPart enPart) const;

			/**
			 * @brief 一番上の部位の高さ(=だいたいの身長)を取得する。
			 *        撃破エフェクトを胸のあたりに出すなど、見た目の位置合わせに使う。
			 * @return 身長。部位が1つも無ければ0。
			 */
			float GetHeight() const;


		public:
			/**
			 * @brief キャラクター種別ごとの当たり判定を取得する。初回の呼び出しでJSONを読み込む。
			 * @param enModelType キャラクターの種別。
			 * @return その種別の当たり判定。
			 */
			static const HitBoxSet& GetShared(CharacterModelType enModelType);

			/**
			 * @brief JSONを読み直す(既定値へ戻してから上書きし直す)。
			 */
			static void Load();

			/**
			 * @brief 部位の名前を取得する(デバッグ表示・JSONのキー用)。
			 * @param enPart 部位。
			 * @return 部位の名前。
			 */
			static const char* GetPartName(EnHitPart enPart);


		public:
			/**
			 * @brief 部位の形を追加する(既定値の組み立てとJSONの読み込みから呼ぶ)。
			 * @param stShape 追加する形。
			 */
			void AddPartShape(const HitPartShape& stShape);

			/**
			 * @brief 部位のダメージ倍率を設定する。
			 * @param enPart 部位。
			 * @param fRate  ダメージ倍率。
			 */
			void SetDamageRate(EnHitPart enPart, float fRate);

			/**
			 * @brief 形とダメージ倍率を空にする。
			 */
			void Clear();


		private:
			std::vector<HitPartShape> vecPartShapeList_;	//! 部位ごとの当たり判定の形。
			//! 部位ごとのダメージ倍率。要素数ぶん明示的に1.0で初期化する。
			float aDamageRateList_[static_cast<int>(EnHitPart::Num)] = { 1.0f, 1.0f, 1.0f };
		};
	}
}
