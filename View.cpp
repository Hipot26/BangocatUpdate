#include "View.hpp"
#include <math.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <GLFW/glfw3.h>
#include "Pal.hpp"
#include "VtuberDelegate.hpp"
#include "Live2DManager.hpp"
#include "LAppTextureManager.hpp"
#include "Define.hpp"
#include "Model.hpp"
#include "Sprite.hpp"
#include "EventManager.hpp"
#include "InfoReader.hpp"

using namespace std;
using namespace Define;

// ─── Constructor / Destructor ────────────────────────────────────────────────

View::View() : _programId(0), _mod(0), isUseLive2d(true), isUseMask(false),
               _lastRenderTime(0.0)
{
    _clearColor[0] = 1.0f;
    _clearColor[1] = 1.0f;
    _clearColor[2] = 1.0f;
    _clearColor[3] = 0.0f;

    // FIX 2: inicializar estado de teclas de skin (detección de flanco)
    memset(_skinKeyWasPressed, 0, sizeof(_skinKeyWasPressed));

    eventManager = new EventManager();

    const string resourcesPath = ResourcesPath;
    const string modePath      = resourcesPath + ModePath;
    const string maskPath      = resourcesPath + MaskPath;

    _infoReader = new InfoReader();
    _infoReader->InitFaceFromConfig(maskPath.c_str());
    _infoReader->InitFromConfig(modePath.c_str());
    _infoReader->InitFaceAnimConfig(maskPath.c_str());
    _infoReader->InitSkinsConfig((resourcesPath + "skins/").c_str());
}

View::~View()
{
    _renderBuffer.DestroyOffscreenFrame();
    delete eventManager;
    delete _infoReader;
}

// ─── Initialize ──────────────────────────────────────────────────────────────

void View::Initialize(int id)
{
    int width  = VtuberDelegate::GetInstance()->getBufferWidth(id);
    int height = VtuberDelegate::GetInstance()->getBufferHeight(id);
    if (width == 0 || height == 0) return;

    float ratio  = static_cast<float>(height) / static_cast<float>(width);
    float left   = ViewLogicalLeft;
    float right  = ViewLogicalRight;
    float bottom = -ratio;
    float top    =  ratio;

    _viewData[id]._viewMatrix     = new CubismViewMatrix();
    _viewData[id]._deviceToScreen = new CubismMatrix44();

    _viewData[id]._viewMatrix->SetScreenRect(left, right, bottom, top);

    float screenW = fabsf(left - right);
    _viewData[id]._deviceToScreen->LoadIdentity();
    _viewData[id]._deviceToScreen->ScaleRelative(screenW / width, -screenW / width);
    _viewData[id]._deviceToScreen->TranslateRelative(-width, -height);

    _viewData[id]._viewMatrix->SetMaxScale(ViewMaxScale);
    _viewData[id]._viewMatrix->SetMinScale(ViewMinScale);
    _viewData[id]._viewMatrix->SetMaxScreenRect(
        ViewLogicalMaxLeft, ViewLogicalMaxRight,
        ViewLogicalMaxBottom, ViewLogicalMaxTop);

    InitFaceAnimState(id);
    _viewData[id].activeSkin.active = false;
    _lastRenderTime = glfwGetTime();
}

// ─── Release ─────────────────────────────────────────────────────────────────

void View::Release(int id)
{
    if (_viewData[id]._viewMatrix)     delete _viewData[id]._viewMatrix;
    if (_viewData[id]._deviceToScreen) delete _viewData[id]._deviceToScreen;
    for (int i = 0; i < _viewData[id]._faceCount; i++)
        delete _viewData[id]._face[i];
}

// ─── UpdataViewData (original) ───────────────────────────────────────────────

void View::UpdataViewData(int id)
{
    int    width  = VtuberDelegate::GetInstance()->getBufferWidth(id);
    int    height = VtuberDelegate::GetInstance()->getBufferHeight(id);
    double x      = VtuberDelegate::GetInstance()->GetX(id);
    double y      = VtuberDelegate::GetInstance()->GetY(id);
    double scale  = VtuberDelegate::GetInstance()->getScale(id);

    double _x = -static_cast<float>(RenderTargetWidth  - width)  / static_cast<float>(RenderTargetWidth);
    double _y = -static_cast<float>(RenderTargetHeight - height) / static_cast<float>(RenderTargetHeight);

    if (_viewData[id]._viewMatrix) {
        _viewData[id]._viewMatrix->Scale(0.615 * scale, scale * 1.1);
        _viewData[id]._viewMatrix->Translate(x + _x, y + _y);
    } else {
        Initialize(id);
        _viewData[id]._viewMatrix->Scale(0.615 * scale, scale * 1.1);
        _viewData[id]._viewMatrix->Translate(x + _x, y + _y);
    }
}

