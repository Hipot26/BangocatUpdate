#include "Define.hpp"
#include <CubismFramework.hpp>

namespace Define {
    using namespace Csm;

    const csmFloat32 ViewMaxScale       = 10.0f;
    const csmFloat32 ViewMinScale       = 0.8f;
    const csmFloat32 ViewLogicalLeft    = -1.0f;
    const csmFloat32 ViewLogicalRight   =  1.0f;
    const csmFloat32 ViewLogicalMaxLeft    = -2.0f;
    const csmFloat32 ViewLogicalMaxRight   =  2.0f;
    const csmFloat32 ViewLogicalMaxBottom  = -2.0f;
    const csmFloat32 ViewLogicalMaxTop     =  2.0f;

    const csmInt32 PriorityNone   = 0;
    const csmInt32 PriorityIdle   = 1;
    const csmInt32 PriorityNormal = 2;
    const csmInt32 PriorityForce  = 3;

    const CubismFramework::Option::LogLevel CubismLoggingLevel =
        CubismFramework::Option::LogLevel_Verbose;
    const csmBool DebugMod = false;

    const csmInt32 RenderTargetWidth  = 1280;
    const csmInt32 RenderTargetHeight = 768;

    const csmChar *ParaMouseX       = "ParamMouseX";
    const csmChar *ParaMouseY       = "ParamMouseY";
    const csmChar *ParaLeftButton   = "ParamMouseLeftDown";
    const csmChar *ParaRightButton  = "ParamMouseRightDown";

    // ── Teclas simples (original) ─────────────────────────────────────────────
    const csmInt32 KeyAmount = 73;
    const csmChar *KeyDefine[] = {
        // a-z (0-25)
        "a","b","c","d","e","f","g","h","i","j","k","l",
        "m","n","o","p","q","r","s","t","u","v","w","x","y","z",
        // 0-9 (26-35)
        "0","1","2","3","4","5","6","7","8","9",
        // especiales (36-38)
        "space","lshift","lctrl",
        // F1-F12 (39-50)
        "f1","f2","f3","f4","f5","f6","f7","f8","f9","f10","f11","f12",
        // flechas (51-54)
        "up","down","left","right",
        // otros (55-58)
        "<",">","[","]",
        // shift/ctrl derechos (59-60)
        "rshift","rctrl",
        // NUEVO: modificadores adicionales (61-64)
        "lalt","ralt","tab","esc",
        // NUEVO: numpad (65-72)
        "num0","num1","num2","num3","num4","num5","num6","num7","num8"
    };

    const csmChar *ResourcesPath = "Bango Cat/";
    const csmChar *ModePath      = "mode/";
    const csmChar *MaskPath      = "face/";
    const csmChar *KPSPath       = "kps/";
}