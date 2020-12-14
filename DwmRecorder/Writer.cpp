#include "pch.h"
#include "Writer.h"

namespace videoformat {

namespace {

IMFMediaType* _createMediaType(UINT32 width, UINT32 height, UINT32 fps) {
    IMFMediaType* pMediaType = nullptr;
    HRESULT hr = MFCreateMediaType(&pMediaType);
    if (FAILED(hr)) {
        SafeRelease(&pMediaType);
        return nullptr;
    }

    hr = pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (FAILED(hr)) {
        SafeRelease(&pMediaType);
        return nullptr;
    }

    hr = pMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (FAILED(hr)) {
        SafeRelease(&pMediaType);
        return nullptr;
    }

    hr = MFSetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, width, height);
    if (FAILED(hr)) {
        SafeRelease(&pMediaType);
        return nullptr;
    }

    hr = MFSetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, fps, 1);
    if (FAILED(hr)) {
        SafeRelease(&pMediaType);
        return nullptr;
    }

    hr = MFSetAttributeRatio(pMediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (FAILED(hr)) {
        SafeRelease(&pMediaType);
        return nullptr;
    }

    return pMediaType;
}

}

IMFMediaType* createOutputMediaType(UINT32 width, UINT32 height, UINT32 fps)
{
    IMFMediaType* pMediaTypeOut = _createMediaType(width, height, fps);
    if (!pMediaTypeOut) {
        return nullptr;
    }

    HRESULT hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    if (FAILED(hr)) {
        SafeRelease(&pMediaTypeOut);
        return nullptr;
    }

    constexpr UINT32 VIDEO_BIT_RATE = 800000;
    hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, VIDEO_BIT_RATE);
    if (FAILED(hr)) {
        SafeRelease(&pMediaTypeOut);
        return nullptr;
    }

    return pMediaTypeOut;
}

IMFMediaType* createInputMediaType(UINT32 width, UINT32 height, UINT32 fps)
{
    IMFMediaType* pMediaTypeIn = _createMediaType(width, height, fps);
    if (!pMediaTypeIn) {
        return nullptr;
    }

    HRESULT hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_ARGB32);
    if (FAILED(hr)) {
        SafeRelease(&pMediaTypeIn);
        return nullptr;
    }

    return pMediaTypeIn;
}

}

CWriter::CWriter(UINT32 width, UINT32 height, UINT32 fps)
    : m_width(width)
    , m_height(height)
    , m_fps(fps)
    , m_frameDuration(10 * 1000 * 1000 / fps)
    , m_streamIndex(0)
    , m_pWriter(nullptr)
    , __videoFrameBuffer(new DWORD[width * height])
{
    for (DWORD i = 0; i < width * height; ++i) {
        __videoFrameBuffer[i] = 0x0000FF00;
    }

    HRESULT hr = MFCreateSinkWriterFromURL(L"output.mp4", NULL, NULL, &m_pWriter);

    IMFMediaType* pMediaTypeOut = videoformat::createOutputMediaType(width, height, fps);
    if (pMediaTypeOut) {
        hr = m_pWriter->AddStream(pMediaTypeOut, &m_streamIndex);
    }

    IMFMediaType* pMediaTypeIn = videoformat::createInputMediaType(width, height, fps);
    if (pMediaTypeIn) {
        hr = m_pWriter->SetInputMediaType(m_streamIndex, pMediaTypeIn, NULL);
    }

    hr = m_pWriter->BeginWriting();

    SafeRelease(&pMediaTypeOut);
    SafeRelease(&pMediaTypeIn);
}

CWriter::~CWriter()
{
    if (m_pWriter) {
        m_pWriter->Finalize();
    }
    SafeRelease(&m_pWriter);

    if (__videoFrameBuffer) {
        delete[] __videoFrameBuffer;
    }
}

HRESULT CWriter::writeFrame(const LONGLONG nsTimestamp)
{
    const LONG cbWidth = 4 * m_width;
    const DWORD cbBuffer = cbWidth * m_height;

    // Create a new memory buffer.
    IMFMediaBuffer* pBuffer = NULL;
    HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);
    if (FAILED(hr)) {
        return hr;
    }

    BYTE* pData = NULL;
    // Lock the buffer and copy the video frame to the buffer.
    hr = pBuffer->Lock(&pData, NULL, NULL);
    if (SUCCEEDED(hr)) {
        hr = MFCopyImage(
            pData,                      // Destination buffer.
            cbWidth,                    // Destination stride.
            (BYTE*)__videoFrameBuffer,    // First row in source image.
            cbWidth,                    // Source stride.
            cbWidth,                    // Image width in bytes.
            m_height                // Image height in pixels.
        );
    }
    if (pBuffer) {
        pBuffer->Unlock();
    }

    // Set the data length of the buffer.
    if (SUCCEEDED(hr)) {
        hr = pBuffer->SetCurrentLength(cbBuffer);
    }

    // Create a media sample and add the buffer to the sample.
    IMFSample* pSample = NULL;
    if (SUCCEEDED(hr)) {
        hr = MFCreateSample(&pSample);
    }
    if (SUCCEEDED(hr)) {
        hr = pSample->AddBuffer(pBuffer);
    }

    // Set the time stamp and the duration.
    if (SUCCEEDED(hr)) {
        hr = pSample->SetSampleTime(nsTimestamp);
    }
    if (SUCCEEDED(hr)) {
        hr = pSample->SetSampleDuration(m_frameDuration);
    }

    // Send the sample to the Sink Writer.
    if (SUCCEEDED(hr)) {
        hr = m_pWriter->WriteSample(m_streamIndex, pSample);
    }

    SafeRelease(&pSample);
    SafeRelease(&pBuffer);
    return hr;
}