// ─── TranslateKey (original) ─────────────────────────────────────────────────

int View::TranslateKey(int key, int id)
{
    for (int j = 0; j < _viewData[id]._mode[_mod]._keysCount; j++)
        if (strcmp(_infoReader->_modeInfo[_mod].KeyUse[j], KeyDefine[key]) == 0)
            return j;
    return -1;
}

int View::TranslateKey2(int key, int id)
{
    for (int j = 0; j < _viewData[id]._faceCount; j++)
        if (strcmp(_infoReader->_faceInfo[0].HotKey[j], KeyDefine[key]) == 0)
            return j;
    return -1;
}

// ─── Render de capas (original) ──────────────────────────────────────────────

void View::RenderBackgroud(int id)
{
    if (isUseLive2d && _viewData[id]._mode[_mod]._haseModel) {
        if (_viewData[id]._mode[_mod]._back)
            _viewData[id]._mode[_mod]._back->Render(id);
    } else {
        if (_viewData[id]._mode[_mod]._catback)
            _viewData[id]._mode[_mod]._catback->Render(id);
    }
}

void View::RenderCat(int id)
{
    Live2DManager *mgr = Live2DManager::GetInstance();
    if (isUseLive2d && _viewData[id]._mode[_mod]._haseModel)
        mgr->OnUpdate(_viewData[id]._mode[_mod]._modelId);
}

void View::RenderKeys(int id)
{
    for (int i = 0; i < KeyAmount; i++) {
        if (eventManager->GetKeySignal(i)) {
            int key = TranslateKey(i, id);
            if (key != -1 && _viewData[id]._mode[_mod]._keys[key])
                _viewData[id]._mode[_mod]._keys[key]->Render(id);
        }
    }
}

bool View::RenderLeftHands(int id)
{
    Live2DManager *mgr = Live2DManager::GetInstance();
    bool isUp = true;
    if (_viewData[id]._mode[_mod]._leftHandsCount > 0) {
        for (int i = 0; i < KeyAmount; i++) {
            if (eventManager->GetKeySignal(i)) {
                int key = TranslateKey(i, id);
                if (key != -1 && key < _viewData[id]._mode[_mod]._leftHandsCount) {
                    if (_viewData[id]._mode[_mod]._leftHands[key])
                        _viewData[id]._mode[_mod]._leftHands[key]->Render(id);
                    isUp = false; break;
                }
            }
        }
    } else if (_viewData[id]._mode[_mod]._hasLeftHandModel) {
        if (!isUseLive2d) mgr->OnUpdate(_viewData[id]._mode[_mod]._leftHandModelId);
        isUp = false;
    } else if (isUseLive2d) isUp = false;
    return isUp;
}

bool View::RenderRightHands(int id)
{
    Live2DManager *mgr = Live2DManager::GetInstance();
    bool isUp = true;
    if (_viewData[id]._mode[_mod]._rightHandsCount > 0) {
        for (int i = 0; i < KeyAmount; i++) {
            if (eventManager->GetKeySignal(i)) {
                int key = TranslateKey(i, id) - _viewData[id]._mode[_mod]._leftHandsCount;
                if (key >= 0 && key < _viewData[id]._mode[_mod]._rightHandsCount) {
                    if (_viewData[id]._mode[_mod]._rightHands[key])
                        _viewData[id]._mode[_mod]._rightHands[key]->Render(id);
                    isUp = false; break;
                }
            }
        }
    } else if (_viewData[id]._mode[_mod]._hasRightHandModel) {
        if (!isUseLive2d) {
            float offsetY = _infoReader->_modeInfo[_mod].ModelRightHandOffsetY;
            mgr->OnUpdate(_viewData[id]._mode[_mod]._rightHandModelId, offsetY);
        }
        isUp = false;
    } else if (isUseLive2d) isUp = false;
    return isUp;
}

