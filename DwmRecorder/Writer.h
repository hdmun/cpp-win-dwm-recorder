#pragma once

class CWriter
{
public:
	CWriter(UINT32 width, UINT32 height, UINT32 fps);
	virtual ~CWriter();

private:
	void initializeSinkWriter();
	void initializeEncoder();

public:
	HRESULT writeFrame(IMFMediaBuffer* pBuffer, const LONGLONG nsTimestamp, UINT64 duration);

private:
	UINT32 m_width;
	UINT32 m_height;
	UINT32 m_fps;
	DWORD m_streamIndex;

	IMFSinkWriter* m_pWriter;
};
