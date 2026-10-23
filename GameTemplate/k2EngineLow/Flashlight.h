#pragma once
#include "Light.h"

namespace nsK2EngineLow
{
	/**
	 * @brief 手持ちライト。カメラの位置から視線方向へ照らす。
	 *        RenderingEngine が持ち、更新も RenderingEngine が行う。
	 *        どこからでも g_renderingEngine->GetFlashlight() で操作できる。
	 */
	class Flashlight
	{
	public:
		/**
		 * @brief 毎フレーム呼ぶ(RenderingEngine::Execute から)。キー入力と、ライトの更新を行う。
		 * @param light 書き込み先のシーンライト。
		 */
		void Update(SceneLight& light);

		void Toggle() { bIsOn_ = !bIsOn_; }
		void SetOn(bool bOn) { bIsOn_ = bOn; }
		bool IsOn() const { return bIsOn_; }

		/** @brief 装備しているか。falseの間は、キーも効かず、必ず消灯する。 */
		void SetEquipped(bool bEquipped) { bIsEquipped_ = bEquipped; }
		bool IsEquipped() const { return bIsEquipped_; }

		/* 見た目の調整用。*/
		void SetColor(const Vector3& vColor) { vColor_ = vColor; }
		void SetRange(float fRange) { fRange_ = fRange; }
		void SetHalfAngleDeg(float fDeg) { fHalfAngleDeg_ = fDeg; }

	private:
		bool bIsEquipped_ = true;		//! 初期装備として持っている。
		bool bIsOn_ = false;			//! 点灯中か(初期は消灯)。
		bool bWasPressKey_ = false;		//! 前フレームのキー状態。
		Vector3 vColor_ = { 4.0f, 3.8f, 3.2f };	//! 光の色(やや暖色)。
		float fRange_ = 2000.0f;		//! 届く距離。
		float fHalfAngleDeg_ = 28.0f;	//! 光の広がり(半角、度)。
	};
}