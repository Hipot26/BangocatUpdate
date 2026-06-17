/**
  Original: Weng Y 2020/05/25
  Modified: WASAPI mic + animación estilo Veadotube + parpadeo durante talk
*/
#include "VtuberPlugin.hpp"
#include "VtuberFrameWork.hpp"
#include "VtuberDelegate.hpp"
#include "View.hpp"
#include "MicCapture.hpp"
#include <obs-module.h>
#include <string.h>
#include <string>
#include <vector>

namespace {

struct Vtuber_data {
    obs_source_t *source;
    uint16_t      modelId;
    MicCapture   *mic;
    float         talkThreshold;
    bool          audioEnabled;
    int           deviceIndex;
    bool          wasTalking;    // para detectar cambio de estado
};

} // namespace

const char *VtuberPlugin::VtuberPlugin::VtuberGetName(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "bongobs cat";
}

void *VtuberPlugin::VtuberPlugin::VtuberCreate(obs_data_t *settings,
                                                obs_source_t *source)
{
    Vtuber_data *vtb   = (Vtuber_data *)bzalloc(sizeof(Vtuber_data));
    vtb->source        = source;
    vtb->modelId       = 0;
    vtb->mic           = nullptr;
    vtb->talkThreshold = (float)obs_data_get_double(settings, "talk_threshold");
    vtb->audioEnabled  = obs_data_get_bool(settings, "audio_enabled");
    vtb->deviceIndex   = (int)obs_data_get_int(settings, "mic_device");
    vtb->wasTalking    = false;

    double x              = obs_data_get_double(settings, "x");
    double y              = obs_data_get_double(settings, "y");
    int    width          = obs_data_get_int(settings, "width");
    int    height         = obs_data_get_int(settings, "height");
    double scale          = obs_data_get_double(settings, "scale");
    double delayTime      = obs_data_get_double(settings, "delaytime");
    bool   random_motion  = obs_data_get_bool(settings, "random_motion");
    bool   breath         = obs_data_get_bool(settings, "breath");
    bool   eyeblink       = obs_data_get_bool(settings, "eyeblink");
    bool   track          = obs_data_get_bool(settings, "track");
    const char *mode      = obs_data_get_string(settings, "mode");
    bool   live2D         = obs_data_get_bool(settings, "live2d");
    bool   relative_mouse = obs_data_get_bool(settings, "relative_mouse");
    bool   mouse_h_flip   = obs_data_get_bool(settings, "mouse_horizontal_flip");
    bool   mouse_v_flip   = obs_data_get_bool(settings, "mouse_vertical_flip");
    bool   mask           = obs_data_get_bool(settings, "mask");

    VtuberFrameWork::InitVtuber(vtb->modelId);
    VtuberFrameWork::UpData(vtb->modelId, x, y, width, height, scale,
                            delayTime, random_motion, breath, eyeblink,
                            NULL, track, mode, live2D, relative_mouse,
                            mouse_h_flip, mouse_v_flip, mask);

    if (vtb->audioEnabled) {
        vtb->mic = new MicCapture();
        if (!vtb->mic->Start(vtb->deviceIndex)) {
            delete vtb->mic;
            vtb->mic = nullptr;
        }
    }

    return vtb;
}

void VtuberPlugin::VtuberPlugin::VtuberDestroy(void *data)
{
    Vtuber_data *vtb = (Vtuber_data *)data;
    if (vtb->mic) { delete vtb->mic; vtb->mic = nullptr; }
    VtuberFrameWork::UinitVtuber(0);
    bfree(vtb);
}

