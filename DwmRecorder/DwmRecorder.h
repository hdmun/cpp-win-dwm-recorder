#pragma once

extern "C" {

namespace dwmrecorder
{

struct Config
{
	LPCWSTR wsFileName{ nullptr };
	UINT32 usFps{ 30 };
	UINT32 usVideoRate{ 8 * 1 * 1024 * 1024 };  // 1MB
	UINT32 usVideoQuality{ 70 };
};

#ifdef _DLL_EXPORTS
#define DWMRECORDER_API __declspec(dllexport)
#else
#define DWMRECORDER_API __declspec(dllimport)
#endif

#ifdef _DLL_EXPORTS
DWMRECORDER_API bool __stdcall initialize();
DWMRECORDER_API void __stdcall finalize();

DWMRECORDER_API void __stdcall start(HWND hWnd, const Config& config);
DWMRECORDER_API void __stdcall stop();

#else

typedef bool (*_initialize)();
typedef bool (*_finalize)();
typedef bool (*_start)(HWND, const Config&);
typedef bool (*_stop)();

__declspec(selectany) _initialize pfnInitialize = nullptr;
__declspec(selectany) _finalize pfnFinalize = nullptr;
__declspec(selectany) _start pfnStart = nullptr;
__declspec(selectany) _stop pfnStop = nullptr;

inline bool initializeModule()
{
	HMODULE hModule = ::LoadLibraryA("DwmRecorder.dll");
	if (!hModule) {
		return false;
	}

	pfnInitialize = (_initialize)(GetProcAddress(hModule, "initialize"));
	if (!pfnInitialize) {
		return false;
	}

	pfnFinalize = (_finalize)(GetProcAddress(hModule, "finalize"));
	if (!pfnFinalize) {
		return false;
	}

	pfnStart = (_start)(GetProcAddress(hModule, "start"));
	if (!pfnStart) {
		return false;
	}

	pfnStop = (_stop)(GetProcAddress(hModule, "stop"));
	if (!pfnStop) {
		return false;
	}

	return true;
}

inline bool initialize()
{
	if (pfnInitialize) {
		return pfnInitialize();
	}
	return false;
}

inline void finalize()
{
	if (pfnFinalize) {
		pfnFinalize();
	}
}

inline void start(HWND hWnd, Config config)
{
	if (pfnStart) {
		pfnStart(hWnd, config);
	}
}

inline void stop()
{
	if (pfnStop) {
		pfnStop();
	}
}
#endif

}

}