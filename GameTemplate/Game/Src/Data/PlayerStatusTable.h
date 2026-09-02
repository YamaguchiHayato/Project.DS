#pragma once
#include <string>

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @struct ViewShakeStatus
		 * @brief  歩きと視点移動で生まれる揺れの調整値。
		 *         「歩いている感じ」を作るところなので、触りながら詰められるよう外へ出してある。
		 */
		struct ViewShakeStatus
		{
			float	fSwayGain_ = 28.0f;				//! 視点を1ラジアン振ったときに銃がずれる距離。
			float	fSwayMaxOffset_ = 10.0f;		//! 銃のずれの上限(これ以上は離れない)。
			float	fSwayRecoverRate_ = 9.0f;		//! ずれた銃が中央へ戻る速さ。
			float	fSwayRollRate_ = 0.010f;		//! 銃のずれ1単位あたりの傾き(ラジアン)。

			float	fBobWalkSpeed_ = 9.0f;			//! 歩きのボブの速さ。
			float	fBobWalkAmp_ = 2.0f;			//! 歩きで銃が横に振れる幅。
			float	fBobSprintSpeed_ = 13.0f;		//! 走りのボブの速さ。
			float	fBobSprintAmp_ = 4.0f;			//! 走りで銃が横に振れる幅。
			float	fBobPhaseLag_ = 0.55f;			//! 銃のボブをカメラより遅らせる位相。手が体に遅れてついてくる感じを出す。
			float	fBobWeaponUpRate_ = 0.6f;		//! 銃のボブの縦の振れ幅(横に対する比)。
			float	fBobWeightRate_ = 8.0f;			//! ボブの重みが移り変わる速さ(止まった瞬間にピタッと消さないため)。

			float	fViewBobHeightWalk_ = 1.6f;		//! 歩きでカメラが上下する量。
			float	fViewBobHeightSprint_ = 3.0f;	//! 走りでカメラが上下する量。
			float	fViewBobRollWalk_ = 0.010f;		//! 歩きでカメラが傾く角度(ラジアン)。
			float	fViewBobRollSprint_ = 0.020f;	//! 走りでカメラが傾く角度(ラジアン)。

			float	fStrafeRollAngle_ = 0.035f;		//! 横移動でカメラが傾く角度(ラジアン)。逆に感じたら符号を反転する。
			float	fStrafeFollowRate_ = 6.0f;		//! 横移動の傾きが追いつく速さ。
			float	fAdsSuppressRate_ = 0.7f;		//! 覗き込み中に揺れを抑える割合(1.0で完全に止まる)。
		};

		/**
		 * @struct ReloadMotionStatus
		 * @brief  リロード演出「マガジンを抜く→挿す→構え直す」の振れ幅。
		 *         段階の区切り(いつ抜き終わるか等)はコード側に持ち、ここでは動く量だけを扱う。
		 */
		struct ReloadMotionStatus
		{
			float	fLowerDown_ = 14.0f;		//! マガジンを抜くときに銃を下げる距離。
			float	fLowerAngle_ = 0.5f;		//! 同・銃口を下げる角度(ラジアン)。
			float	fPullBack_ = 14.0f;			//! 同・銃を手前へ引く距離。
			float	fPullRight_ = 10.0f;		//! 同・銃を画面中央へ寄せる距離。
			float	fRollAngle_ = 0.75f;		//! 同・差込口が見えるよう銃を傾ける角度(ラジアン)。
			float	fInsertUp_ = 9.0f;			//! マガジンを挿し込むときに突き上げる距離。
			float	fInsertAngle_ = 0.16f;		//! 同・銃口が上を向く角度(ラジアン)。
			float	fSettleUp_ = 4.0f;			//! 構え直したときに行き過ぎる距離(戻りの勢い)。
		};

		/**
		 * @struct PlayerStatus
		 * @brief  プレイヤー(サバイバー)の調整用パラメータ。
		 *         ここに書いた値が Assets/data/player.json が無いときの既定値になる。
		 */
		struct PlayerStatus
		{
			int		iMaxHP_ = 100;					//! 最大HP。
			float	fMoveSpeed_ = 200.0f;			//! 歩きの移動速度(1秒あたり)。
			float	fSprintRate_ = 1.6f;			//! スプリント中の移動速度の倍率。
			float	fEyeHeight_ = 160.0f;			//! 目(カメラ)の高さ。射撃の起点とカメラ位置に使う。
			float	fWeaponRange_ = 3000.0f;		//! ヒットスキャンの射程。
			float	fBodyModelScale_ = 2.3f;		//! 体モデルの表示スケール。unityChanは素で身長約76しかないので、世界の想定(175)へ合わせる。

			/*
			 * 三人称で銃を持たせるボーンの名前。
			 * ※unityChanは左右の名前が反転していて、"LeftHand" が見た目の右手になる。
			 *   モデルを差し替えたら実際に持たせて確かめること。
			 */
			std::string sHandBoneName_ = "Character1_LeftHand";	//! 銃を持たせるボーンの名前。

			float	fBleedOutTime_ = 15.0f;			//! ダウンしてから死亡するまでの出血時間(秒)。
			int		iReviveHP_ = 30;				//! 救助で復帰したときのHP。

			float	fShoveRange_ = 180.0f;			//! 突き飛ばしが届く距離。
			float	fShovePush_ = 120.0f;			//! 突き飛ばしで敵を押し返す距離。
			float	fShoveFrontDot_ = 0.5f;			//! 正面判定のしきい値(0.5=正面±60度)。
			float	fShoveCooldownTime_ = 0.7f;		//! 突き飛ばしのクールダウン(秒)。

			int		iMedkitCount_ = 1;				//! 開始時に持っている回復アイテムの数。
			int		iGrenadeCount_ = 2;				//! 開始時に持っている投擲アイテムの数。

			ViewShakeStatus		stViewShake_;		//! 歩きと視点移動の揺れ。
			ReloadMotionStatus	stReloadMotion_;	//! リロード演出の振れ幅。
		};

		/**
		 * @file   PlayerStatusTable.h
		 * @brief  プレイヤーステータス表。Assets/data/player.json から調整用の数値を読み込んで保持する。
		 *         Player はこの表から数値を引くだけなので、
		 *         HPや移動速度の調整はJSONを書き換えるだけで済み、リビルドが要らない。
		 *         JSONが無い・項目が足りない場合は PlayerStatus の既定値がそのまま使われる。
		 * @author Izumida Kiryu
		 * @date   2026/09/02
		 */
		class PlayerStatusTable
		{
		public:
			/* コンストラクタとデストラクタ。*/
			PlayerStatusTable() = default;
			virtual ~PlayerStatusTable() = default;


		public:
			/**
			 * @brief プレイヤーのパラメータを取得する。初回の呼び出しでJSONを読み込む。
			 * @return プレイヤーのパラメータ。
			 */
			static const PlayerStatus& Get();

			/**
			 * @brief JSONを読み直す(既定値へ戻してから上書きし直す)。
			 */
			static void Load();
		};
	}
}
