#include "MicCapture.hpp"
#include <math.h>
#include <string>
#include <vector>
#include <Functiondiscoverykeys_devpkey.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

// Helper: WCHAR → std::string
static std::string WCharToStr(const WCHAR *w) {
    if (!w) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return "";
    std::string s(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, -1, &s[0], len, NULL, NULL);
    return s;
}

// ─── Constructor / Destructor ─────────────────────────────────────────────────

MicCapture::MicCapture()
    : _device(nullptr), _audioClient(nullptr), _captureClient(nullptr),
      _level(0.0f), _running(false), _thread(nullptr), _stopEvent(nullptr)
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
}

MicCapture::~MicCapture()
{
    Stop();
    CoUninitialize();
}

// ─── EnumDevices ──────────────────────────────────────────────────────────────

std::vector<std::string> MicCapture::EnumDevices()
{
    std::vector<std::string> result;

    IMMDeviceEnumerator *enumerator = nullptr;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                     __uuidof(IMMDeviceEnumerator), (void **)&enumerator);
    if (!enumerator) return result;

    IMMDeviceCollection *collection = nullptr;
    enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection);
    if (!collection) { enumerator->Release(); return result; }

    UINT count = 0;
    collection->GetCount(&count);

    // Añadir "Default Microphone" como primera opción
    result.push_back("Default Microphone");

    for (UINT i = 0; i < count; i++) {
        IMMDevice *device = nullptr;
        collection->Item(i, &device);
        if (!device) continue;

        IPropertyStore *props = nullptr;
        device->OpenPropertyStore(STGM_READ, &props);
        if (props) {
            PROPVARIANT varName;
            PropVariantInit(&varName);
            props->GetValue(PKEY_Device_FriendlyName, &varName);
            if (varName.vt == VT_LPWSTR)
                result.push_back(WCharToStr(varName.pwszVal));
            PropVariantClear(&varName);
            props->Release();
        }
        device->Release();
    }

    collection->Release();
    enumerator->Release();
    return result;
}

// ─── Start ────────────────────────────────────────────────────────────────────

bool MicCapture::Start(int deviceIndex)
{
    Stop(); // detener cualquier captura previa

    IMMDeviceEnumerator *enumerator = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                  CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                  (void **)&enumerator);
    if (FAILED(hr) || !enumerator) return false;

    if (deviceIndex <= 0) {
        // Usar dispositivo por defecto
        hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &_device);
    } else {
        IMMDeviceCollection *collection = nullptr;
        enumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &collection);
        if (collection) {
            collection->Item((UINT)(deviceIndex - 1), &_device);
            collection->Release();
        }
    }
    enumerator->Release();

    if (!_device) return false;

    hr = _device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                            (void **)&_audioClient);
    if (FAILED(hr)) return false;

    // Formato: float 32-bit, misma frecuencia que el dispositivo
    WAVEFORMATEX *pwfx = nullptr;
    _audioClient->GetMixFormat(&pwfx);

    hr = _audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0,
                                   10000000, 0, pwfx, nullptr);
    CoTaskMemFree(pwfx);
    if (FAILED(hr)) return false;

    hr = _audioClient->GetService(__uuidof(IAudioCaptureClient),
                                   (void **)&_captureClient);
    if (FAILED(hr)) return false;

    _audioClient->Start();

    _stopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    _running   = true;
    _thread    = CreateThread(nullptr, 0, CaptureThread, this, 0, nullptr);

    return _thread != nullptr;
}

// ─── Stop ─────────────────────────────────────────────────────────────────────

void MicCapture::Stop()
{
    if (_running) {
        _running = false;
        SetEvent(_stopEvent);
        if (_thread) {
            WaitForSingleObject(_thread, 2000);
            CloseHandle(_thread);
            _thread = nullptr;
        }
        if (_stopEvent) { CloseHandle(_stopEvent); _stopEvent = nullptr; }
    }

    if (_audioClient)   { _audioClient->Stop();  _audioClient->Release();   _audioClient   = nullptr; }
    if (_captureClient) { _captureClient->Release(); _captureClient = nullptr; }
    if (_device)        { _device->Release();        _device        = nullptr; }

    _level = 0.0f;
}

// ─── Hilo de captura ──────────────────────────────────────────────────────────

DWORD WINAPI MicCapture::CaptureThread(LPVOID param)
{
    ((MicCapture *)param)->RunCapture();
    return 0;
}

void MicCapture::RunCapture()
{
    while (_running) {
        // Esperar ~10ms entre lecturas
        WaitForSingleObject(_stopEvent, 10);
        if (!_running) break;

        UINT32 packetLength = 0;
        if (FAILED(_captureClient->GetNextPacketSize(&packetLength))) break;

        while (packetLength > 0) {
            BYTE   *data       = nullptr;
            UINT32  numFrames  = 0;
            DWORD   flags      = 0;

            if (FAILED(_captureClient->GetBuffer(&data, &numFrames, &flags, nullptr, nullptr)))
                break;

            float rms = 0.0f;
            if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT) && data && numFrames > 0) {
                const float *samples = (const float *)data;
                double sum = 0.0;
                for (UINT32 i = 0; i < numFrames; i++) {
                    double s = samples[i];
                    sum += s * s;
                }
                rms = (float)sqrt(sum / numFrames);
            }

            _captureClient->ReleaseBuffer(numFrames);

            // Suavizado: ataque rápido, caída lenta (igual que Veadotube)
            if (rms > _level)
                _level = _level * 0.2f + rms * 0.8f;   // ataque rápido
            else
                _level = _level * 0.92f + rms * 0.08f;  // caída lenta

            if (FAILED(_captureClient->GetNextPacketSize(&packetLength))) break;
        }
    }
}