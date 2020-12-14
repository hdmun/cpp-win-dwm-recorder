#include "pch.h"
#include "Context.h"

CContext::CContext()
	: m_hSurface(nullptr)
	, m_pDevice(nullptr)
	, m_pDeviceContext(nullptr)
{
	{
		HRESULT hr = ::CoInitializeEx(nullptr, COINITBASE_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr)) {
			return;
		}
	}
	{
		HRESULT hr = ::MFStartup(MF_VERSION, MFSTARTUP_LITE);
		if (FAILED(hr)) {
			return;
		}
	}
}

CContext::~CContext()
{
	::MFShutdown();
	::CoUninitialize();
}

bool CContext::initialize(HANDLE hSurface)
{
	LOG(__FUNCTION__);

	m_hSurface = hSurface;

	D3D_FEATURE_LEVEL pFeatureLevel;
	UINT Flags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_SINGLETHREADED;
	HRESULT hr = ::D3D11CreateDevice(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, Flags, nullptr, 0, D3D11_SDK_VERSION,
		&m_pDevice, &pFeatureLevel, &m_pDeviceContext
	);
	if (FAILED(hr)) {
		return false;
	}

	return true;
}

void CContext::finalize()
{
	LOG(__FUNCTION__);

	m_hSurface = nullptr;
	if (m_pDeviceContext) {
		m_pDeviceContext->Release();
		m_pDeviceContext = nullptr;
	}
	if (m_pDevice) {
		m_pDevice->Release();
		m_pDevice = nullptr;
	}
}
