#pragma once
class CWriter
{
public:
	CWriter(UINT32 width, UINT32 height, UINT32 fps);
	virtual ~CWriter();

	HRESULT writeFrame(const LONGLONG nsTimestamp);

private:
	UINT32 m_width;
	UINT32 m_height;
	UINT32 m_fps;
	UINT64 m_frameDuration;
	DWORD m_streamIndex;

	IMFSinkWriter* m_pWriter;

	DWORD* __videoFrameBuffer;
};
