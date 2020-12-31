#pragma once

class CRecorder
{
public:
	CRecorder();
	virtual ~CRecorder();

	bool initialize();
	void finalize();

	void start(HANDLE hSurface, UINT32 fps);
	void stop() { m_bRecording = false; }

private:
	bool initializeSurfaceHandle(HANDLE hSurface);
	IMFMediaBuffer* createVideoBuffer(UINT32 width, UINT32 height) const;
	ID3D11Texture2D* createSurfaceTexture() const;

private:
	HANDLE m_hSurface;
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
	D3D11_TEXTURE2D_DESC m_desc;

	bool m_bRecording;
};
