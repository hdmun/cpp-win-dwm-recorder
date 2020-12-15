#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma comment(lib, "d3d11.lib")
#include <d3d11.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#pragma comment(lib, "mf")
#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

#include <codecapi.h>
#include <strmif.h>

template <class T> void SafeRelease(T** ppT)
{
	if (*ppT) {
		(*ppT)->Release();
		*ppT = NULL;
	}
}
