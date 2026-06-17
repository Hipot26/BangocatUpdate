#pragma once

#define MAXMODCOUNT   10
#define MAXKEYCOUNT  128
#define MAXEFACECOUNT 64
#define MAXKPSNUMBERS 10

// ── Nuevos límites ────────────────────────────────────────────────────────────
#define MAXLAYERCOUNT  16
#define MAXFACEFRAMES  16
#define MAXFACESTATES   8
#define MAXSKINS       16

// ─── Animación facial ─────────────────────────────────────────────────────────
struct FACE_ANIM_STATE {
    char  *name;
    char  *frames[MAXFACEFRAMES];
    int    frameCount;
    float  frameDuration;
    bool   loop;
};

struct FACE_ANIM_CONFIG {
    bool            enabled;
    char           *idleState;
    FACE_ANIM_STATE states[MAXFACESTATES];
    int             stateCount;
    bool            autoBlink;
    float           blinkIntervalMin;
    float           blinkIntervalMax;
};

// ─── Skin/accesorio ───────────────────────────────────────────────────────────
// Toggle con hotkey o puntos de canal.
// zOrder: 0=debajo del gato, 1=encima del gato, 2=encima de manos
struct SKIN_INFO {
    char  *id;            // identificador único, ej: "halloween"
    char  *imagePath;     // relativa a skins/, ej: "halloween/hat.png"
    char  *hotkey;        // tecla toggle, ej: "f5" (vacío=solo canal)
    int    zOrder;        // 0=skin_back, 1=skin_front, 2=skin_over
    float  duration;      // segundos activo (0=permanente)
    int    channelPoints; // coste en puntos de canal (0=gratis)
};

struct SKINS_CONFIG {
    SKIN_INFO skins[MAXSKINS];
    int       count;
};

// ─── Modo (original + LayerOrder) ────────────────────────────────────────────
struct MODE_INFO {
    char *BackgroundImageName;
    char *CatBackgroundImageName;
    bool  HasModel;
    char *CatModelPath;
    char *KeysImagePath;
    int   KeysCount;
    char *KeysImageName[MAXKEYCOUNT];
    char *KeyUse[MAXKEYCOUNT];
    bool  ModelHasLeftHandModel;
    char *ModelLeftHandModelPath;
    int   ModelLeftHandCount;
    char *LeftHandImagePath;
    char *LeftHandUpImageName;
    char *LeftHandImageName[MAXKEYCOUNT];
    bool  ModelHasRightHandModel;
    char *ModelRightHandModelPath;
    int   ModelRightHandCount;
    char *RightHandImagePath;
    char *RightHandUpImageName;
    char *RightHandImageName[MAXKEYCOUNT];

    // Offset Y para el modelo Live2D de la mano derecha
    // Unidades Cubism: 1.0 = altura completa del canvas
    // Negativo = bajar, positivo = subir
    float ModelRightHandOffsetY;

    // NUEVO — orden Z configurable por JSON
    // Capas válidas: "background" "cat" "keys" "left_hand" "right_hand"
    //                "mask" "face_anim" "skin_back" "skin_front"
    //                "skin_over" "up_hands"
    // Sin LayerOrder en el JSON → orden original del plugin (retrocompatible)
    char *LayerOrder[MAXLAYERCOUNT];
    int   LayerCount;
};

struct FACE_INFO {
    char *HotKey[MAXEFACECOUNT];
    int   Facecount;
    char *FaceImages[MAXEFACECOUNT];
};

struct KPS_INFO {
    int   KpsCount;
    char *KPSImages[MAXKPSNUMBERS];
};

class InfoReader {
public:
    InfoReader();
    ~InfoReader();

    bool InitFromConfig(const char *filePath);
    bool InitFaceFromConfig(const char *filePath);
    bool InitKpsFromConfig(const char *filePath);
    bool InitFaceAnimConfig(const char *facePath);  // NUEVO
    bool InitSkinsConfig(const char *skinsPath);    // NUEVO

    int       ModeCount;
    char     *ModePath[MAXMODCOUNT];
    MODE_INFO       *_modeInfo;
    FACE_INFO       *_faceInfo;
    KPS_INFO        *_kpsInfo;
    FACE_ANIM_CONFIG faceAnim;  // NUEVO
    SKINS_CONFIG     skins;     // NUEVO

private:
    bool ModeFromConfig(const char *filePath, int _modeid);
};