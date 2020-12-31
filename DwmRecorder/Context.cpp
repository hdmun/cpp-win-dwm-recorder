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

bool CContext::initialize()
{
	D3D_FEATURE_LEVEL pFeatureLevel;
	UINT Flags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_SINGLETHREADED;
	HRESULT hr = ::D3D11CreateDevice(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, Flags, nullptr, 0, D3D11_SDK_VERSION,
		&m_pDevice, &pFeatureLevel, &m_pDeviceContext
	);
	if (FAILED(hr)) {
		ERROR_LOG(L"failed to `D3D11CreateDevice`, hr: 0x%lx", hr);
		return false;
	}

	return true;
}

void CContext::finalize()
{
	INFO_LOG(L"finalize CContext");

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

void CContext::start(HANDLE hSurface, UINT32 fps)
{
	if (m_bRecording) {
		INFO_LOG(L"already recording");
		return;
	}

	initializeSurfaceHandle(hSurface);

	CWriter writer(m_desc.Width, m_desc.Height, fps);

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

		HRESULT hr = writer.writeFrame(uLastFrameStartPos, uDurationSinceLastFrame100Nanos, this);
		if (FAILED(hr)) {
			break;
		}

		lastFrame = std::chrono::high_resolution_clock::now();
		uLastFrameStartPos += uDurationSinceLastFrame100Nanos;
	}
}

IMFMediaBuffer* CContext::CreateMediaBuffer(UINT32 width, UINT32 height) const
{
	ID3D11Texture2D* pTexture = GetSurfaceTexture();
	if (!pTexture) {
		return nullptr;
	}

	D3D11_TEXTURE2D_DESC desc = { 0, };
	pTexture->GetDesc(&desc);
	if (desc.Width != width || desc.Height != height) {
		SafeRelease(&pTexture);
		return nullptr;
	}

	const LONG cbWidth = 4 * width;
	const DWORD cbBuffer = cbWidth * height;

	IMFMediaBuffer* pBuffer = nullptr;
	HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);
	if (FAILED(hr)) {
		SafeRelease(&pTexture);
		SafeRelease(&pBuffer);
		return nullptr;
	}

	BYTE* pData = NULL;
	// Lock the buffer and copy the video frame to the buffer.
	hr = pBuffer->Lock(&pData, NULL, NULL);
	if (SUCCEEDED(hr)) {
		D3D11_MAPPED_SUBRESOURCE mapped = { 0, };
		hr = m_pDeviceContext->Map(pTexture, 0, D3D11_MAP_READ, 0, &mapped);
		if (SUCCEEDED(hr)) {
			hr = MFCopyImage(
				pData,                      // Destination buffer.
				cbWidth,                    // Destination stride.
				reinterpret_cast<BYTE*>(mapped.pData),    // First row in source image.
				mapped.RowPitch,                    // Source stride.
				cbWidth,                    // Image width in bytes.
				height                // Image height in pixels.
			);
		}
	}
	if (pBuffer) {
		pBuffer->Unlock();
	}

	if (FAILED(hr)) {
		SafeRelease(&pTexture);
		SafeRelease(&pBuffer);
		return nullptr;
	}

	// Set the data length of the buffer.
	hr = pBuffer->SetCurrentLength(cbBuffer);
	if (FAILED(hr)) {
		SafeRelease(&pTexture);
		SafeRelease(&pBuffer);
		return nullptr;
	}

	SafeRelease(&pTexture);
	return pBuffer;
}

bool CContext::initializeSurfaceHandle(HANDLE hSurface)
{
	m_hSurface = hSurface;

	ID3D11Texture2D* pSharedTexture = nullptr;
	HRESULT hr = m_pDevice->OpenSharedResource(m_hSurface, __uuidof(ID3D11Texture2D), (void**)(&pSharedTexture));
	if (FAILED(hr)) {
		ERROR_LOG(L"failed to `OpenSharedResource`, hr: 0x%lx", hr);
		return false;
	}

	D3D11_TEXTURE2D_DESC desc = { 0, };
	pSharedTexture->GetDesc(&desc);
	::SafeRelease(&pSharedTexture);

	m_desc.Width = desc.Width;
	m_desc.Height = desc.Height;
	m_desc.Format = desc.Format;
	m_desc.ArraySize = 1;
	m_desc.BindFlags = 0; // D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
	m_desc.MiscFlags = 0; // D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
	m_desc.SampleDesc.Count = 1;
	m_desc.SampleDesc.Quality = 0;
	m_desc.MipLevels = 1;
	m_desc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ;
	m_desc.Usage = D3D11_USAGE::D3D11_USAGE_STAGING;

	return true;
}

ID3D11Texture2D* CContext::GetSurfaceTexture() const
{
	ID3D11Texture2D* pFrameCopy = nullptr;
	HRESULT hr = m_pDevice->CreateTexture2D(&m_desc, nullptr, &pFrameCopy);
	if (FAILED(hr)) {
		SafeRelease(&pFrameCopy);
		return nullptr;
	}

	ID3D11Texture2D* pSharedTexture = nullptr;
	hr = m_pDevice->OpenSharedResource(m_hSurface, __uuidof(ID3D11Texture2D), (void**)(&pSharedTexture));
	if (FAILED(hr)) {
		SafeRelease(&pFrameCopy);
		SafeRelease(&pSharedTexture);
		return nullptr;
	}

	m_pDeviceContext->CopyResource(pFrameCopy, pSharedTexture);
	SafeRelease(&pSharedTexture);
	return pFrameCopy;
}