void View::RenderUphands(bool leftup, bool righttup, int id)
{
    if (leftup  && _viewData[id]._mode[_mod]._leftHandUp)
        _viewData[id]._mode[_mod]._leftHandUp->Render(id);
    if (righttup && _viewData[id]._mode[_mod]._rightHandUp)
        _viewData[id]._mode[_mod]._rightHandUp->Render(id);
}

void View::ReanderMask(int id)
{
    for (int i = 0; i < KeyAmount; i++) {
        if (eventManager->GetKeySignal(i)) {
            int key = TranslateKey2(i, id);
            if (key >= 0 && key < _viewData[id]._faceCount) {
                _viewData[id]._curentface = key; break;
            }
        }
    }
    if (isUseMask && _viewData[id]._face[_viewData[id]._curentface])
        _viewData[id]._face[_viewData[id]._curentface]->Render(id);
}

void View::ChangeMode(int mod, int id) {}

// ─── Animación facial ─────────────────────────────────────────────────────────

int View::FindFaceState(const char *name)
{
    if (!name) return 0;
    for (int i = 0; i < _infoReader->faceAnim.stateCount; i++)
        if (_infoReader->faceAnim.states[i].name &&
            strcmp(_infoReader->faceAnim.states[i].name, name) == 0)
            return i;
    return 0;
}

void View::InitFaceAnimState(int id)
{
    FaceAnimState &fa = _viewData[id].faceAnim;
    memset(&fa, 0, sizeof(fa));
    if (!_infoReader->faceAnim.enabled) return;
    fa.returnStateIdx = FindFaceState(_infoReader->faceAnim.idleState);
    fa.stateIdx       = fa.returnStateIdx;
    float mn = _infoReader->faceAnim.blinkIntervalMin;
    float mx = _infoReader->faceAnim.blinkIntervalMax;
    fa.blinkInterval = mn + ((float)rand() / RAND_MAX) * (mx - mn);
    fa.blinkTimer    = fa.blinkInterval;
    fa.isTalking      = false;
}

void View::SetFaceState(const char *stateName, int id)
{
    if (!_infoReader->faceAnim.enabled) return;
    int idx = FindFaceState(stateName);
    FaceAnimState &fa = _viewData[id].faceAnim;
    if (idx == fa.stateIdx) return;
    fa.stateIdx   = idx;
    fa.frameIdx   = 0;
    fa.frameTimer = 0.0f;
}

void View::UpdateFaceAnim(int id, float dt)
{
    if (!_infoReader->faceAnim.enabled) return;
    FACE_ANIM_CONFIG &cfg = _infoReader->faceAnim;
    FaceAnimState    &fa  = _viewData[id].faceAnim;

    // Auto parpadeo — funciona tanto en idle como en talk
    if (cfg.autoBlink) {
        fa.blinkTimer -= dt;
        if (fa.blinkTimer <= 0.0f) {
            // Solo parpadear si estamos en el estado base (idle o talk)
            if (fa.stateIdx == fa.returnStateIdx) {
                // Elegir el tipo de parpadeo según si habla o no
                const char *blinkName = fa.isTalking ? "blink_talk" : "blink";
                int bi = FindFaceState(blinkName);
                // Si no existe blink_talk, usar blink normal
                if (bi == fa.returnStateIdx && fa.isTalking)
                    bi = FindFaceState("blink");
                if (bi != fa.returnStateIdx) {
                    fa.stateIdx = bi; fa.frameIdx = 0; fa.frameTimer = 0.0f;
                }
            }
            float mn = cfg.blinkIntervalMin;
            float mx = cfg.blinkIntervalMax;
            fa.blinkInterval = mn + ((float)rand() / RAND_MAX) * (mx - mn);
            fa.blinkTimer = fa.blinkInterval;
        }
    }

    if (fa.stateIdx >= cfg.stateCount) return;
    FACE_ANIM_STATE &st = cfg.states[fa.stateIdx];
    if (st.frameCount == 0) return;
    fa.frameTimer += dt;
    if (fa.frameTimer >= st.frameDuration) {
        fa.frameTimer -= st.frameDuration;
        fa.frameIdx++;
        if (fa.frameIdx >= st.frameCount) {
            fa.frameIdx = 0;
            if (!st.loop) fa.stateIdx = fa.returnStateIdx;
        }
    }
}

