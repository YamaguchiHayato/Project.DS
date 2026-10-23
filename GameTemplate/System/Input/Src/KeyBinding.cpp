#include "pch.h"
#include "KeyBinding.h"
#include <Windows.h>
#include <cmath>
#include <DirectXMath.h>
#include "util/Noncopyable.h"
#include "math/Math.h"
#include "math/Vector.h"
#include "HID/Mouse.h"
#include <cctype>

namespace nsApp
{
	namespace nsData
	{
#define GET_INPUT_STATE EnInputDevice


		bool KeyBinding::IsPress() const
		{
			/* デバイスが未設定の場合は押されていないとする。*/ 
			if (device_ == GET_INPUT_STATE::Keyboard)
				return (GetAsyncKeyState(code_) & 0x8000) != 0;

			/* マウスの情報を取得する。*/
			nsK2EngineLow::Mouse* pMouse = nsK2EngineLow::g_mouse;

			/* マウスが初期化されていない場合は押されていないとする。*/
			if (pMouse == nullptr)
				return false;

			/* デバイスがマウスボタンの場合は、マウスの状態を取得する。*/
			if(device_ == GET_INPUT_STATE::MouseButton)
				return pMouse->IsPress(static_cast<nsK2EngineLow::EnMouseButton>(code_));

			/* デバイスがホイールの場合は、ホイールの状態を取得する。*/
			// ホイール: アップ。
			if (device_ == GET_INPUT_STATE::WheelUp)
				return pMouse->GetWheelDelta() > 0;

			// ホイール: ダウン。
			if (device_ == GET_INPUT_STATE::WheelDown)
				return pMouse->GetWheelDelta() < 0;

			return false;			
		}


		bool KeyBinding::FromName(const std::string& name, KeyBinding& outBinding)
		{
			outBinding = KeyBinding();

			/* デバイスが未設定の場合は押されていないとする。*/
			if(name.empty())
				return false; 

			/* 左クリックの場合。*/
			if (name == "MouseLeft")
			{
				outBinding.device_ = GET_INPUT_STATE::MouseButton;
				outBinding.code_ = nsK2EngineLow::EnMouseButton::enMouseButtonLeft;
				return true;
			}

			/* 右クリックの場合。*/
			if (name == "MouseRight")
			{
				outBinding.device_ = GET_INPUT_STATE::MouseButton;
				outBinding.code_ = nsK2EngineLow::EnMouseButton::enMouseButtonRight;
				return true;
			}
			/* 中央クリックの場合。*/
			if (name == "MouseMiddle")
			{
				outBinding.device_ = GET_INPUT_STATE::MouseButton;
				outBinding.code_ = nsK2EngineLow::EnMouseButton::enMouseButtonMiddle;
				return true;
			}

			/* ホイールを上げた場合。*/
			if (name == "WheelUp")
			{
				outBinding.device_ = GET_INPUT_STATE::WheelUp;
				return true;
			}

			/* ホイールを下げた場合。*/
			if (name == "WheelDown")
			{
				outBinding.device_ = GET_INPUT_STATE::WheelDown;
				return true;
			}

			/* キーボードの場合。*/
			outBinding.device_ = GET_INPUT_STATE::Keyboard;

			/* shiftキーの場合。*/
			if (name == "Shift")
			{
				outBinding.code_ = VK_SHIFT; return true;
				return true;
			}

			/* Ctrlキーの場合。*/
			if (name == "Control")
			{
				outBinding.code_ = VK_CONTROL; return true;
				return true;
			}

			/* Escキー。*/
			if (name == "Escape")
			{
				outBinding.code_ = VK_ESCAPE; return true;
				return true;
			}

			/* Returnキー。*/
			if (name == "Return")
			{
				outBinding.code_ = VK_RETURN; return true;
				return true;
			}

			/* Spaceキー。*/
			if (name == "Space")
			{
				outBinding.code_ = VK_SPACE; return true;
				return true;;
			}

			/* Tabキー。*/
			if (name == "Tab")
			{
				outBinding.code_ = VK_TAB; return true;
				return true;
			}

			/* 1文字のキーの場合。*/
			if (name.size() == 1)
			{
				const unsigned char c = static_cast<unsigned char>(name[0]);
				if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
				{
					/* 大文字アルファベットまたは数字の場合は、そのままコードに変換する。*/
					outBinding.code_ = c;
					return true;
				}
			}
			outBinding = KeyBinding();


			return false;
		}


		std::string KeyBinding::ToName() const
		{
			/* デバイスが未設定の場合は空文字を返す。*/
			for (const BindingName& name : bindingName)
			{
				/* デバイスとコードが一致する場合は、対応する名前を返す。*/
				if (device_ == name.bindingDevice && code_ == name.bindingCode)
					return name.pName;
			}

			/* デバイスがキーボードで、コードが大文字アルファベットまたは数字の場合は、そのまま文字列に変換する。*/
			if(device_ == GET_INPUT_STATE::Keyboard && (std::isupper(code_) || std::isdigit(code_)))
				return std::string(1, static_cast<char>(code_));

			return "";
		}


		bool KeyBinding::Capture(KeyBinding& outBinding)
		{
			/* デバイスが未設定の場合は押されていないとする。*/
			for (const BindingName& name : bindingName)
			{
				/* デバイスとコードが一致する場合は、対応する名前を返す。*/
				outBinding.device_ = name.bindingDevice;
				outBinding.code_ = name.bindingCode;

				/* このフレームで押されているかを確認する。*/
				if (outBinding.IsPress())
					return true;
			}


			/* 文字を管理。*/
			for (int alphabet = 'A'; alphabet <= 'Z'; alphabet++)
			{
				/* 大文字アルファベットの場合は、GetAsyncKeyStateで押されているかを確認する。*/
				if ((GetAsyncKeyState(alphabet) & 0x8000) != 0)
				{
					/* 大文字アルファベットが押されている場合は、出力変数に設定する。*/
					outBinding.device_ = EnInputDevice::Keyboard;
					outBinding.code_ = alphabet;
					return true;
				}
			}

			
			/* 数字。*/
			for (int number = '0'; number <= '9'; number++)
			{
				/* 数字キーが押されているかを確認する。*/
				if ((GetAsyncKeyState(number) & 0x8000) != 0)
				{
					/* 数字キーが押されている場合は、出力変数に設定する。*/
					outBinding.device_ = EnInputDevice::Keyboard;
					outBinding.code_ = number;
					return true;
				}
			}

			/* どの入力も押されていない場合は、出力変数を未設定にする。*/
			outBinding = KeyBinding();
			return false;
		}
	}
}