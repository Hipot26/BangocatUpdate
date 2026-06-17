#include "InfoReader.hpp"
#include "json.h"
#include "Pal.hpp"
#include "Define.hpp"
#include <string>
#include <cstring>
#include "CubismSdk/Framework/src/Type/CubismBasicType.hpp"

using namespace std;
using namespace Json;
using namespace Live2D::Cubism::Framework;

namespace {

csmByte *CreateBuffer(const csmChar *path, csmSizeInt *size)
    { return Pal::LoadFileAsBytes(path, size); }

void DeleteBuffer(csmByte *buffer, const csmChar *path = "")
    { Pal::ReleaseBytes(buffer); }

char *CreateParam(string _param) {
    char *buf = new char[_param.size() + 1];
    buf[_param.size()] = 0x00;
    memcpy(buf, _param.c_str(), _param.size());
    return buf;
}

// Orden original del plugin — usado si LayerOrder no está en el JSON
static const char *DEFAULT_LAYERS[] = {
    "background", "keys", "cat", "left_hand", "right_hand", "mask", "up_hands"
};
static const int DEFAULT_LAYER_COUNT = 7;

bool ParseJson(const char *path, Value &root) {
    csmSizeInt size;
    csmByte *buf = CreateBuffer(path, &size);
    if (!buf || size == 0) { DeleteBuffer(buf); return false; }
    JSONCPP_STRING err;
    CharReaderBuilder builder;
    const std::unique_ptr<CharReader> reader(builder.newCharReader());
    bool ok = reader->parse((char *)buf, (char *)buf + size, &root, &err);
    DeleteBuffer(buf);
    return ok;
}

} // namespace

// ─── Constructor / Destructor ─────────────────────────────────────────────────

InfoReader::InfoReader() : ModeCount(0)
{
    _modeInfo = new MODE_INFO[MAXMODCOUNT];
    _faceInfo = new FACE_INFO[1];
    _kpsInfo  = new KPS_INFO[1];
    memset(&faceAnim, 0, sizeof(faceAnim));
    memset(&skins,    0, sizeof(skins));
}

InfoReader::~InfoReader()
{
    delete[] _modeInfo;
    delete   _faceInfo;
    delete   _kpsInfo;
    for (int i = 0; i < ModeCount; i++) delete ModePath[i];
    ModeCount = 0;
}

// ─── InitFromConfig (original, sin cambios) ───────────────────────────────────

bool InfoReader::InitFromConfig(const char *filePath)
{
    string _filePath = string(filePath);
    string _fileConf = _filePath + "config.json";
    Value root;
    if (!ParseJson(_fileConf.c_str(), root)) return false;

    int index = 0;
    auto _modelPath = root["ModelPath"];
    for (auto entry = _modelPath.begin(); entry != _modelPath.end(); entry++) {
        string _modeName = entry->asCString();
        string modePath  = _filePath + _modeName;
        if (ModeFromConfig(modePath.c_str(), index)) {
            ModePath[index] = CreateParam(_modeName);
            index++;
        }
    }
    ModeCount = index;
    return true;
}

// ─── ModeFromConfig — original + LayerOrder ───────────────────────────────────

bool InfoReader::ModeFromConfig(const char *filePath, int _modeid)
{
    string _filePath = string(filePath) + "/config.json";
    Value root;
    if (!ParseJson(_filePath.c_str(), root)) return false;

    // ── Campos originales (idénticos al original) ─────────────────────────────
    _modeInfo[_modeid].BackgroundImageName    = CreateParam(root["BackgroundImageName"].asCString());
    _modeInfo[_modeid].CatBackgroundImageName = CreateParam(root["CatBackgroundImageName"].asCString());
    _modeInfo[_modeid].HasModel     = root["HasModel"].asBool();
    _modeInfo[_modeid].CatModelPath = CreateParam(root["CatModelPath"].asCString());
    _modeInfo[_modeid].KeysImagePath = CreateParam(root["KeysImagePath"].asCString());

    int index = 0;
    for (auto &e : root["KeysImageName"])
        _modeInfo[_modeid].KeysImageName[index++] = CreateParam(e.asCString());
    int kindex = 0;
    for (auto &e : root["KeyUse"])
        _modeInfo[_modeid].KeyUse[kindex++] = CreateParam(e.asCString());
    _modeInfo[_modeid].KeysCount = (index > kindex) ? kindex : index;

    _modeInfo[_modeid].ModelHasLeftHandModel  = root["ModelHasLeftHandModel"].asBool();
    _modeInfo[_modeid].ModelLeftHandModelPath = CreateParam(root["ModelLeftHandModelPath"].asCString());
    _modeInfo[_modeid].LeftHandImagePath      = CreateParam(root["LeftHandImagePath"].asCString());
    _modeInfo[_modeid].LeftHandUpImageName    = CreateParam(root["LeftHandUpImageName"].asCString());
    index = 0;
    for (auto &e : root["LeftHandImageName"])
        _modeInfo[_modeid].LeftHandImageName[index++] = CreateParam(e.asCString());
    _modeInfo[_modeid].ModelLeftHandCount = index;

    _modeInfo[_modeid].ModelHasRightHandModel  = root["ModelHasRightHandModel"].asBool();
    _modeInfo[_modeid].ModelRightHandModelPath = CreateParam(root["ModelRightHandModelPath"].asCString());
    _modeInfo[_modeid].RightHandImagePath      = CreateParam(root["RightHandImagePath"].asCString());
    _modeInfo[_modeid].RightHandUpImageName    = CreateParam(root["RightHandUpImageName"].asCString());
    index = 0;
    for (auto &e : root["RightHandImageName"])
        _modeInfo[_modeid].RightHandImageName[index++] = CreateParam(e.asCString());
    _modeInfo[_modeid].ModelRightHandCount = index;

    // Offset Y para modelo Live2D mano derecha (0.0 por defecto = sin offset)
    _modeInfo[_modeid].ModelRightHandOffsetY = root.isMember("ModelRightHandOffsetY")
        ? root["ModelRightHandOffsetY"].asFloat() : 0.0f;

    // ── NUEVO: LayerOrder ─────────────────────────────────────────────────────
    if (root.isMember("LayerOrder") && root["LayerOrder"].isArray()) {
        int li = 0;
        for (auto &e : root["LayerOrder"]) {
            if (li >= MAXLAYERCOUNT) break;
            _modeInfo[_modeid].LayerOrder[li++] = CreateParam(e.asString());
        }
        _modeInfo[_modeid].LayerCount = li;
    } else {
        // Retrocompatible: orden original si no hay LayerOrder en el JSON
        for (int i = 0; i < DEFAULT_LAYER_COUNT; i++)
            _modeInfo[_modeid].LayerOrder[i] = CreateParam(string(DEFAULT_LAYERS[i]));
        _modeInfo[_modeid].LayerCount = DEFAULT_LAYER_COUNT;
    }

    return true;
}

