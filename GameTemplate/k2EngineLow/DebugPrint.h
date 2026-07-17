#pragma once
#include <Windows.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace nsK2EngineLow {

    inline void DebugPrint(const char* format, ...)
    {
#ifdef _DEBUG
        if (!format) return;
        char buf[1024] = {};

        va_list args;
        va_start(args, format);

        // vsprintf_s で例外が起きることがあるので安全版
        int written = 0;
        __try {
            written = vsprintf_s(buf, format, args);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            strcpy_s(buf, "[DebugPrint] <format error or invalid arg>\n");
        }

        va_end(args);

        OutputDebugStringA(buf);
#endif
    }

    inline void DebugPrintW(const wchar_t* format, ...)
    {
#ifdef _DEBUG
        if (!format) return;
        wchar_t buf[1024] = {};

        va_list args;
        va_start(args, format);
        int written = 0;
        __try {
            written = vswprintf_s(buf, format, args);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            wcscpy_s(buf, L"[DebugPrintW] <format error or invalid arg>\n");
        }
        va_end(args);

        OutputDebugStringW(buf);
#endif
    }

} // namespace nsK2EngineLow
