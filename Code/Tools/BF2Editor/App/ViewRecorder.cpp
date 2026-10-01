// Code/Tools/BF2Editor/App/ViewRecorder.cpp - see ViewRecorder.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/ViewRecorder.h"

#include "Framework/EditorLog.h"

#include <d3d9.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace bf2editor
{

namespace
{

std::wstring widen(const std::string& text)
{
    if (text.empty())
        return std::wstring();
    const int n = MultiByteToWideChar(CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), 0, 0);
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_ACP, 0, text.c_str(), static_cast<int>(text.size()), &out[0], n);
    return out;
}

std::string hresultText(const char* what, HRESULT hr)
{
    char text[160];
    std::snprintf(text, sizeof(text), "%s failed (HRESULT 0x%08lX)", what, static_cast<unsigned long>(hr));
    return text;
}

const unsigned kFrameRate = 30;             // the stream's nominal rate; the samples carry their own times
const unsigned kBitRate = 16000000;         // 16 Mbit/s

} // namespace

ViewRecorder::ViewRecorder()
    : m_writer(0)
    , m_stream(0)
    , m_recording(false)
    , m_started(false)
    , m_comInitialised(false)
    , m_frames(0)
    , m_width(0)
    , m_height(0)
    , m_lastTime(0)
{
    m_begin.QuadPart = 0;
}

ViewRecorder::~ViewRecorder()
{
    // shutdown() ran at the editor's exit; this only lets go of what is left.
    close();
}

void ViewRecorder::shutdown()
{
    if (m_recording)
    {
        std::string result;
        stop(result);
    }
    close();
    if (m_started)
        MFShutdown();
    m_started = false;
    if (m_comInitialised)
        CoUninitialize();
    m_comInitialised = false;
}

bool ViewRecorder::start(const std::string& path, std::string& error)
{
    if (m_recording)
    {
        error = "the view is being recorded already (to " + m_path + ")";
        return false;
    }
    if (path.size() < 5 || _stricmp(path.c_str() + path.size() - 4, ".mp4") != 0)
    {
        error = "the recording is an .mp4 file: give a path that ends in .mp4";
        return false;
    }
    if (!m_comInitialised)
    {
        const HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
        m_comInitialised = SUCCEEDED(hr);       // RPC_E_CHANGED_MODE: COM is up in another mode, which serves too
    }
    if (!m_started)
    {
        const HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);
        if (FAILED(hr))
        {
            error = hresultText("starting Media Foundation", hr) + "; the view cannot be recorded on this system";
            return false;
        }
        m_started = true;
    }
    // The file is opened with the first frame, whose size it takes.
    m_path = path;
    m_frames = 0;
    m_width = m_height = 0;
    m_lastTime = 0;
    m_error.clear();
    m_recording = true;
    QueryPerformanceCounter(&m_begin);
    logLine(LogLevel::Info, "record: recording the view to %s", m_path.c_str());
    return true;
}

bool ViewRecorder::open(unsigned width, unsigned height, std::string& error)
{
    IMFAttributes* attributes = 0;
    MFCreateAttributes(&attributes, 1);
    if (attributes)
        attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
    HRESULT hr = MFCreateSinkWriterFromURL(widen(m_path).c_str(), 0, attributes, &m_writer);
    if (attributes)
        attributes->Release();
    if (FAILED(hr) || !m_writer)
    {
        error = hresultText(("opening " + m_path).c_str(), hr);
        return false;
    }
    IMFMediaType* output = 0;
    IMFMediaType* input = 0;
    hr = MFCreateMediaType(&output);
    if (SUCCEEDED(hr)) hr = output->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(hr)) hr = output->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    if (SUCCEEDED(hr)) hr = output->SetUINT32(MF_MT_AVG_BITRATE, kBitRate);
    if (SUCCEEDED(hr)) hr = output->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (SUCCEEDED(hr)) hr = MFSetAttributeSize(output, MF_MT_FRAME_SIZE, width, height);
    if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(output, MF_MT_FRAME_RATE, kFrameRate, 1);
    if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(output, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    if (SUCCEEDED(hr)) hr = m_writer->AddStream(output, &m_stream);
    if (SUCCEEDED(hr)) hr = MFCreateMediaType(&input);
    if (SUCCEEDED(hr)) hr = input->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(hr)) hr = input->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (SUCCEEDED(hr)) hr = input->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    if (SUCCEEDED(hr)) hr = MFSetAttributeSize(input, MF_MT_FRAME_SIZE, width, height);
    if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(input, MF_MT_FRAME_RATE, kFrameRate, 1);
    if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(input, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    // Rows top-down, as the back buffer has them.
    if (SUCCEEDED(hr)) hr = input->SetUINT32(MF_MT_DEFAULT_STRIDE, width * 4);
    if (SUCCEEDED(hr)) hr = m_writer->SetInputMediaType(m_stream, input, 0);
    if (SUCCEEDED(hr)) hr = m_writer->BeginWriting();
    if (output)
        output->Release();
    if (input)
        input->Release();
    if (FAILED(hr))
    {
        error = hresultText("setting up the H.264 encoder", hr);
        m_writer->Release();
        m_writer = 0;
        DeleteFileA(m_path.c_str());
        return false;
    }
    m_width = width;
    m_height = height;
    return true;
}

