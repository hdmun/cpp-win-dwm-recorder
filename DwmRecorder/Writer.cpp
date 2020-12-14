#include "pch.h"
#include "Writer.h"
#include "Context.h"

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

namespace {

HRESULT SetCodecAttributeU32(ICodecAPI* pCodec, const GUID& guid, UINT32 value)
{
    VARIANT val;
    val.vt = VT_UI4;
    val.uintVal = value;
    return pCodec->SetValue(&guid, &val);
}

}


CWriter::CWriter(UINT32 width, UINT32 height, UINT32 fps)
    : m_width(width)
    , m_height(height)
    , m_fps(fps)
    , m_streamIndex(0)
    , m_pWriter(nullptr)
{
    initializeSinkWriter();
    initializeEncoder();

    HRESULT hr = m_pWriter->BeginWriting();
    if (FAILED(hr)) {
        // assert!
    }
}

CWriter::~CWriter()
{
    if (m_pWriter) {
        m_pWriter->Finalize();
    }
    SafeRelease(&m_pWriter);
}


void CWriter::initializeSinkWriter()
{
    // HRESULT hr = MFCreateSinkWriterFromURL(L"output.mp4", NULL, NULL, &m_pWriter);
    IMFByteStream* pOutStream = nullptr;
    HRESULT hr = ::MFCreateFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_RESET_IF_EXIST, MF_FILEFLAGS_NONE, L"output.mp4", &pOutStream);
    if (FAILED(hr)) {
        return;
    }

    IMFMediaType* pMediaTypeOut = videoformat::createOutputMediaType(m_width, m_height, m_fps);
    if (!pMediaTypeOut) {
        return;
    }

    IMFMediaSink* pMp4StreamSink = nullptr;
    ::MFCreateMPEG4MediaSink(pOutStream, pMediaTypeOut, nullptr, &pMp4StreamSink);
    SafeRelease(&pMediaTypeOut);

    IMFAttributes* pAttributes = nullptr;
    ::MFCreateAttributes(&pAttributes, 6);
    pAttributes->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_MPEG4);
    pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, true);
    pAttributes->SetUINT32(MF_MPEG4SINK_MOOV_BEFORE_MDAT, true);
    pAttributes->SetUINT32(MF_LOW_LATENCY, false);
    pAttributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, false);

    hr = ::MFCreateSinkWriterFromMediaSink(pMp4StreamSink, pAttributes, &m_pWriter);
    if (FAILED(hr) || !m_pWriter) {
        return;
    }

    IMFMediaType* pMediaTypeIn = videoformat::createInputMediaType(m_width, m_height, m_fps);
    if (pMediaTypeIn) {
        hr = m_pWriter->SetInputMediaType(m_streamIndex, pMediaTypeIn, NULL);
        if (FAILED(hr)) {
            return;
        }
    }

    SafeRelease(&pMediaTypeIn);
}

void CWriter::initializeEncoder()
{
    ICodecAPI* pEncoder = nullptr;
    m_pWriter->GetServiceForStream(m_streamIndex, GUID_NULL, IID_PPV_ARGS(&pEncoder));
    if (pEncoder) {
        constexpr UINT32 usVideoBitrateControlMode = eAVEncCommonRateControlMode_Quality;
        SetCodecAttributeU32(pEncoder, CODECAPI_AVEncCommonRateControlMode, usVideoBitrateControlMode);

        switch (usVideoBitrateControlMode) {
        case eAVEncCommonRateControlMode_Quality:
        {
            constexpr UINT32 usVideoQuality = 70;
            SetCodecAttributeU32(pEncoder, CODECAPI_AVEncCommonQuality, usVideoQuality);
            break;
        }
        default:
            break;
        }
    }
}

HRESULT CWriter::writeFrame(const LONGLONG nsTimestamp, UINT64 duration, CContext* ctx)
{
    // Create a new memory buffer.
    IMFMediaBuffer* pBuffer = ctx->CreateMediaBuffer(m_width, m_height);

    // Create a media sample and add the buffer to the sample.
    IMFSample* pSample = NULL;
    HRESULT hr = MFCreateSample(&pSample);
    if (SUCCEEDED(hr)) {
        hr = pSample->AddBuffer(pBuffer);
    }

    // Set the time stamp and the duration.
    if (SUCCEEDED(hr)) {
        hr = pSample->SetSampleTime(nsTimestamp);
    }
    if (SUCCEEDED(hr)) {
        hr = pSample->SetSampleDuration(duration);
    }

    // Send the sample to the Sink Writer.
    if (SUCCEEDED(hr)) {
        hr = m_pWriter->WriteSample(m_streamIndex, pSample);
    }

    SafeRelease(&pSample);
    SafeRelease(&pBuffer);
    return hr;
}
