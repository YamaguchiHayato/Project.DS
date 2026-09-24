#pragma once

namespace nsApp
{
	namespace nsSystem
	{
		/**
		 * @struct ModelBounds
		 * @brief  モデルの頂点から測ったローカル空間の箱(AABB)。
		 */
		struct ModelBounds
		{
			Vector3 vMin_ = Vector3::Zero;	//! 箱の最小の角。
			Vector3 vMax_ = Vector3::Zero;	//! 箱の最大の角。
			bool bIsValid_ = false;			//! 頂点が1つでも取れて測れたか。

			/**
			 * @brief 箱の中心を取得する。モデル原点のズレを打ち消すのに使う。
			 * @return 中心。
			 */
			inline Vector3 GetCenter() const
			{
				Vector3 vCenter = vMin_ + vMax_;
				vCenter *= 0.5f;
				return vCenter;
			}

			/**
			 * @brief 箱の各辺の長さを取得する。
			 * @return 各辺の長さ。
			 */
			inline Vector3 GetSize() const
			{
				return vMax_ - vMin_;
			}

			/**
			 * @brief 一番長い辺を取得する。モデルの上向き軸に依存せず「大きさ」として使える。
			 * @return 一番長い辺。測れていなければ0。
			 */
			float GetLongestEdge() const;
		};

		/**
		 * @file   ModelBounds.h
		 * @brief  モデルの頂点を走査して実際の大きさ(ローカルAABB)を測る。
		 *         「実寸で置きたい」「モデルごとの原点ズレを吸収したい」ときに、
		 *         想定の大きさではなく本当の表示サイズを基準にするために使う。
		 *         読み込み時に一度だけ呼ぶこと(全頂点を舐めるので毎フレーム呼ぶものではない)。
		 * @author Izumida Kiryu
		 * @date   2026/09/12
		 */
		/**
		 * @brief モデルの全メッシュの全頂点からローカルAABBを測る。
		 * @param model 測るモデル(読み込み済み)。
		 * @return 測った箱。頂点が取れなければ bIsValid_ が false。
		 */
		ModelBounds MeasureModelBounds(Model& model);
	}
}
