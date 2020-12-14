#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma comment(lib, "d3d11.lib")
#include <d3d11.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

#include <iostream>
#include <future>

#define LOG(format, ...) _log(format "\n", __VA_ARGS__)
#define ERR(format, ...) LOG("Error: " format, __VA_ARGS__)

inline void _log(const char* format, ...)
{
	char buffer[256];
	va_list args;
	va_start(args, format);
	vsprintf_s(buffer, 256, format, args);
	OutputDebugStringA(buffer);
	va_end(args);
}

template <class T> void SafeRelease(T** ppT)
{
	if (*ppT) {
		(*ppT)->Release();
		*ppT = NULL;
	}
}