void View::RenderFaceAnim(int id)
{
    if (!_infoReader->faceAnim.enabled) return;
    if (!_viewData[id].faceFramesLoaded) return;
    FaceAnimState &fa = _viewData[id].faceAnim;
    if (fa.stateIdx >= MAXFACESTATES || fa.frameIdx >= MAXFACEFRAMES) return;
    Sprite *s = _viewData[id].faceFrames[fa.stateIdx][fa.frameIdx];
    if (s) s->Render(id);
}

// ─── Skins ────────────────────────────────────────────────────────────────────

// FIX 2: detección de flanco — solo hace toggle en el primer frame que
// se detecta la tecla presionada, no en cada frame del render.
void View::CheckSkinHotkeys(int id)
{
    for (int s = 0; s < _infoReader->skins.count; s++) {
        SKIN_INFO &sk = _infoReader->skins.skins[s];
        if (!sk.hotkey || sk.hotkey[0] == '\0') continue;

        bool pressed = false;
        for (int k = 0; k < KeyAmount; k++) {
            if (KeyDefine[k] && strcmp(KeyDefine[k], sk.hotkey) == 0) {
                pressed = eventManager->GetKeySignal(k);
                break;
            }
        }

        // Flanco de subida: solo actuar cuando pasa de false → true
        if (pressed && !_skinKeyWasPressed[s]) {
            ActiveSkin &ac = _viewData[id].activeSkin;
            if (ac.active && ac.skinIdx == s)
                DeactivateSkin(id);
            else
                ActivateSkin(s, id);
        }
        _skinKeyWasPressed[s] = pressed;
    }
}

void View::UpdateSkinTimer(int id, float dt)
{
    ActiveSkin &ac = _viewData[id].activeSkin;
    if (ac.active && ac.timeRemaining > 0.0f) {
        ac.timeRemaining -= dt;
        if (ac.timeRemaining <= 0.0f) DeactivateSkin(id);
    }
}

void View::ActivateSkin(int skinIdx, int id)
{
    ActiveSkin &ac  = _viewData[id].activeSkin;
    ac.skinIdx      = skinIdx;
    ac.active       = true;
    float dur = _infoReader->skins.skins[skinIdx].duration;
    ac.timeRemaining = (dur > 0.0f) ? dur : 0.0f;
}

void View::DeactivateSkin(int id)
{
    _viewData[id].activeSkin.active = false;
}

void View::OnChannelPointReward(const char *skinId, int id)
{
    for (int s = 0; s < _infoReader->skins.count; s++) {
        if (_infoReader->skins.skins[s].id &&
            strcmp(_infoReader->skins.skins[s].id, skinId) == 0) {
            ActiveSkin &ac = _viewData[id].activeSkin;
            if (ac.active && ac.skinIdx == s) DeactivateSkin(id);
            else                               ActivateSkin(s, id);
            return;
        }
    }
}

void View::RenderSkinsByZOrder(int id, int zOrder)
{
    ActiveSkin &ac = _viewData[id].activeSkin;
    if (!ac.active) return;
    if (_infoReader->skins.skins[ac.skinIdx].zOrder != zOrder) return;
    Sprite *s = _viewData[id].skinSprites[ac.skinIdx];
    if (s) s->Render(id);
}

