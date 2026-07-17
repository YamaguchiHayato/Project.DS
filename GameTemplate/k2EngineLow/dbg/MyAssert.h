#pragma once
#include <stdarg.h>
#include <cstdio>
#include <Windows.h>

namespace nsK2EngineLow {

    /*!
    *@brief	アサート
    *@param[in]	flag	偽のときアサートが発生します。
    */
    static inline void MyAssert(bool flag, const char* format, const char* file, long line, ...)
    {
        if (!flag) {
            char msg[1024];

            va_list args;
            va_start(args, line);
            vsprintf_s(msg, format, args);
            va_end(args);

            char fileLine[256];
            sprintf_s(fileLine, "\n\nFile: %s\nLine: %ld", file, line);
            strcat_s(msg, fileLine);

            MessageBoxA(nullptr, msg, "K2Engine Assert", MB_OK | MB_ICONERROR);
            std::abort();
        }
    }

} // namespace nsK2EngineLow

#ifdef K2_DEBUG
#define K2_ASSERT(flg, format, ...) nsK2EngineLow::MyAssert(flg, format, __FILE__, __LINE__, __VA_ARGS__)
#else
#define K2_ASSERT(flg, format, ...)
#endif
