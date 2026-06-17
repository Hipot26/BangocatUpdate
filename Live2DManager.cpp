#include "Live2DManager.hpp"
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Rendering/CubismRenderer.hpp>
#include "Pal.hpp"
#include "Define.hpp"
#include "VtuberDelegate.hpp"
#include "Model.hpp"
#include "View.hpp"
#include "EventManager.hpp"

using namespace Csm;
using namespace Define;
using namespace std;

namespace {
    Live2DManager *s_instance = NULL;
}

Live2DManager *Live2DManager::GetInstance()
{
    if (s_instance == NULL)
        s_instance = new Live2DManager();
    return s_instance;
}

void Live2DManager::ReleaseInstance()
{
    if (s_instance != NULL)
        delete s_instance;
    s_instance = NULL;
}

Live2DManager::Live2DManager()
    : _modelCounter(0), _isUseRelativeMouse(false)
{}

Live2DManager::~Live2DManager()
{
    for (size_t i = 0; i < MAXMODELDATA; i++)
        ReleaseAllModel(i);
}

void Live2DManager::ReleaseAllModel(csmUint16 id)
{
    for (csmUint32 i = 0; i < _modeldata[id]._models.GetSize(); i++)
        delete _modeldata[id]._models[i];
    _modeldata[id]._models.Clear();
    _modeldata[id]._modelPath.Clear();
}

Model *Live2DManager::GetModel(csmUint16 id) const
{
    if (0 < _modeldata[id]._models.GetSize())
        return _modeldata[id]._models[0];
    return NULL;
}

// MODIFICADO: offsetY desplaza el modelo Live2D en el eje Y antes de dibujarlo.
// offsetY > 0 = sube, offsetY < 0 = baja.
// Llamado desde View::RenderRightHands con el valor de MODE_INFO::ModelRightHandOffsetY
void Live2DManager::OnUpdate(csmUint16 id, float offsetY) const
{
    CubismMatrix44 projection;

    // Posición del ratón
    EventManager *em = VtuberDelegate::GetInstance()->GetView()->GetEventManager();

    int width, height;
    Pal::GetDesktopResolution(width, height);
    int px, py;

    if (_isUseRelativeMouse) {
        VtuberDelegate::GetInstance()->GetView()->GetEventManager()
            ->GetRelativeMouse(px, py);
        em->MouseEventMoved(width, height, px, py);
        px = em->GetCenterX() - em->GetStartX() + width / 2;
        py = em->GetCenterY() - em->GetStartY() + height / 2;
    } else {
        VtuberDelegate::GetInstance()->GetView()->GetEventManager()
            ->GetCurrentMousePosition(px, py);
    }

    float mousex = static_cast<float>(px) / static_cast<float>(width);
    float mousey = static_cast<float>(py) / static_cast<float>(height);

    // Matriz de vista
    CubismMatrix44 *_viewMatrix =
        VtuberDelegate::GetInstance()->GetView()->GetViewMatrix(0);
    if (_viewMatrix != NULL)
        projection.MultiplyByMatrix(_viewMatrix);

    // ── NUEVO: aplicar offset Y antes de dibujar ──────────────────────────────
    // Esto traslada SOLO este modelo, sin afectar nada más.
    if (offsetY != 0.0f)
        projection.TranslateY(offsetY);

    Model *model = GetModel(id);
    if (model) {
        VtuberDelegate::GetInstance()->GetView()->PreModelDraw(*model);

        model->UpdateTime();
        model->UpdataSetting(_randomMotion, _delayTime, _isBreath, _isEyeBlink,
                             _isTrack, _isMouseHorizontalFlip, _IsMouseVerticalFlip);
        model->UpdateMouseState(mousex, mousey,
                                em->GetLeftButton(), em->GetRightButton());
        model->Update(id);
        model->Draw(projection);

        VtuberDelegate::GetInstance()->GetView()->PostModelDraw(*model);
    }
}

csmInt32 Live2DManager::ChangeScene(const csmChar *_modelPath)
{
    int id = _modelCounter;
    if (strcmp(_modelPath, "") == 0)
        return -1;

    if (strcmp(_modeldata[id]._modelPath.GetRawString(), _modelPath) == 0)
        return id;

    string modelFilePath = std::string(_modelPath);
    size_t pos           = modelFilePath.rfind("/");
    string modelPath     = modelFilePath.substr(0, pos + 1);
    string modelJsonName = modelFilePath.substr(pos + 1, modelFilePath.size() - pos - 1);

    if (strcmp(modelJsonName.c_str(), "") == 0)
        return -1;

    if (VtuberDelegate::GetInstance()->isLoadResource(id))
        ReleaseAllModel(id);

    _modeldata[id]._models.PushBack(new Model());

    if (_modeldata[id]._models[0]->LoadAssets(modelPath.c_str(), modelJsonName.c_str())) {
        _modelCounter++;
        _modeldata[id]._modelPath = _modelPath;
    } else {
        _modeldata[id]._models.Clear();
        return -1;
    }

    SelectTarget useRenderTarget = SelectTarget_None;
    VtuberDelegate::GetInstance()->GetView()->SwitchRenderingTarget(useRenderTarget, id);

    float clearColor[3] = {1.0f, 1.0f, 1.0f};
    VtuberDelegate::GetInstance()->GetView()->SetRenderTargetClearColor(
        clearColor[0], clearColor[1], clearColor[2]);

    return id;
}

void Live2DManager::ChangeMouseMovement(csmBool _mouse)
{
    if (_isUseRelativeMouse != _mouse) {
        _isUseRelativeMouse = _mouse;
        if (_mouse) {
            EventManager *em =
                VtuberDelegate::GetInstance()->GetView()->GetEventManager();
            int px, py;
            VtuberDelegate::GetInstance()->GetView()->GetEventManager()
                ->GetRelativeMouse(px, py);
            em->MouseEventBegan(px, py);
        }
    }
}

void Live2DManager::UpdateModelSetting(csmBool randomMotion, csmFloat32 delayTime,
                                        csmBool isBreath, csmBool isEyeBlink,
                                        csmBool isTrack, csmBool isMouseHorizontalFlip,
                                        csmBool IsMouseVerticalFlip)
{
    _randomMotion          = randomMotion;
    _delayTime             = delayTime;
    _isBreath              = isBreath;
    _isEyeBlink            = isEyeBlink;
    _isTrack               = isTrack;
    _isMouseHorizontalFlip = isMouseHorizontalFlip;
    _IsMouseVerticalFlip   = IsMouseVerticalFlip;
}