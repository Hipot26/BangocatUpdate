#pragma once

#define MAXMODELDATA 1024

#include <CubismFramework.hpp>
#include <Math/CubismMatrix44.hpp>
#include <Math/CubismViewMatrix.hpp>
#include <Type/csmVector.hpp>

class Model;

class Live2DManager
{
    struct ModelData {
        Csm::csmVector<Model *> _models;
        Csm::csmString          _modelPath;
    };

public:
    static Live2DManager *GetInstance();
    static void           ReleaseInstance();

    Model        *GetModel(Csm::csmUint16 id) const;
    void          ReleaseAllModel(Csm::csmUint16 id);

    // MODIFICADO: añadido offsetY (0.0f = sin offset, default retrocompatible)
    void OnUpdate(Csm::csmUint16 id, float offsetY = 0.0f) const;

    Csm::csmInt32 ChangeScene(const Csm::csmChar *_modelPath);
    void          ChangeMouseMovement(Csm::csmBool _mouse);
    void          UpdateModelSetting(Csm::csmBool randomMotion,
                                     Csm::csmFloat32 delayTime,
                                     Csm::csmBool isBreath,
                                     Csm::csmBool isEyeBlink,
                                     Csm::csmBool isTrack,
                                     Csm::csmBool isMouseHorizontalFlip,
                                     Csm::csmBool IsMouseVerticalFlip);

private:
    Live2DManager();
    virtual ~Live2DManager();

    ModelData        _modeldata[MAXMODELDATA];
    Csm::csmInt32    _modelCounter;
    Csm::csmBool     _isUseRelativeMouse;
    Csm::csmBool     _randomMotion;
    Csm::csmFloat32  _delayTime;
    Csm::csmBool     _isBreath;
    Csm::csmBool     _isEyeBlink;
    Csm::csmBool     _isTrack;
    Csm::csmBool     _isMouseHorizontalFlip;
    Csm::csmBool     _IsMouseVerticalFlip;
};