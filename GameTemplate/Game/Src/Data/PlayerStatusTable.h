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
			/* 視点を振ったときの銃の遅れ。*/
			float	fSwayGain_ = 28.0f;				//! 視点を1ラジアン振ったときに銃がずれる距離。
			float	fSwayMaxOffset_ = 10.0f;		//! 銃のずれの上限(これ以上は離れない)。
			float	fSwayRecoverRate_ = 9.0f;		//! ずれた銃が中央へ戻る速さ。
			float	fSwayRollRate_ = 0.010f;		//! 銃のずれ1単位あたりの傾き(ラジアン)。

			/* 銃の歩行ボブ。*/
			float	fBobWalkSpeed_ = 9.0f;			//! 歩きのボブの速さ。
			float	fBobWalkAmp_ = 2.0f;			//! 歩きで銃が横に振れる幅。
			float	fBobSprintSpeed_ = 13.0f;		//! 走りのボブの速さ。
			float	fBobSprintAmp_ = 4.0f;			//! 走りで銃が横に振れる幅。
			float	fBobPhaseLag_ = 0.55f;			//! 銃のボブをカメラより遅らせる位相。手が体に遅れてついてくる感じを出す。
			float	fBobWeaponUpRate_ = 0.6f;		//! 銃のボブの縦の振れ幅(横に対する比)。
			float	fBobWeightRate_ = 8.0f;			//! ボブの重みが移り変わる速さ(止まった瞬間にピタッと消さないため)。

			/* カメラ自体の揺れ。*/
			float	fViewBobHeightWalk_ = 1.6f;		//! 歩きでカメラが上下する量。
			float	fViewBobHeightSprint_ = 3.0f;	//! 走りでカメラが上下する量。
			float	fViewBobRollWalk_ = 0.010f;		//! 歩きでカメラが傾く角度(ラジアン)。
			float	fViewBobRollSprint_ = 0.020f;	//! 走りでカメラが傾く角度(ラジアン)。

			/* 横移動の傾きと、覗き込み中の抑制。*/
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
		 * @struct HealthRuleStatus
		 * @brief  L4D2式の体力ルールの調整値。
		 *         体力は「恒久HP」と、時間で減っていく「一時体力」の2階建て。
		 *         メディキットは恒久HPを、鎮痛剤・アドレナリンは一時体力を増やす。
		 */
		struct HealthRuleStatus
		{
			/* 一時体力。*/
			float	fTempHPDecayRate_ = 0.5f;		//! 一時体力が1秒あたりに減る量。
			/* メディキット。*/
			float	fMedkitUseTime_ = 3.0f;			//! 使い切るまでに押し続ける時間(秒)。途中で動くと最初からになる。
			float	fMedkitHealRate_ = 0.8f;		//! 失った恒久HPのうち回復する割合(L4D2と同じ80%)。
			/* 鎮痛剤とアドレナリン(どちらか1つだけ持てる)。*/
			int		iPillsTempHP_ = 50;				//! 鎮痛剤で増える一時体力。
			int		iAdrenalineTempHP_ = 25;		//! アドレナリンで増える一時体力。
			float	fAdrenalineTime_ = 15.0f;		//! アドレナリンの効果時間(秒)。
			float	fAdrenalineSpeedRate_ = 1.3f;	//! 効果中の移動速度の倍率。負傷歩行も無視する。
			float	fAdrenalineActionRate_ = 1.5f;	//! 効果中のリロード・回復の速さの倍率。
			/* 負傷歩行。合計HP(恒久＋一時)で判定する。*/
			int		iLimpHP_ = 40;					//! 合計HPがこれ未満で足を引きずる。
			float	fLimpSpeedRate_ = 0.68f;		//! 足を引きずるときの移動速度の倍率。
			int		iCriticalHP_ = 10;				//! 合計HPがこれ以下で瀕死。さらに遅くなる。
			float	fCriticalSpeedRate_ = 0.4f;		//! 瀕死のときの移動速度の倍率。
			/* 白黒(サードストライク)と、ダウン中の扱い。*/
			int		iMaxReviveCount_ = 2;			//! メディキットを使わずに復帰できる回数。使い切ると白黒になり、次のダウンで死亡する。
			int		iReviveTempHP_ = 30;			//! 復帰したときに得る一時体力。
			float	fDownEyeHeight_ = 60.0f;		//! ダウン中の目の高さ。倒れているので低くなる。
			float	fDownDamageTimeRate_ = 0.15f;	//! ダウン中に受けたダメージ1あたりに縮む出血時間(秒)。殴られ続けると早く死ぬ。
			float	fEyeHeightFollowRate_ = 10.0f;	//! 目の高さが目標(立ち/ダウン)へ移る速さ。
		};

		/**
		 * @struct PlayerStatus
		 * @brief  プレイヤー(サバイバー)の調整用パラメータ。
		 *         ここに書いた値が Assets/data/player.json が無いときの既定値になる。
		 */
		struct PlayerStatus
		{
			/* 基本のステータス。*/
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

			/* ダウンと救助。*/
			float	fBleedOutTime_ = 15.0f;			//! ダウンしてから死亡するまでの出血時間(秒)。
			int		iReviveHP_ = 1;					//! 救助で復帰したときの恒久HP。残りは一時体力(health.reviveTempHP)で補う。

			/* 突き飛ばし(近接)。*/
			float	fShoveRange_ = 180.0f;			//! 突き飛ばしが届く距離。
			float	fShovePush_ = 120.0f;			//! 突き飛ばしで敵を押し返す距離。
			float	fShoveFrontDot_ = 0.5f;			//! 正面判定のしきい値(0.5=正面±60度)。
			float	fShoveCooldownTime_ = 0.7f;		//! 突き飛ばしのクールダウン(秒)。

			/* 開始時の所持品。*/
			int		iMedkitCount_ = 1;				//! 開始時に持っている回復アイテムの数。
			int		iGrenadeCount_ = 2;				//! 開始時に持っている投擲アイテムの数。
			std::string sStartQuickItem_ = "Pills";	//! 開始時に持っている即効アイテム("Pills" / "Adrenaline" / "None")。

			/* 体力のルール。*/
			HealthRuleStatus	stHealthRule_;		//! L4D2式の体力(一時体力・メディキット・負傷歩行・白黒)。

			/* 見た目の調整値。まとまりごとに別の構造体へ分けている。*/
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