void VtuberPlugin::VtuberPlugin::VtuberRender(void *data, gs_effect_t *effect)
{
    Vtuber_data *vtb = (Vtuber_data *)data;

    if (vtb->audioEnabled && vtb->mic) {
        View *view = VtuberDelegate::GetInstance()->GetView();
        if (view) {
            float level    = vtb->mic->GetLevel();
            bool  talking  = (level >= vtb->talkThreshold);

            if (talking) {
                // Hablando: activar talk.
                // El parpadeo durante talk lo maneja UpdateFaceAnim
                // usando el estado "blink_talk" si autoBlink está activo.
                view->SetFaceStateTalk(vtb->modelId);
            } else {
                // Silencio: volver a idle (el parpadeo normal sigue automático)
                view->SetFaceStateIdle(vtb->modelId);
            }
            vtb->wasTalking = talking;
        }
    }

    int width  = VtuberFrameWork::GetWidth(vtb->modelId);
    int height = VtuberFrameWork::GetHeight(vtb->modelId);

    obs_enter_graphics();
    gs_texture_t *tex = gs_texture_create(width, height, GS_RGBA, 1, NULL, GS_DYNAMIC);
    uint8_t  *ptr;
    uint32_t  linesize;
    if (gs_texture_map(tex, &ptr, &linesize))
        VtuberFrameWork::ReanderVtuber(vtb->modelId, (char *)ptr, width, height);
    gs_texture_unmap(tex);
    obs_source_draw(tex, 0, 0, 0, 0, true);
    gs_texture_destroy(tex);
    obs_leave_graphics();
}

uint32_t VtuberPlugin::VtuberPlugin::VtuberWidth(void *data)
    { return VtuberFrameWork::GetWidth(((Vtuber_data *)data)->modelId); }
uint32_t VtuberPlugin::VtuberPlugin::VtuberHeight(void *data)
    { return VtuberFrameWork::GetHeight(((Vtuber_data *)data)->modelId); }

static void fill_vtuber_model_list(obs_property_t *p, void *data)
{
    int _size;
    const char **names = VtuberFrameWork::GetModeDefine(_size);
    for (int i = 0; i < _size; i++)
        obs_property_list_add_string(p, names[i], names[i]);
}

static void fill_mic_list(obs_property_t *p)
{
    std::vector<std::string> devices = MicCapture::EnumDevices();
    for (int i = 0; i < (int)devices.size(); i++)
        obs_property_list_add_int(p, devices[i].c_str(), (long long)i);
}