// ─── Render principal ─────────────────────────────────────────────────────────
//
// FIX 1: face_anim siempre se incluye en el LayerOrder por defecto si
//         faceAnim.enabled es true, aunque el modo no lo defina.
// Los modos que SÍ definen LayerOrder lo usan tal cual.
// Los modos que NO definen LayerOrder usan el orden original + face_anim
// se inserta automáticamente después de "cat" si está habilitado.
//
void View::Render(int id)
{
    UpdataViewData(id);

    double now = glfwGetTime();
    float  dt  = (float)(now - _lastRenderTime);
    _lastRenderTime = now;
    if (dt > 0.1f) dt = 0.1f;
    if (dt < 0.0f) dt = 0.0f;

    // FIX 1: siempre actualizar animación facial si está habilitada,
    // independientemente del modo activo
    UpdateFaceAnim(id, dt);

    CheckSkinHotkeys(id);
    UpdateSkinTimer(id, dt);

    MODE_INFO &mi = _infoReader->_modeInfo[_mod];
    bool leftup = false, righttup = false, handsRendered = false;
    bool faceAnimRendered = false;

    for (int i = 0; i < mi.LayerCount; i++) {
        const char *layer = mi.LayerOrder[i];
        if (!layer) continue;

        if      (strcmp(layer, "background") == 0) RenderBackgroud(id);
        else if (strcmp(layer, "cat")        == 0) {
            RenderCat(id);
            // FIX 1: si face_anim está habilitado pero NO está en LayerOrder,
            // lo renderizamos justo después de "cat" automáticamente
            if (_infoReader->faceAnim.enabled) {
                bool inOrder = false;
                for (int j = 0; j < mi.LayerCount; j++)
                    if (mi.LayerOrder[j] && strcmp(mi.LayerOrder[j], "face_anim") == 0)
                        { inOrder = true; break; }
                if (!inOrder) { RenderFaceAnim(id); faceAnimRendered = true; }
            }
        }
        else if (strcmp(layer, "keys")       == 0) RenderKeys(id);
        else if (strcmp(layer, "left_hand")  == 0) { leftup   = RenderLeftHands(id);  handsRendered = true; }
        else if (strcmp(layer, "right_hand") == 0) { righttup = RenderRightHands(id); handsRendered = true; }
        else if (strcmp(layer, "mask")       == 0) ReanderMask(id);
        else if (strcmp(layer, "face_anim")  == 0) { RenderFaceAnim(id); faceAnimRendered = true; }
        else if (strcmp(layer, "skin_back")  == 0) RenderSkinsByZOrder(id, 0);
        else if (strcmp(layer, "skin_front") == 0) RenderSkinsByZOrder(id, 1);
        else if (strcmp(layer, "skin_over")  == 0) RenderSkinsByZOrder(id, 2);
        else if (strcmp(layer, "up_hands")   == 0) {
            if (!handsRendered) {
                leftup   = RenderLeftHands(id);
                righttup = RenderRightHands(id);
            }
            RenderUphands(leftup, righttup, id);
        }
    }
}

// ─── InitializeSpirite ───────────────────────────────────────────────────────

