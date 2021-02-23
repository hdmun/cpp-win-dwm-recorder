#pragma once

extern "C" {

namespace dwmrecorder
{

#ifndef DWMRECORDER_API
struct Config
{
	UINT32 usFps{ 30 };
	UINT32 usVideoRate{ 8 * 1 * 1024 * 1024 };  // 1MB
	UINT32 usVideoQuality{ 70 };
};

#ifdef DWMRECORDER_DLL
#ifdef DLL_EXPORTS
#define DWMRECORDER_API __declspec(dllexport)
#else
#define DWMRECORDER_API __declspec(dllimport)
#endif

#else
#define DWMRECORDER_API
#endif  // #ifdef DWMRECORDER_DLL

#endif  // #ifndef DWMRECORDER_API


DWMRECORDER_API bool __stdcall initialize();
DWMRECORDER_API void __stdcall finalize();

DWMRECORDER_API void __stdcall start(HWND hWnd, Config config);
DWMRECORDER_API void __stdcall stop();

}

}