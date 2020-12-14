#include "pch.h"
#include "Context.h"
#include "Writer.h"


CContext::CContext()
	: m_hSurface(nullptr)
	, m_pDevice(nullptr)
	, m_pDeviceContext(nullptr)
	, m_bRecording( false )
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

void CContext::start(UINT32 fps)
{
	LOG(__FUNCTION__);

	if (m_bRecording) {
		return;
	}

	const UINT32 VIDEO_WIDTH = 640;
	const UINT32 VIDEO_HEIGHT = 480;
	const UINT64 VIDEO_FRAME_DURATION = 10 * 1000 * 1000 / fps;
	const UINT32 VIDEO_FRAME_COUNT = 20 * fps;  // 20sec 30fps

	CWriter writer(VIDEO_WIDTH, VIDEO_HEIGHT, fps);

	const UINT64 uVideoFrameDurationMillis = 1000 / fps;
	const UINT64 uVideoFrameDuration100Nanos = uVideoFrameDurationMillis * 10 * 1000;

	m_bRecording = true;

	// Send frames to the sink writer.
	ULONGLONG uLastFrameStartPos = 0;
	std::chrono::high_resolution_clock::time_point lastFrame = std::chrono::high_resolution_clock::now();
	while (m_bRecording) {
		UINT64 uDurationSinceLastFrame100Nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - lastFrame).count() / 100;
		if (uDurationSinceLastFrame100Nanos < uVideoFrameDuration100Nanos) {
			// double delay = (double)(uVideoFrameDuration100Nanos - uDurationSinceLastFrame100Nanos) / 10 / 1000;
			continue;
		}

		HRESULT hr = writer.writeFrame(uLastFrameStartPos);
		if (FAILED(hr)) {
			break;
		}

		lastFrame = std::chrono::high_resolution_clock::now();
		uLastFrameStartPos += uDurationSinceLastFrame100Nanos;
	}
}