void View::InitializeSpirite(int id)
{
    _programId = VtuberDelegate::GetInstance()->CreateShader();

    int width  = VtuberDelegate::GetInstance()->getBufferWidth(id);
    int height = VtuberDelegate::GetInstance()->getBufferHeight(id);
    LAppTextureManager *tm = VtuberDelegate::GetInstance()->GetTextureManager();

    const string resourcesPath = ResourcesPath;
    const string modePath      = resourcesPath + ModePath;
    const string maskPath      = resourcesPath + MaskPath;
    const string skinsPath     = resourcesPath + "skins/";

    auto MakeSprite = [&](const string &path) -> Sprite * {
        LAppTextureManager::TextureInfo *tex = tm->CreateTextureFromPngFile(path);
        if (!tex) return nullptr;
        float x = width * 0.5f, y = height * 0.5f;
        float fw = static_cast<float>(width * 0.5), fh = static_cast<float>(height * 0.5);
        return new Sprite(x, y, fw, fh, tex->id, _programId);
    };

    // Máscara original
    _viewData[id]._faceCount  = _infoReader->_faceInfo->Facecount;
    _viewData[id]._curentface = 0;
    for (int fc = 0; fc < _infoReader->_faceInfo->Facecount; fc++) {
        string img = _infoReader->_faceInfo->FaceImages[fc];
        _viewData[id]._face[fc] = MakeSprite(maskPath + img);
    }

    // Sprites de animación facial
    _viewData[id].faceFramesLoaded = false;
    if (_infoReader->faceAnim.enabled) {
        for (int s = 0; s < _infoReader->faceAnim.stateCount; s++) {
            FACE_ANIM_STATE &st = _infoReader->faceAnim.states[s];
            for (int f = 0; f < st.frameCount; f++) {
                _viewData[id].faceFrames[s][f] = (st.frames[f] && st.frames[f][0])
                    ? MakeSprite(maskPath + st.frames[f]) : nullptr;
            }
        }
        _viewData[id].faceFramesLoaded = true;
    }

    // Sprites de skins
    for (int s = 0; s < _infoReader->skins.count; s++) {
        SKIN_INFO &sk = _infoReader->skins.skins[s];
        _viewData[id].skinSprites[s] = (sk.imagePath && sk.imagePath[0])
            ? MakeSprite(skinsPath + sk.imagePath) : nullptr;
    }

    // Modos (original)
    _viewData[id]._modecount = _infoReader->ModeCount;
    for (int m = 0; m < _infoReader->ModeCount; m++) {
        MODE_INFO *mi = _infoReader->_modeInfo;
        string tp = modePath + _infoReader->ModePath[m] + "/";

        _viewData[id]._mode[m]._back    = (mi[m].BackgroundImageName && strcmp(mi[m].BackgroundImageName, "") != 0)
            ? MakeSprite(tp + mi[m].BackgroundImageName) : NULL;
        _viewData[id]._mode[m]._catback = (mi[m].CatBackgroundImageName && strcmp(mi[m].CatBackgroundImageName, "") != 0)
            ? MakeSprite(tp + mi[m].CatBackgroundImageName) : NULL;

        string kp = tp + mi[m].KeysImagePath + "/";
        _viewData[id]._mode[m]._keysCount = mi[m].KeysCount;
        for (int i = 0; i < mi[m].KeysCount; i++) {
            if (!mi[m].KeysImageName[i]) break;
            _viewData[id]._mode[m]._keys[i] = MakeSprite(kp + mi[m].KeysImageName[i]);
        }

        if (mi[m].LeftHandImagePath && strcmp(mi[m].LeftHandImagePath, "") != 0) {
            string lp = tp + mi[m].LeftHandImagePath + "/";
            _viewData[id]._mode[m]._leftHandUp = (mi[m].LeftHandUpImageName && strcmp(mi[m].LeftHandUpImageName, "") != 0)
                ? MakeSprite(lp + mi[m].LeftHandUpImageName) : NULL;
            _viewData[id]._mode[m]._leftHandsCount = mi[m].ModelLeftHandCount;
            for (int i = 0; i < mi[m].ModelLeftHandCount; i++)
                _viewData[id]._mode[m]._leftHands[i] = MakeSprite(lp + mi[m].LeftHandImageName[i]);
        } else {
            _viewData[id]._mode[m]._leftHandUp     = NULL;
            _viewData[id]._mode[m]._leftHandsCount = 0;
        }

        if (mi[m].RightHandImagePath && strcmp(mi[m].RightHandImagePath, "") != 0) {
            string rp = tp + mi[m].RightHandImagePath + "/";
            _viewData[id]._mode[m]._rightHandUp = (mi[m].RightHandUpImageName && strcmp(mi[m].RightHandUpImageName, "") != 0)
                ? MakeSprite(rp + mi[m].RightHandUpImageName) : NULL;
            _viewData[id]._mode[m]._rightHandsCount = mi[m].ModelRightHandCount;
            for (int i = 0; i < mi[m].ModelRightHandCount; i++)
                _viewData[id]._mode[m]._rightHands[i] = MakeSprite(rp + mi[m].RightHandImageName[i]);
        } else {
            _viewData[id]._mode[m]._rightHandUp     = NULL;
            _viewData[id]._mode[m]._rightHandsCount = 0;
        }
    }
}

// ─── InitializeModel (original) ──────────────────────────────────────────────

void View::InitializeModel(int id)
{
    MODE_INFO    *mi  = _infoReader->_modeInfo;
    Live2DManager *mgr = Live2DManager::GetInstance();
    const string resourcesPath = ResourcesPath;
    const string modePath      = resourcesPath + ModePath;

    for (int m = 0; m < _infoReader->ModeCount; m++) {
        string tp = modePath + _infoReader->ModePath[m] + "/";
        _viewData[id]._mode[m]._haseModel = mi[m].HasModel;
        string cp = tp + mi[m].CatModelPath;
        string cn = Pal::GetModelName(cp.c_str());
        _viewData[id]._mode[m]._modelId = mgr->ChangeScene((cp + "/" + cn).c_str());

        _viewData[id]._mode[m]._hasLeftHandModel = mi[m].ModelHasLeftHandModel;
        string lp = tp + mi[m].ModelLeftHandModelPath;
        string ln = Pal::GetModelName(lp.c_str());
        _viewData[id]._mode[m]._leftHandModelId = mgr->ChangeScene((lp + "/" + ln).c_str());

        _viewData[id]._mode[m]._hasRightHandModel = mi[m].ModelHasRightHandModel;
        string rp = tp + mi[m].ModelRightHandModelPath;
        string rn = Pal::GetModelName(rp.c_str());
        _viewData[id]._mode[m]._rightHandModelId = mgr->ChangeScene((rp + "/" + rn).c_str());
    }
}

// ─── Resto de métodos (originales) ───────────────────────────────────────────

