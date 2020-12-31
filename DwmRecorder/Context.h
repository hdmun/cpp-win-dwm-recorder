#pragma once

class CContext
{
public:
	CContext();
	virtual ~CContext();

	bool initialize();
	void finalize();

	void start(HANDLE hSurface, UINT32 fps);
	void stop() { m_bRecording = false; }

	IMFMediaBuffer* CreateMediaBuffer(UINT32 width, UINT32 height) const;

private:
	bool initializeSurfaceHandle(HANDLE hSurface);
	ID3D11Texture2D* GetSurfaceTexture() const;

private:
	HANDLE m_hSurface;
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
	D3D11_TEXTURE2D_DESC m_desc;

	bool m_bRecording;
};
