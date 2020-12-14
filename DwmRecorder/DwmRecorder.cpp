#include "pch.h"
#include "framework.h"
#include "DwmRecorder.h"

extern "C"{
namespace dwmrecorder
{

DWMRECORDER_API bool __stdcall initialize(HWND hWnd)
{
	LOG("%s, HWND: 0x%x", __FUNCTION__, hWnd);
	return true;
}

DWMRECORDER_API void __stdcall finalize()
{
	LOG(__FUNCTION__);
}

}
}