void View::PreModelDraw(Model &refModel)
{
    Csm::Rendering::CubismOffscreenFrame_OpenGLES2 *useTarget = NULL;
    if (target != SelectTarget_None) {
        useTarget = (target == SelectTarget_ViewFrameBuffer)
                    ? &_renderBuffer : &refModel.GetRenderBuffer();
        if (!useTarget->IsValid()) {
            int w = VtuberDelegate::GetInstance()->getBufferWidth(0);
            int h = VtuberDelegate::GetInstance()->getBufferHeight(0);
            if (w && h) useTarget->CreateOffscreenFrame(
                static_cast<csmUint32>(w), static_cast<csmUint32>(h));
        }
        useTarget->BeginDraw();
        useTarget->Clear(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
    }
}

void View::PostModelDraw(Model &refModel)
{
    Csm::Rendering::CubismOffscreenFrame_OpenGLES2 *useTarget = NULL;
    if (target != SelectTarget_None) {
        useTarget = (target == SelectTarget_ViewFrameBuffer)
                    ? &_renderBuffer : &refModel.GetRenderBuffer();
        useTarget->EndDraw();
    }
}

void View::SwitchRenderingTarget(SelectTarget t, int id) { target = t; }
void View::SetRenderTargetClearColor(float r, float g, float b)
    { _clearColor[0]=r; _clearColor[1]=g; _clearColor[2]=b; }

Csm::CubismViewMatrix *View::GetViewMatrix(int id)         { return _viewData[id]._viewMatrix; }
Csm::CubismMatrix44   *View::GetDeviceToScreenMatrix(int id){ return _viewData[id]._deviceToScreen; }
InfoReader            *View::GetInfoReader()               { return _infoReader; }

float View::TransformViewX(float deviceX, int id) const {
    return _viewData[id]._viewMatrix->InvertTransformX(
           _viewData[id]._deviceToScreen->TransformX(deviceX));
}
float View::TransformViewY(float deviceY, int id) const {
    return _viewData[id]._viewMatrix->InvertTransformY(
           _viewData[id]._deviceToScreen->TransformY(deviceY));
}

EventManager *View::GetEventManager() { return eventManager; }

void View::OnMouseMoved(float pointX, float pointY, int id) const
{
    float viewX = this->TransformViewX(eventManager->GetX(), id);
    float viewY = this->TransformViewY(eventManager->GetY(), id);
    eventManager->MouseEventMoved(pointX, pointY);
}

void View::Update(bool _isLive2D, bool _isUseMask) {
    isUseLive2d = _isLive2D;
    isUseMask   = _isUseMask;
}

void View::setMod(uint16_t i) { _mod = i; }

// ─── SetFaceStateTalk / SetFaceStateIdle ─────────────────────────────────────
// Separados de SetFaceState para manejar correctamente el parpadeo durante talk.

void View::SetFaceStateTalk(int id)
{
    if (!_infoReader->faceAnim.enabled) return;
    FaceAnimState &fa = _viewData[id].faceAnim;

    fa.isTalking = true;

    // Si estamos en idle o blink (sin hablar), cambiar a talk
    int talkIdx  = FindFaceState("talk");
    int idleIdx  = FindFaceState("idle");
    int blinkIdx = FindFaceState("blink");

    if (fa.stateIdx == idleIdx || fa.stateIdx == blinkIdx) {
        fa.stateIdx       = talkIdx;
        fa.returnStateIdx = talkIdx;  // parpadeo durante talk vuelve aquí
        fa.frameIdx       = 0;
        fa.frameTimer     = 0.0f;
    }
    // Si ya estamos en talk o blink_talk, no interrumpir
}

void View::SetFaceStateIdle(int id)
{
    if (!_infoReader->faceAnim.enabled) return;
    FaceAnimState &fa = _viewData[id].faceAnim;

    if (!fa.isTalking) return; // ya estaba en idle, no hacer nada
    fa.isTalking = false;

    int idleIdx      = FindFaceState("idle");
    int blinkTalkIdx = FindFaceState("blink_talk");
    int talkIdx      = FindFaceState("talk");

    // Solo cambiar si estábamos en talk o blink_talk
    if (fa.stateIdx == talkIdx || fa.stateIdx == blinkTalkIdx) {
        fa.stateIdx       = idleIdx;
        fa.returnStateIdx = idleIdx;  // parpadeo normal vuelve a idle
        fa.frameIdx       = 0;
        fa.frameTimer     = 0.0f;
    }
}