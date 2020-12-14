#pragma once

class CContext
{
public:
	CContext();
	virtual ~CContext();

	bool initialize(HANDLE hSurface);
	void finalize();

private:
	HANDLE m_hSurface;
	ID3D11Device* m_pDevice;
	ID3D11DeviceContext* m_pDeviceContext;
};