// ─── InitFaceFromConfig (original, sin cambios) ───────────────────────────────

bool InfoReader::InitFaceFromConfig(const char *filePath)
{
    string _filePath = string(filePath) + "config.json";
    Value root;
    if (!ParseJson(_filePath.c_str(), root)) return false;

    int findex = 0;
    for (auto &e : root["HotKey"])
        _faceInfo[0].HotKey[findex++] = CreateParam(e.asCString());
    int index = 0;
    for (auto &e : root["FaceImageName"])
        _faceInfo[0].FaceImages[index++] = CreateParam(e.asCString());
    _faceInfo[0].Facecount = (findex > index) ? index : findex;
    return true;
}

// ─── InitKpsFromConfig (original, sin cambios) ────────────────────────────────

bool InfoReader::InitKpsFromConfig(const char *filePath)
{
    string _filePath = string(filePath) + "config.json";
    Value root;
    if (!ParseJson(_filePath.c_str(), root)) return false;

    int index = 0;
    for (auto &e : root["Numbers"])
        _kpsInfo[0].KPSImages[index++] = CreateParam(e.asCString());
    _kpsInfo[0].KpsCount = index;
    return true;
}

// ─── NUEVO: InitFaceAnimConfig ────────────────────────────────────────────────
// Lee face/config.json para la animación facial por sprites.

bool InfoReader::InitFaceAnimConfig(const char *facePath)
{
    string _filePath = string(facePath) + "config.json";
    Value root;
    if (!ParseJson(_filePath.c_str(), root)) { faceAnim.enabled = false; return false; }

    faceAnim.enabled = root.isMember("enabled") ? root["enabled"].asBool() : false;
    if (!faceAnim.enabled) return false;

    faceAnim.idleState        = CreateParam(root.isMember("idleState") ? root["idleState"].asString() : "idle");
    faceAnim.autoBlink        = root.isMember("autoBlink")        ? root["autoBlink"].asBool()        : true;
    faceAnim.blinkIntervalMin = root.isMember("blinkIntervalMin") ? root["blinkIntervalMin"].asFloat() : 2.5f;
    faceAnim.blinkIntervalMax = root.isMember("blinkIntervalMax") ? root["blinkIntervalMax"].asFloat() : 6.0f;

    int si = 0;
    for (auto &st : root["states"]) {
        if (si >= MAXFACESTATES) break;
        FACE_ANIM_STATE &fs = faceAnim.states[si];
        fs.name          = CreateParam(st.isMember("name")          ? st["name"].asString()          : "");
        fs.frameDuration = st.isMember("frameDuration") ? st["frameDuration"].asFloat() : 0.1f;
        fs.loop          = st.isMember("loop")          ? st["loop"].asBool()           : true;
        int fi = 0;
        for (auto &fr : st["frames"]) {
            if (fi >= MAXFACEFRAMES) break;
            fs.frames[fi++] = CreateParam(fr.asString());
        }
        fs.frameCount = fi;
        si++;
    }
    faceAnim.stateCount = si;
    return true;
}

// ─── NUEVO: InitSkinsConfig ───────────────────────────────────────────────────
// Lee skins/config.json. Define los accesorios disponibles con hotkey toggle
// y/o puntos de canal.

bool InfoReader::InitSkinsConfig(const char *skinsPath)
{
    string _filePath = string(skinsPath) + "config.json";
    Value root;
    if (!ParseJson(_filePath.c_str(), root)) { skins.count = 0; return false; }

    int idx = 0;
    for (auto &sk : root["skins"]) {
        if (idx >= MAXSKINS) break;
        SKIN_INFO &s   = skins.skins[idx];
        s.id           = CreateParam(sk.isMember("id")            ? sk["id"].asString()            : "");
        s.imagePath    = CreateParam(sk.isMember("image")         ? sk["image"].asString()         : "");
        s.hotkey       = CreateParam(sk.isMember("hotkey")        ? sk["hotkey"].asString()        : "");
        s.duration     = sk.isMember("duration")      ? sk["duration"].asFloat()      : 0.0f;
        s.channelPoints= sk.isMember("channelPoints") ? sk["channelPoints"].asInt()   : 0;

        // zOrder: "back"=0, "front"=1, "over_hands"=2
        string zo = sk.isMember("zOrder") ? sk["zOrder"].asString() : "front";
        if      (zo == "back")       s.zOrder = 0;
        else if (zo == "over_hands") s.zOrder = 2;
        else                         s.zOrder = 1;

        idx++;
    }
    skins.count = idx;
    return true;
}