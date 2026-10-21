#include "pch.h"
#include "InputObserver.h"
#include <fstream>

namespace
{
	const char* TSV_FILE = "Assets/data/keyConfig.tsv"; //! 入力割り当てのTSVファイル。
}

namespace nsApp
{
	namespace nsData
	{
		const ActionName InputObserver::actionName_[InputObserver::actionNameCount_] =
		{
			{ "moveForward", EnAction::MoveForward },
			{ "moveBack", EnAction::MoveBack },
			{ "moveLeft", EnAction::MoveLeft },
			{ "moveRight", EnAction::MoveRight },
			{ "fire", EnAction::Fire },
			{ "ads", EnAction::Ads },
			{ "weaponNext", EnAction::WeaponNext },
			{ "weaponPrev", EnAction::WeaponPrev },
			{ "reload", EnAction::Reload },
			{ "interact", EnAction::Interact },
			{ "light", EnAction::Light },
			{ "pause", EnAction::Pause },
			{ "heal", EnAction::Heal },
			{ "throw", EnAction::Throw },
			{ "quickItem", EnAction::QuickItem },
			{ "sprint", EnAction::Sprint },
			{ "crouch", EnAction::Crouch },
			{ "shove", EnAction::Shove },
			{ "mainWeapon", EnAction::MainWeapon },
			{ "subWeapon", EnAction::SubWeapon },
			{ "confirm", EnAction::Confirm },
			{ "viewMode", EnAction::ViewMode },
			{ "startGame", EnAction::StartGame },
			{ "openDebug", EnAction::OpenDebug },
		};

		InputObserver InputObserver::instance_; 


		void InputObserver::BeginFrame()
		{
			/* 起動後の最初のフレームだけ、割り当てを読む。*/
			if (!isLoaded_)
				Load();

			/* 各操作の押下状態を更新する。*/
			for (index_ = 0; index_ < actionCount_; index_++)
			{
				/* 主と予備のどちらかが押されていれば、その操作は押下中。*/
				press_[index_] = primary_[index_].IsPress() || secondary_[index_].IsPress();

				/* 前回は離れていて、今回押されたフレームだけトリガー。*/
				trigger_[index_] = press_[index_] && !prevPress_[index_];
				prevPress_[index_] = press_[index_];
			}
		}


		void InputObserver::Load()
		{
			/* ファイルが無くても遊べるように、先に既定値を入れる。*/
			FillDefault();
			isLoaded_ = true;

			/* TSVを読み込む。無いときは既定値のまま。*/
			if (!table_.Load(TSV_FILE))
				return;

			/* TSVの列が無いときは既定値のまま。*/
			for (row_ = 0; row_ < table_.GetRowCount(); row_++)
			{
				/* TSVの行から操作名を読み、対応する EnAction を得る。*/	
				if (!ToAction(table_.GetString(row_, "action"), loadedAction_))
					continue;

				/* 名前が変換できた列だけ上書きする。空欄は既定値のまま。*/
				if (KeyBinding::FromName(table_.GetString(row_, "binding"), loaded_))
					primary_[static_cast<int>(loadedAction_)] = loaded_;

				if (KeyBinding::FromName(table_.GetString(row_, "binding2"), loaded_))
					secondary_[static_cast<int>(loadedAction_)] = loaded_;
			}
		}


		bool InputObserver::Save()
		{
			std::ofstream tsvFile(TSV_FILE);

			/* ファイルを開く。*/
			if (!tsvFile.is_open())
				return false;

			tsvFile << "action\tbinding\tbinding2\n";


			for (index_ = 0; index_ < actionNameCount_; index_++)
			{
				/* 操作名と割り当てをTSVに書き込む。*/
				const int actionIndex = static_cast<int>(actionName_[index_].action);

				tsvFile << actionName_[index_].pName << "\t"
					<< primary_[actionIndex].ToName() << "\t"
					<< secondary_[actionIndex].ToName() << "\n";
			}

			return true;
		}


		void InputObserver::FillDefault()
		{
			SetDefault(EnAction::MoveForward, "W");
			SetDefault(EnAction::MoveBack, "S");
			SetDefault(EnAction::MoveLeft, "A");
			SetDefault(EnAction::MoveRight, "D");
			SetDefault(EnAction::Fire, "MouseLeft");
			SetDefault(EnAction::Ads, "MouseRight");
			SetDefault(EnAction::WeaponNext, "WheelUp");
			SetDefault(EnAction::WeaponPrev, "WheelDown");
			SetDefault(EnAction::Reload, "R");
			SetDefault(EnAction::Interact, "E");
			SetDefault(EnAction::Light, "F");
			SetDefault(EnAction::Pause, "Escape");
			SetDefault(EnAction::Heal, "H", "4");
			SetDefault(EnAction::Throw, "G", "3");
			SetDefault(EnAction::QuickItem, "5");
			SetDefault(EnAction::Sprint, "Shift");
			SetDefault(EnAction::Crouch, "Control");
			SetDefault(EnAction::Shove, "V");
			SetDefault(EnAction::MainWeapon, "1");
			SetDefault(EnAction::SubWeapon, "2");
			SetDefault(EnAction::Confirm, "Return");
			SetDefault(EnAction::ViewMode, "T");
			SetDefault(EnAction::StartGame, "1");
			SetDefault(EnAction::OpenDebug, "2");
		}


		void InputObserver::SetDefault(EnAction action, const char* pPrimary, const char* pSecondary)
		{
			/* 配列の添字に変換する。*/
			index_ = static_cast<int>(action);
			KeyBinding::FromName(pPrimary, primary_[index_]);
			KeyBinding::FromName(pSecondary, secondary_[index_]);
		}


		bool InputObserver::ToAction(const std::string& name, EnAction& outAction)
		{
			/* 操作名の表を走査して、対応する EnAction を探す。*/
			for (index_ = 0; index_ < actionNameCount_; index_++)
			{
				/* 名前が一致したら、対応する EnAction を返す。*/
				if (name == actionName_[index_].pName)
				{
					/* 見つかった操作名の EnAction を返す。*/
					outAction = actionName_[index_].action;
					return true;
				}
			}

			return false;
		}
	}
}