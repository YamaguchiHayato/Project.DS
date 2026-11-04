#include "stdafx.h"
#include "SettingUI.h"
#include "GraphicsSetting.h"


namespace
{
	const char* FRAME_PATH = "Assets/image/config/configFrame.DDS";
	const wchar_t* ITEM_LABEL[] = { L"画角", L"マウス感度", L"明るさ", L"ウィンドウサイズ", L"操作設定" };
	const unsigned int WINDOW_WIDTH[] = { 1280, 1600, 1920 };
	const unsigned int WINDOW_HEIGHT[] = { 720, 900, 1080 };
	const int WINDOW_COUNT = 3;
	const float ITEM_X = -690.0f;
	const float ITEM_TOP_Y = 300.0f;
	const float ITEM_STEP_Y = -110.0f;
	const float ITEM_WIDTH = 380.0f;
	const float ITEM_HEIGHT = 78.0f;
	const float CONTENT_X = 260.0f;
	const float CONTENT_Y = 0.0f;
	const float CONTENT_WIDTH = 1080.0f;
	const float CONTENT_HEIGHT = 860.0f;
	const float LABEL_OFFSET_X = 0.0f;
	const float LABEL_OFFSET_Y = 0.0f;
	const float VALUE_OFFSET_X = 0.0f;
	const float VALUE_OFFSET_Y = 0.0f;
	const Vector2 PIVOT_CENTER = { 0.5f, 0.5f };
	const Vector2 FONT_PIVOT = { 0.5f, 0.5f };
	const Vector4 SELECTED_COLOR = Vector4::White;
	const Vector4 IDLE_COLOR = Vector4::Black;
	const Vector4 SELECTED_FONT_COLOR = Vector4::Black;
	const Vector4 IDLE_FONT_COLOR = Vector4::White;
}


namespace nsApp
{
	namespace nsUI
	{

	}
}