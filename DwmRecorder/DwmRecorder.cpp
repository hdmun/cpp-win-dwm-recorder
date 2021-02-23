#include "pch.h"
#include "framework.h"
#include "DwmRecorder.h"
#include "Recorder.h"

#if _DEBUG
bool isLoggingEnabled = true;
LogLevel logSeverityLevel = LogLevel::Debug;
#else
bool isLoggingEnabled = false;
LogLevel logSeverityLevel = LogLevel::Info;
#endif

std::wstring logFilePath;

extern "C"{
namespace dwmrecorder
{

typedef BOOL(WINAPI* PFDwmGetDxSharedSurface)(
	HANDLE hHandle,
	HANDLE* phSurface,
	LUID* pAdapterLuid,
	ULONG* pFmtWindow,
	ULONG* pPresentFlags,
	ULONGLONG* pWin32kUpdateId
	);


PFDwmGetDxSharedSurface DwmGetDxSharedSurface = nullptr;
CRecorder g_ctx;
std::future<void> g_record;

DWMRECORDER_API bool __stdcall initialize()
{
	HMODULE hUser32Dll = LoadLibraryA("user32.dll");
	if (hUser32Dll == nullptr) {
		ERROR_LOG(L"failed to load library `user32.dll`");
		return false;
	}

	DwmGetDxSharedSurface = (PFDwmGetDxSharedSurface)GetProcAddress(hUser32Dll, "DwmGetDxSharedSurface");
	if (DwmGetDxSharedSurface == nullptr) {
		ERROR_LOG(L"failed to get address `DwmGetDxSharedSurface`, DLL: 0x%lx", hUser32Dll);
		return false;
	}

	return g_ctx.initialize();
}

DWMRECORDER_API void __stdcall finalize()
{
	g_ctx.finalize();
}

DWMRECORDER_API void __stdcall start(HWND hWnd, Config config)
{
	HANDLE hSurface = nullptr;
	LUID adapterLuid = { 0, };
	ULONG pFmtWindow = 0;
	ULONG pPresentFlags = 0;
	ULONGLONG pWin32kUpdateId = 0;
	BOOL bSuccess = DwmGetDxSharedSurface(hWnd, &hSurface, &adapterLuid, &pFmtWindow, &pPresentFlags, &pWin32kUpdateId);
	if (!bSuccess || !hSurface) {
		ERROR_LOG(L"failed to call `DwmGetDxSharedSurface`, HWND: 0x%lx, ret: %d, hSurface: 0x%lx", hWnd, bSuccess, hSurface);
		return;
	}

	g_record = std::async(std::launch::async, &CRecorder::start, &g_ctx, hSurface, config);
}

DWMRECORDER_API void __stdcall stop()
{
	g_ctx.stop();
	if (g_record.valid()) {
		g_record.wait();
	}
}

}
}