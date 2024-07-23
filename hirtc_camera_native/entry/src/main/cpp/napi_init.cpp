#include "napi/native_api.h"
#include "hilog/log.h"
// #include "api/scoped_refptr.h"
// #include "pc/video_track_source.h"
// #include "ohos_capturer_track_source.h"
// #include "ohos_camera_capture.h"
#include "ohos_camera.h"
#include "ohos_render.h"
#include "ohos_audio.h"

#include <arm-linux-ohos/bits/alltypes.h>
#include <multimedia/image_framework/image_mdk.h>
#include <multimedia/image_framework/image_receiver_mdk.h>
#include <malloc.h>

static napi_value InitCamera(napi_env env, napi_callback_info info) {
    webrtc::ohos::GlobalOHOSCamera::GetInstance().GetOhosCamera()->Init(env, info);
//     rtc::scoped_refptr<webrtc::ohos::CapturerTrackSource> ohos_cts = webrtc::ohos::CapturerTrackSource::Create();

    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "had been try create");
    
    webrtc::ohos::GlobalOHOSCamera::GetInstance().GetOhosCamera()->SetCameraIndex(0);
    webrtc::ohos::GlobalOHOSCamera::GetInstance().GetOhosCamera()->InitCamera();
    webrtc::ohos::GlobalOHOSCamera::GetInstance().GetOhosCamera()->StartCamera();
    
    return nullptr;
}

static napi_value StopCamera(napi_env env, napi_callback_info info) {
    webrtc::ohos::GlobalOHOSCamera::GetInstance().GetOhosCamera()->StopCamera();
    webrtc::ohos::GlobalOHOSCamera::GetInstance().GetOhosCamera()->CameraRelease();
    
    return nullptr;
}

static napi_value ChangeCamera(napi_env env, napi_callback_info info) {
    webrtc::ohos::GlobalOHOSCamera::GetInstance().GetOhosCamera()->StopCamera();
    webrtc::ohos::GlobalOHOSCamera::GetInstance().GetOhosCamera()->CameraRelease();
    uint32_t camera_index = webrtc::ohos::GlobalOHOSCamera::GetInstance().GetOhosCamera()->GetCameraIndex();
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "camera_index = %{public}d", camera_index);
    camera_index = camera_index <= 1 ? 1 - camera_index : 0;
    webrtc::ohos::GlobalOHOSCamera::GetInstance().GetOhosCamera()->SetCameraIndex(camera_index);
    webrtc::ohos::GlobalOHOSCamera::GetInstance().GetOhosCamera()->InitCamera();
    webrtc::ohos::GlobalOHOSCamera::GetInstance().GetOhosCamera()->StartCamera();

    return nullptr;
}

static napi_value StartRecord(napi_env env, napi_callback_info info) {
    StartAudioReocrd();
    return nullptr;
}

static napi_value StopAudio(napi_env env, napi_callback_info info) {
    StopAudio();
    return nullptr;
}

static napi_value StartPlay(napi_env env, napi_callback_info info) {
    StartAudioPlay();
    return nullptr;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "Init begins");
    if ((nullptr == env) || (nullptr == exports)) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_DOMAIN, "mytest", "env or exports is null");
        return nullptr;
    }
    
    napi_property_descriptor desc[] = {
        {"initCamera", nullptr, InitCamera, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"stopCamera", nullptr, StopCamera, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"changeCamera", nullptr, ChangeCamera, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"startRecord", nullptr, StartRecord, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"stopAudio", nullptr, StopAudio, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"startPlay", nullptr, StartPlay, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    // render
    webrtc::ohos::PluginManager::GetInstance()->Export(env, exports);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void) { napi_module_register(&demoModule); }