void ViewRecorder::frame(IDirect3DDevice9* device)
{
    if (!m_recording || !device)
        return;
    IDirect3DSurface9* back = 0;
    if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) || !back)
        return;
    D3DSURFACE_DESC desc;
    back->GetDesc(&desc);
    std::string error;
    if (desc.Format != D3DFMT_A8R8G8B8 && desc.Format != D3DFMT_X8R8G8B8)
        error = "the back buffer is not 32-bit RGB";
    // The encoder takes even sizes: an odd last row or column is left out.
    const unsigned width = desc.Width & ~1u, height = desc.Height & ~1u;
    if (error.empty())
    {
        if (!m_writer)
            open(width, height, error);         // the first frame: the file takes its size
        else if (width != m_width || height != m_height)
            error = "the view changed size while it was recorded";
    }
    IDirect3DSurface9* copy = 0;
    if (error.empty())
    {
        if (FAILED(device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &copy, 0))
            || FAILED(device->GetRenderTargetData(back, copy)))
            error = "the back buffer could not be read";
    }
    if (error.empty())
    {
        D3DLOCKED_RECT locked;
        IMFMediaBuffer* buffer = 0;
        IMFSample* sample = 0;
        BYTE* target = 0;
        HRESULT hr = MFCreateMemoryBuffer(m_width * m_height * 4, &buffer);
        if (SUCCEEDED(hr)) hr = buffer->Lock(&target, 0, 0);
        if (SUCCEEDED(hr) && SUCCEEDED(copy->LockRect(&locked, 0, D3DLOCK_READONLY)))
        {
            for (unsigned y = 0; y < m_height; ++y)
                std::memcpy(target + static_cast<size_t>(y) * m_width * 4,
                            static_cast<const BYTE*>(locked.pBits) + static_cast<size_t>(y) * locked.Pitch, m_width * 4);
            copy->UnlockRect();
        }
        if (target)
            buffer->Unlock();
        if (SUCCEEDED(hr)) hr = buffer->SetCurrentLength(m_width * m_height * 4);
        if (SUCCEEDED(hr)) hr = MFCreateSample(&sample);
        if (SUCCEEDED(hr)) hr = sample->AddBuffer(buffer);
        // The frame's time: when it was drawn, after the recording began (100 ns units), always later than
        // the last one's.
        LARGE_INTEGER now, frequency;
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&frequency);
        long long time = static_cast<long long>(static_cast<double>(now.QuadPart - m_begin.QuadPart) * 1e7
                                                / static_cast<double>(frequency.QuadPart));
        if (m_frames > 0 && time <= m_lastTime)
            time = m_lastTime + 1;
        const long long duration = m_frames > 0 ? time - m_lastTime : 10000000LL / kFrameRate;
        if (SUCCEEDED(hr)) hr = sample->SetSampleTime(m_frames > 0 ? time : 0);
        if (SUCCEEDED(hr)) hr = sample->SetSampleDuration(duration);
        if (SUCCEEDED(hr)) hr = m_writer->WriteSample(m_stream, sample);
        if (sample)
            sample->Release();
        if (buffer)
            buffer->Release();
        if (FAILED(hr))
            error = hresultText("writing a frame", hr);
        else
        {
            m_lastTime = m_frames > 0 ? time : 0;
            ++m_frames;
        }
    }
    if (copy)
        copy->Release();
    back->Release();
    if (!error.empty())
    {
        m_error = error;
        logLine(LogLevel::Error, "record: %s; the recording stops", error.c_str());
        std::string ignored;
        stop(ignored);
    }
}

void ViewRecorder::close()
{
    if (m_writer)
    {
        m_writer->Release();
        m_writer = 0;
    }
}

bool ViewRecorder::stop(std::string& result)
{
    if (!m_recording)
    {
        result = "the view is not being recorded";
        return false;
    }
    m_recording = false;
    bool ok = m_error.empty();
    if (m_writer)
    {
        const HRESULT hr = m_writer->Finalize();
        if (FAILED(hr) && ok)
        {
            m_error = hresultText("finishing the file", hr);
            ok = false;
        }
    }
    close();
    if (!m_frames)
    {
        result = "no frame was recorded" + (m_error.empty() ? std::string() : ": " + m_error);
        return false;
    }
    char text[200];
    std::snprintf(text, sizeof(text), "recorded %u frames (%.1f s, %ux%u) to ", m_frames,
                  static_cast<double>(m_lastTime) / 1e7, m_width, m_height);
    result = text + m_path + (m_error.empty() ? std::string() : "; stopped early: " + m_error);
    logLine(ok ? LogLevel::Info : LogLevel::Warning, "record: %s", result.c_str());
    return ok;
}

ViewRecorder& viewRecorder()
{
    static ViewRecorder recorder;
    return recorder;
}

} // namespace bf2editor
