#pragma once
#include "Src/KeyBinding.h"
#include "util/Noncopyable.h"
#include "tsv/TSVTable.h"

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @enum EnAction
		 * @brief ゲーム内の操作の種類。
		 * @details 各操作には、キーボードやマウスの入力が関連付けられる。
		 * @author Yamaguchi Hayato
		 * @date 2026/10/21
		 * @todo 要素が増えた場合は列挙を更新すること。
		 */
		enum class EnAction
		{
			MoveForward,	//! 前進。
			MoveBack,		//! 後退。
			MoveLeft,		//! 左移動。
			MoveRight,		//! 右移動。
			Fire,			//! 射撃。
			Ads,			//! 覗き込み。
			WeaponNext,		//! 次の武器。
			WeaponPrev,		//! 前の武器。
			Reload,			//! リロード。
			Interact,		//! インタラクト。
			Light,			//! ライト。
			Pause,			//! ポーズ。
			Heal,			//! 回復。
			Throw,			//! 投擲。
			QuickItem,		//! 即効アイテム。
			Sprint,			//! スプリント。
			Crouch,			//! しゃがみ。
			Shove,			//! 突き飛ばし。
			MainWeapon,		//! メイン武器。
			SubWeapon,		//! サブ武器。
			Confirm,		//! 決定。
			ViewMode,		//! 視点切替。
			StartGame,		//! タイトルから本編へ。
			OpenDebug,		//! タイトルからデバッグへ。
			Count,			//! 操作の数。配列の大きさに使う。
		};


		/**
		 * @struct ActionName
		 * @brief TSVの操作名と EnAction の対応。
		 */
		struct ActionName
		{
			const char* pName; //! TSVに書く操作名。
			EnAction action; //! 対応する操作。
		};


		/**
		 * @brief ゲーム全体の入力。操作が押されたかだけを返す。
		 *        割り当ては Assets/data/keyConfig.tsv。無いときは既定のキーボードとマウス。
		 *        実体は Get() が返す1個だけ。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/21
		 */
		class InputObserver
		{
		public:
			/**
			 * @brief 入力管理の実体を返す。
			 * @return プログラム内で1個だけの InputObserver。
			 */
			static InputObserver& Get()
			{
				static InputObserver instance;
				return instance_;
			}

			/**
			 * @brief 1フレームに1回、エンジンの BeginFrame の直後で呼ぶ。
			 *        初回は割り当てを読み、その後に押下と押した瞬間を確定する。
			 */
			void BeginFrame();

			/**
			 * @brief 現在の割り当てをTSVファイルに保存する。
			 * @return 書き込めたら true。
			 */
			bool Save();

			/**
			 * @brief その操作が押しっぱなしなら true。
			 * @param action 調べる操作。
			 * @return 押されていれば true。
			 */
			inline bool IsPress(EnAction action) const
			{
				return press_[static_cast<int>(action)];
			}

			/**
			 * @brief その操作を、このフレームで押した瞬間なら true。
			 * @param action 調べる操作。
			 * @return 押した瞬間なら true。
			 */
			inline bool IsTrigger(EnAction action) const
			{
				return trigger_[static_cast<int>(action)];
			}


		private:
			/* コンストラクタ。*/
			InputObserver() = default;


		private:
			/**
			 * @brief 割り当てを TSV から読み込む。
			 * @details TSVが無いときは既定の割り当てを入れる。
			 */
			void Load();

			/**
			 * @brief 既定の割り当てを入れる。
			 * @details TSVが無いときはこれだけで動く。
			 */
			void FillDefault();

			/**
			 * @brief 1つの操作へ、主割り当てと予備割り当てを入れる。
			 * @param action 対象の操作。
			 * @param pPrimary 主割り当ての名前。
			 * @param pSecondary 予備割り当ての名前。不要なら空文字。
			 */
			void SetDefault(EnAction action, const char* pPrimary, const char* pSecondary = "");

			/**
			 * @brief TSVに書いた操作名を EnAction へ変換する。
			 * @param name 操作名。
			 * @param outAction 変換結果。
			 * @return 知っている名前なら true。
			 */
			bool ToAction(const std::string& name, EnAction& outAction);


		private:
			static constexpr int actionCount_ = static_cast<int>(EnAction::Count); //! 操作の数。
			static constexpr int actionNameCount_ = 24; //! 操作名の表の行数。
			static const ActionName actionName_[actionNameCount_]; //! 操作名と列挙の対応表。
			static InputObserver instance_; //! プログラム内の実体。
			nsK2EngineLow::nsSystem::nsTSVFile::TSVTable table_; //! 読み込み中のTSV。
			KeyBinding primary_[actionCount_]; //! 各操作の主割り当て。
			KeyBinding secondary_[actionCount_]; //! 各操作の予備割り当て。
			KeyBinding loaded_; //! TSVから変換した割り当ての受け取り。
			EnAction loadedAction_ = EnAction::MoveForward; //! 変換中の操作。
			int index_ = 0; //! 操作配列を走査する位置。
			int row_ = 0; //! TSVの行。
			bool press_[actionCount_] = {}; //! 今フレームで押されているか。
			bool prevPress_[actionCount_] = {}; //! 前フレームで押されていたか。
			bool trigger_[actionCount_] = {}; //! 今フレームで押した瞬間か。
			bool isLoaded_ = false; //! 割り当てを読み終えたか。
		};
	}
}