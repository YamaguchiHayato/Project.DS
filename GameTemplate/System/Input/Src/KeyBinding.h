#pragma once
#include <string>

namespace nsApp
{
	namespace nsData
	{
		/**
		 * @file   KeyBinding.h
		 * @brief  操作1つに割り当てた入力。
		 * @author Yamaguchi Hayato
		 * @date   2026/10/21
		 */
		enum class EnInputDevice
		{
			None, 
			Keyboard,
			MouseButton,
			WheelUp,
			WheelDown,
		};
		
		/**
		 * @struct KeyBinding
		 * @brief 操作1つに割り当てた入力。
		 * @author Yamaguchi Hayato
		 * @date 2026/10/20
		 */
		struct KeyBinding
		{
			EnInputDevice device_ = EnInputDevice::None; ///! 入力デバイス。
			int code_ = 0; ///! 入力コード。

			/**
			 * @brief このフレームで押されているか。
			 * @details ホイールは回転したフレームだけ true。
			 * @return true: 押されている, false: 押されていない
			 */
			bool IsPress() const;
			
			/**
			 * @brief 名前からKeyBindingを生成する。
			 * @param name 入力名
			 * @param outBinding 出力するKeyBinding
			 */
			static bool FromName(const std::string& name, KeyBinding& outBinding);

			/**
			 * @brief 割りてた入力をTSVに上書きできるように文字列にする。
			 * @return 文字列化した入力。 未設定の場合は空文字が入ります。
			 */
			std::string ToName() const;

			/**
			 * @brief ユーザーが入力したキーを取得する。
			 * @param outBinding 取得した入力を格納する変数。
			 * @return 入力が取得できたら true。キャンセルされた場合は false。
			 */
			static bool Capture(KeyBinding& outBinding);
		};

		/**
		 * @struct BindingName
		 * @brief 入力名と入力コードの対応表。
		 */
		struct BindingName
		{
			EnInputDevice bindingDevice; //! 入力の種類。
			int bindingCode; //! 入力コード。
			const char* pName; //! 入力名。
		};

		/**
		 * @brief 入力名と入力コードの対応表。
		 * @details ここに追加したものは、KeyBinding::FromName() と KeyBinding::ToName() に反映されます。
		 */
		const BindingName bindingName[] =
		{
			{ EnInputDevice::MouseButton, 0,"MouseLeft" },
			{ EnInputDevice::MouseButton, 1, "MouseRight" },
			{ EnInputDevice::MouseButton, 2, "MouseMiddle" },
			{ EnInputDevice::WheelUp, 0, "WheelUp" },
			{ EnInputDevice::WheelDown, 0, "WheelDown" },
			{ EnInputDevice::Keyboard, 0x10,"Shift" },
			{ EnInputDevice::Keyboard, 0x11, "Control" },
			{ EnInputDevice::Keyboard, 0x1B, "Escape" },
			{ EnInputDevice::Keyboard,  0x0D, "Return" },
			{ EnInputDevice::Keyboard, 0x20, "Space" },
			{ EnInputDevice::Keyboard, 0x09, "Tab" },
		};
	}
}
