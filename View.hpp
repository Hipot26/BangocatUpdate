#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Math/CubismMatrix44.hpp>
#include <Math/CubismViewMatrix.hpp>
#include "CubismFramework.hpp"
#include <Rendering/OpenGL/CubismOffscreenSurface_OpenGLES2.hpp>

// Límites originales
#define MAXVIEWDATA  1024
#define MAXMODECOUNT   24
#define MAXKEYCOUNT   128
#define MAXFACECOUNT   64
// Nuevos
#define MAXFACESTATES   8
#define MAXFACEFRAMES  16
#define MAXSKINS       16

class Model;
class Sprite;
class InfoReader;
class EventManager;

enum SelectTarget {
    SelectTarget_None,
    SelectTarget_ModelFrameBuffer,
    SelectTarget_ViewFrameBuffer,
};

// ── Estructura Mode (original, sin cambios) ───────────────────────────────────
struct Mode {
    Sprite  *_back;
    Sprite  *_catback;
    bool     _haseModel;
    int      _modelId;
    bool     _hasRightHandModel;
    int      _leftHandModelId;
    Sprite  *_rightHandUp;
    uint16_t _rightHandsCount;
    Sprite  *_rightHands[MAXKEYCOUNT];
    bool     _hasLeftHandModel;
    int      _rightHandModelId;
    Sprite  *_leftHandUp;
    uint16_t _leftHandsCount;
    Sprite  *_leftHands[MAXKEYCOUNT];
    uint16_t _keysCount;
    Sprite  *_keys[MAXKEYCOUNT];
};

// ── NUEVO: estado del animador facial ─────────────────────────────────────────
struct FaceAnimState {
    int   stateIdx;
    int   frameIdx;
    float frameTimer;
    float blinkTimer;
    float blinkInterval;
    int   returnStateIdx;
    bool  isTalking;       // true cuando el mic detecta voz
};

// ── NUEVO: estado de un skin activo ──────────────────────────────────────────
struct ActiveSkin {
    int   skinIdx;        // índice en InfoReader::skins.skins[]
    float timeRemaining;  // segundos restantes (0=permanente)
    bool  active;
};

// ── ViewData (original + nuevos campos) ───────────────────────────────────────
struct ViewData {
    Csm::CubismViewMatrix *_viewMatrix;
    Csm::CubismMatrix44   *_deviceToScreen;
    int  _modecount;
    Mode _mode[MAXMODECOUNT];
    // Máscara original
    int     _curentface;
    int     _faceCount;
    Sprite *_face[MAXFACECOUNT];
    // NUEVO: sprites de animación facial
    Sprite *faceFrames[MAXFACESTATES][MAXFACEFRAMES];
    bool    faceFramesLoaded;
    // NUEVO: sprites de skins
    Sprite *skinSprites[MAXSKINS];
    // NUEVO: skin activo
    ActiveSkin activeSkin;
    // NUEVO: animador facial
    FaceAnimState faceAnim;
};

// ── View ──────────────────────────────────────────────────────────────────────
class View {
public:
    View();
    ~View();

    // API original (sin cambios de firma)
    void Initialize(int id);
    void Release(int id);
    void Render(int id);
    void InitializeSpirite(int id);
    void InitializeModel(int id);
    void PreModelDraw(Model &refModel);
    void PostModelDraw(Model &refModel);
    void SwitchRenderingTarget(SelectTarget targetType, int id);
    void SetRenderTargetClearColor(float r, float g, float b);
    Csm::CubismViewMatrix *GetViewMatrix(int id);
    Csm::CubismMatrix44   *GetDeviceToScreenMatrix(int id);
    InfoReader            *GetInfoReader();
    float TransformViewX(float deviceX, int id) const;
    float TransformViewY(float deviceY, int id) const;
    EventManager *GetEventManager();
    void OnMouseMoved(float pointX, float pointY, int id) const;
    void Update(bool _isLive2D, bool _isUseMask);
    void setMod(uint16_t i);

    // NUEVO: control de animación facial desde VtuberPlugin (audio)
    // SetFaceStateTalk: activa talk y permite parpadeo durante habla
    // SetFaceStateIdle: vuelve a idle (parpadeo normal)
    void SetFaceStateTalk(int id);
    void SetFaceStateIdle(int id);
    void SetFaceState(const char *stateName, int id);

    // NUEVO: control de skins desde VtuberPlugin (hotkey/canal)
    void ActivateSkin(int skinIdx, int id);
    void DeactivateSkin(int id);
    void OnChannelPointReward(const char *skinId, int id);

private:
    // Original
    void UpdataViewData(int id);
    int  TranslateKey(int key, int id);
    int  TranslateKey2(int key, int id);
    void RenderBackgroud(int id);
    void RenderCat(int id);
    void RenderKeys(int id);
    bool RenderLeftHands(int id);
    bool RenderRightHands(int id);
    void RenderUphands(bool leftup, bool righttup, int id);
    void ReanderMask(int id);
    void ChangeMode(int mod, int id);

    // NUEVO: animación facial
    void InitFaceAnimState(int id);
    void UpdateFaceAnim(int id, float dt);
    void RenderFaceAnim(int id);
    int  FindFaceState(const char *name);

    // NUEVO: skins
    void CheckSkinHotkeys(int id);
    void UpdateSkinTimer(int id, float dt);
    void RenderSkinsByZOrder(int id, int zOrder);

    ViewData _viewData[MAXVIEWDATA];
    uint16_t _mod;
    bool     isUseLive2d;
    bool     isUseMask;
    double   _lastRenderTime;   // NUEVO: para deltaTime

    // NUEVO: flanco de tecla para skins (evita toggle rapido)
    bool     _skinKeyWasPressed[MAXSKINS];

    GLuint   _programId;
    Csm::Rendering::CubismOffscreenFrame_OpenGLES2 _renderBuffer;
    float    _clearColor[4];
    SelectTarget  target;
    EventManager *eventManager;
    InfoReader   *_infoReader;
};