obs_properties_t *VtuberPlugin::VtuberPlugin::VtuberGetProperties(void *data)
{
    Vtuber_data *vtb = (Vtuber_data *)data;
    obs_properties_t *ppts = obs_properties_create();
    obs_properties_set_param(ppts, vtb, NULL);
    obs_property_t *p;

    p = obs_properties_add_list(ppts, "mode", obs_module_text("Mode"),
                                OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    fill_vtuber_model_list(p, data);

    obs_properties_add_bool(ppts, "relative_mouse",
                            obs_module_text("Relative Mouse Movement"));
    obs_properties_add_bool(ppts, "mouse_horizontal_flip",
                            obs_module_text("Mouse Horizontal Flip"));
    obs_properties_add_bool(ppts, "mouse_vertical_flip",
                            obs_module_text("Mouse Vertical Flip"));
    obs_properties_add_bool(ppts, "mask",          obs_module_text("Use Mask"));
    obs_properties_add_bool(ppts, "live2d",        obs_module_text("Live2D"));
    obs_properties_add_float_slider(ppts, "delay", obs_module_text("Speed"),
                                    0.0, 10.0, 0.1);
    obs_properties_add_bool(ppts, "random_motion", obs_module_text("RandomMotion"));
    obs_properties_add_bool(ppts, "breath",        obs_module_text("Breath"));
    obs_properties_add_bool(ppts, "eyeblink",      obs_module_text("EyeBlink"));
    obs_properties_add_bool(ppts, "track",         obs_module_text("Mouse Tracktion"));

    // Audio
    obs_properties_add_bool(ppts, "audio_enabled",
                            obs_module_text("Enable Talk Animation"));
    p = obs_properties_add_list(ppts, "mic_device",
                                obs_module_text("Microphone"),
                                OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    fill_mic_list(p);

    // Slider reducido: 0.0 a 0.1 con paso 0.001 — igual que Veadotube
    obs_properties_add_float_slider(ppts, "talk_threshold",
                                    obs_module_text("Talk Threshold (sensitivity)"),
                                    0.0, 0.1, 0.001);

    return ppts;
}

void VtuberPlugin::VtuberPlugin::Vtuber_update(void *data, obs_data_t *settings)
{
    Vtuber_data *vtb = (Vtuber_data *)data;

    double x              = obs_data_get_double(settings, "x");
    double y              = obs_data_get_double(settings, "y");
    int    width          = obs_data_get_int(settings, "width");
    int    height         = obs_data_get_int(settings, "height");
    double vscale         = obs_data_get_double(settings, "scale");
    double delayTime      = obs_data_get_double(settings, "delay");
    bool   random_motion  = obs_data_get_bool(settings, "random_motion");
    bool   breath         = obs_data_get_bool(settings, "breath");
    bool   eyeblink       = obs_data_get_bool(settings, "eyeblink");
    bool   track          = obs_data_get_bool(settings, "track");
    const char *mode      = obs_data_get_string(settings, "mode");
    bool   live2D         = obs_data_get_bool(settings, "live2d");
    bool   relative_mouse = obs_data_get_bool(settings, "relative_mouse");
    bool   mouse_h_flip   = obs_data_get_bool(settings, "mouse_horizontal_flip");
    bool   mouse_v_flip   = obs_data_get_bool(settings, "mouse_vertical_flip");
    bool   mask           = obs_data_get_bool(settings, "mask");
    bool   audioEnabled   = obs_data_get_bool(settings, "audio_enabled");
    float  talkThreshold  = (float)obs_data_get_double(settings, "talk_threshold");
    int    deviceIndex    = (int)obs_data_get_int(settings, "mic_device");

    vtb->talkThreshold = talkThreshold;

    bool needRestart = (audioEnabled != vtb->audioEnabled) ||
                       (audioEnabled && deviceIndex != vtb->deviceIndex);
    if (needRestart) {
        if (vtb->mic) { delete vtb->mic; vtb->mic = nullptr; }
        if (audioEnabled) {
            vtb->mic = new MicCapture();
            if (!vtb->mic->Start(deviceIndex)) {
                delete vtb->mic; vtb->mic = nullptr;
            }
        } else {
            View *view = VtuberDelegate::GetInstance()->GetView();
            if (view) view->SetFaceStateIdle(vtb->modelId);
        }
    }
    vtb->audioEnabled = audioEnabled;
    vtb->deviceIndex  = deviceIndex;

    VtuberFrameWork::UpData(vtb->modelId, x, y, width, height, vscale,
                            delayTime, random_motion, breath, eyeblink,
                            NULL, track, mode, live2D, relative_mouse,
                            mouse_h_flip, mouse_v_flip, mask);
}

void VtuberPlugin::VtuberPlugin::Vtuber_defaults(obs_data_t *settings)
{
    obs_data_set_default_int(settings,    "width",  1280);
    obs_data_set_default_int(settings,    "height", 768);
    obs_data_set_default_double(settings, "x",      0);
    obs_data_set_default_double(settings, "y",      0.02);
    obs_data_set_default_double(settings, "scale",  1.83);
    obs_data_set_default_double(settings, "delaytime", 1.0);
    obs_data_set_default_bool(settings,   "random_motion", true);
    obs_data_set_default_bool(settings,   "breath",   true);
    obs_data_set_default_bool(settings,   "eyeblink", true);
    obs_data_set_default_bool(settings,   "track",    true);
    obs_data_set_default_string(settings, "Mode", "standard");
    obs_data_set_default_bool(settings,   "live2d",   true);
    obs_data_set_default_bool(settings,   "relative_mouse",       false);
    obs_data_set_default_bool(settings,   "mouse_horizontal_flip", true);
    obs_data_set_default_bool(settings,   "mouse_vertical_flip",   true);
    obs_data_set_default_bool(settings,   "mask",  false);
    obs_data_set_default_bool(settings,   "audio_enabled",  false);
    obs_data_set_default_int(settings,    "mic_device",     0);
    obs_data_set_default_double(settings, "talk_threshold", 0.02);
}