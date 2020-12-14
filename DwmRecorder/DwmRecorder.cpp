#include "pch.h"
#include "framework.h"
#include "DwmRecorder.h"
#include "Context.h"
#include "Writer.h"

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
CContext g_ctx;

DWMRECORDER_API bool __stdcall initialize(HWND hWnd)
{
	LOG("%s, HWND: 0x%x", __FUNCTION__, hWnd);

	HMODULE hUser32Dll = LoadLibraryA("user32.dll");
	if (hUser32Dll == nullptr) {
		return false;
	}

	DwmGetDxSharedSurface = (PFDwmGetDxSharedSurface)GetProcAddress(hUser32Dll, "DwmGetDxSharedSurface");
	if (DwmGetDxSharedSurface == nullptr) {
		return false;
	}

	HANDLE hSurface = nullptr;
	LUID adapterLuid = { 0, };
	ULONG pFmtWindow = 0;
	ULONG pPresentFlags = 0;
	ULONGLONG pWin32kUpdateId = 0;
	BOOL bSuccess = DwmGetDxSharedSurface(hWnd, &hSurface, &adapterLuid, &pFmtWindow, &pPresentFlags, &pWin32kUpdateId);
	if (!bSuccess || !hSurface) {
		return false;
	}

	return g_ctx.initialize(hSurface);
}

DWMRECORDER_API void __stdcall finalize()
{
	LOG(__FUNCTION__);

	g_ctx.finalize();
}

DWMRECORDER_API void __stdcall start()
{
	const UINT32 VIDEO_WIDTH = 640;
	const UINT32 VIDEO_HEIGHT = 480;
	const UINT32 VIDEO_FPS = 30;
	const UINT64 VIDEO_FRAME_DURATION = 10 * 1000 * 1000 / VIDEO_FPS;
	const UINT32 VIDEO_FRAME_COUNT = 20 * VIDEO_FPS;  // 20sec 30fps

	CWriter writer(VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_FPS);

	// Send frames to the sink writer.
	LONGLONG rtStart = 0;
	for (DWORD i = 0; i < VIDEO_FRAME_COUNT; ++i)
	{
		HRESULT hr = writer.writeFrame(rtStart);
		if (FAILED(hr))
		{
			break;
		}
		rtStart += VIDEO_FRAME_DURATION;
	}
}

DWMRECORDER_API void __stdcall stop()
{
	return DWMRECORDER_API void();
}

}
}