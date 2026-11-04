#include "stdafx.h"
#include "OptionEntry.h"
#include <iterator>

namespace
{
	using nsApp::nsData::SettingCategory;
	using nsApp::nsData::CameraItem;
	using nsApp::nsData::ControlItem;
	using nsApp::nsData::VideoItem;
	using nsApp::nsData::SoundItem;
	using nsApp::nsUI::OptionTab;
	using nsApp::nsUI::OptionRowType;
	using nsApp::nsUI::OptionCommand;

	/* 選択肢の表示名。並びは設定値の番号と同じ。 */
	const wchar_t* const INVERT_CHOICES[] = { L"通常", L"反転" };
	const wchar_t* const ADS_CHOICES[] = { L"押している間", L"切り替え" };
	const wchar_t* const ON_OFF_CHOICES[] = { L"OFF", L"ON" };
	const wchar_t* const WINDOW_CHOICES[] = { L"1280 x 720", L"1600 x 900", L"1920 x 1080" };
	const wchar_t* const FRAME_RATE_CHOICES[] = { L"30", L"60", L"無制限" };

	/**
	 * @brief 各カテゴリの列挙を、行の項目番号に変える。
	 * @param item 項目。
	 * @return 項目番号。
	 */
	template<typename ITEM>
	constexpr unsigned char ToItem(ITEM item)
	{
		return static_cast<unsigned char>(item);
	}
}

namespace nsApp
{
	namespace nsUI
	{
		/* タブ, 出し方, カテゴリ, 項目, 数字の倍率, 表示名, 説明, 選択肢。 */
		const OptionEntry OPTION_ENTRIES[] =
		{
			/* カメラ。 */
			{ OptionTab::Camera, OptionRowType::Choice, SettingCategory::Camera, ToItem(CameraItem::InvertY), 1.0f,
				L"上下反転", L"カメラを上下に動かす向きを設定します。", INVERT_CHOICES },
			{ OptionTab::Camera, OptionRowType::Choice, SettingCategory::Camera, ToItem(CameraItem::InvertX), 1.0f,
				L"左右反転", L"カメラを左右に動かす向きを設定します。", INVERT_CHOICES },
			{ OptionTab::Camera, OptionRowType::Gauge, SettingCategory::Camera, ToItem(CameraItem::Fov), 1.0f,
				L"視野角", L"画面に映る範囲の広さを設定します。\n数値が大きいほど広く見えます。", nullptr },

			/* 操作。 */
			{ OptionTab::Control, OptionRowType::Gauge, SettingCategory::Control, ToItem(ControlItem::Sensitivity), 10.0f,
				L"マウス感度", L"マウスでのカメラ操作の感度を設定します。\n数値が大きいほど速く振り向きます。", nullptr },
			{ OptionTab::Control, OptionRowType::Gauge, SettingCategory::Control, ToItem(ControlItem::AdsSensitivity), 10.0f,
				L"覗き込み感度", L"銃を構えて覗き込んでいるときの感度を設定します。", nullptr },
			{ OptionTab::Control, OptionRowType::Choice, SettingCategory::Control, ToItem(ControlItem::AdsToggle), 1.0f,
				L"覗き込み操作", L"右クリックを押している間だけ覗き込むか、\n押すたびに切り替えるかを設定します。", ADS_CHOICES },
			{ OptionTab::Control, OptionRowType::Command, SettingCategory::Control, ToItem(OptionCommand::KeyConfig), 1.0f,
				L"キー設定", L"キーの割り当てを変更します。", nullptr },

			/* グラフィックス。 */
			{ OptionTab::Video, OptionRowType::Gauge, SettingCategory::Video, ToItem(VideoItem::Brightness), 10.0f,
				L"画面の明るさ", L"画面の明るさを設定します。\n暗い場所が見えすぎない程度がおすすめです。", nullptr },
			{ OptionTab::Video, OptionRowType::Choice, SettingCategory::Video, ToItem(VideoItem::WindowSize), 1.0f,
				L"ウィンドウサイズ", L"ウィンドウの大きさを設定します。\n次回起動時に反映されます。", WINDOW_CHOICES },
			{ OptionTab::Video, OptionRowType::Choice, SettingCategory::Video, ToItem(VideoItem::MotionBlur), 1.0f,
				L"モーションブラー", L"視点を動かしたときの残像を設定します。\n酔いやすい場合は OFF にしてください。", ON_OFF_CHOICES },
			{ OptionTab::Video, OptionRowType::Choice, SettingCategory::Video, ToItem(VideoItem::FrameRate), 1.0f,
				L"フレームレート上限", L"1秒あたりの描画回数の上限を設定します。", FRAME_RATE_CHOICES },

			/* サウンド。 */
			{ OptionTab::Sound, OptionRowType::Choice, SettingCategory::Sound, ToItem(SoundItem::Mute), 1.0f,
				L"ミュート", L"ゲーム全体の音を消します。", ON_OFF_CHOICES },
			{ OptionTab::Sound, OptionRowType::Gauge, SettingCategory::Sound, ToItem(SoundItem::Master), 20.0f,
				L"マスター音量", L"ゲーム全体の音量を設定します。", nullptr },
			{ OptionTab::Sound, OptionRowType::Gauge, SettingCategory::Sound, ToItem(SoundItem::Bgm), 20.0f,
				L"BGM 音量", L"音楽の音量を設定します。", nullptr },
			{ OptionTab::Sound, OptionRowType::Gauge, SettingCategory::Sound, ToItem(SoundItem::Se), 20.0f,
				L"SE 音量", L"足音や銃声などの SE の音量を設定します。", nullptr },
		};

		/* OPTION_ENTRIES の行数。 */
		const int OPTION_ENTRY_COUNT = static_cast<int>(std::size(OPTION_ENTRIES));

		/* タブの表示名。並びは OptionTab の列挙と同じ。 */
		const wchar_t* const OPTION_TAB_LABELS[] = { L"カメラ", L"操作", L"グラフィックス", L"サウンド" };

		/* タブの列挙と表示名の数がずれたらビルドエラーにする。 */
		static_assert(std::size(OPTION_TAB_LABELS) == static_cast<size_t>(OptionTab::Count), "OPTION_TAB_LABELS と OptionTab の数が違う");
	}
}