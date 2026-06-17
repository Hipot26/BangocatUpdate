#pragma once
#include <string>
#include <vector>
#include <functional>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

// ─── MicCapture ───────────────────────────────────────────────────────────────
// Captura audio del micrófono seleccionado usando WASAPI (Windows Audio).
// Calcula RMS en tiempo real y llama al callback con el nivel 0.0-1.0.
// Idéntico al enfoque de Veadotube: capture loopback del dispositivo elegido.

class MicCapture {
public:
    MicCapture();
    ~MicCapture();

    // Lista los micrófonos disponibles (nombre para mostrar en UI)
    static std::vector<std::string> EnumDevices();

    // Inicia la captura del dispositivo con ese índice (0=default)
    bool Start(int deviceIndex);

    // Detiene la captura
    void Stop();

    // Nivel de audio actual (0.0 a 1.0), actualizado por el hilo de captura
    float GetLevel() const { return _level; }

    bool IsRunning() const { return _running; }

private:
    static DWORD WINAPI CaptureThread(LPVOID param);
    void RunCapture();

    IMMDevice          *_device;
    IAudioClient       *_audioClient;
    IAudioCaptureClient*_captureClient;

    volatile float _level;
    volatile bool  _running;
    HANDLE         _thread;
    HANDLE         _stopEvent;
};