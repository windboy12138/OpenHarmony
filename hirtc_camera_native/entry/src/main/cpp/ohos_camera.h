//
// Created on 2024/3/6.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".


#ifndef OH_WEB_RTC_OHOS_CAMERA_H
#define OH_WEB_RTC_OHOS_CAMERA_H

#include "hilog/log.h"
#include "napi/native_api.h"
#include <arm-linux-ohos/bits/alltypes.h>
#include <cstdint>
#include <multimedia/image_framework/image_receiver_mdk.h>
#include <ohcamera/camera.h>
#include <ohcamera/camera_device.h>
#include <ohcamera/camera_manager.h>
#include <multimedia/image_framework/image/image_receiver_native.h>
#include <multimedia/image_framework/image/image_common.h>
#include <multimedia/image_framework/image/image_native.h>
#include <native_buffer/native_buffer.h>
#include <native_image/native_image.h>

#include "ohos_render.h"
// #include "rtc_base/ref_counter.h"
// #include "api/video/video_source_interface.h"
// #include "api/video/video_frame.h"
// #include "modules/video_capture/video_capture.h"


namespace webrtc {
namespace ohos {

struct VideoFrame {
	uint8_t *buffer;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t orientation;
};

class OhosCamera {
public:
	OhosCamera();
	static bool Init(napi_env env, napi_callback_info info);
	int32_t InitCamera();
	int32_t InitImageReceiver();
	int32_t StartCamera();
	int32_t StopCamera();
	int32_t CameraRelease();

	uint32_t GetCameraIndex();
	int32_t SetCameraIndex(uint32_t camera_index);
	napi_value GetImageData(napi_env env, ImageReceiverNative *image_receiver_c);
	bool ImageReceiverOn(uint8_t *buffer, int32_t width, int32_t height, int32_t stride, size_t bufferSize);
	//   void RegisterCaptureDataCallback(rtc::VideoSinkInterface<webrtc::VideoFrame> *dataCallback);
	void UnregisterCaptureDataCallback();
	~OhosCamera();

	void RunRenderProcess();

private:
	int32_t CameraInputCreateAndOpen();
	int32_t CameraInputRelease();

	int32_t PreviewOutputCreate();
	int32_t PreviewOutputRelease();

	int32_t CaptureSessionSetting();
	int32_t CaptureSessionUnsetting();

	int32_t ImageReceiverRelease();

	int32_t DeleteCameraOutputCapability();
	int32_t DeleteCameras();
	int32_t DeleteCameraManage();

	//   rtc::VideoSinkInterface<webrtc::VideoFrame> *data_callback_ {nullptr};
	//   webrtc::VideoCaptureCapability configured_capability_;
	bool is_camera_started_{false};
	char *img_receive_surfaceId_{nullptr};
	static char *x_component_surfaceId_;
	OH_ImageReceiverNative *imageReceiverNative_{nullptr};
	Camera_Manager *camera_manager_{nullptr};
	Camera_Device *cameras_{nullptr};
	uint32_t cameras_size_{0};
	Camera_CaptureSession *capture_session_{nullptr};
	Camera_OutputCapability *camera_output_capability_{nullptr};
	const Camera_Profile *preview_profile_{nullptr};
	Camera_PreviewOutput *preview_output_{nullptr};
	Camera_PreviewOutput *x_component_preview_{nullptr};
	Camera_Input *camera_input_{nullptr};
	uint32_t camera_dev_index_{0};
	uint32_t profile_index_{61};
	ohosRender *ohos_render_{nullptr};
	uint32_t camera_orientitaion_{0};
};
class GlobalOHOSCamera {
public:
	static GlobalOHOSCamera &GetInstance() {
		static GlobalOHOSCamera globalOHOSCamera;
		return globalOHOSCamera;
	}

	~GlobalOHOSCamera() {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "GlobalOHOSCamera::~GlobalOHOSCamera start");
		if (ohosCamera_) {
			delete ohosCamera_;
			ohosCamera_ = nullptr;
		}
	}

	void SetOhosCamera(OhosCamera *ohosCamera) { ohosCamera_ = ohosCamera; }

	void UnsetOhosCamera() { ohosCamera_ = nullptr; }

	OhosCamera *GetOhosCamera() {
		if (ohosCamera_ == nullptr) {
			ohosCamera_ = new OhosCamera;
		}
		return ohosCamera_;
	}

private:
	GlobalOHOSCamera(){};
	OhosCamera *ohosCamera_{nullptr};
};
} // namespace ohos
} // namespace webrtc
#endif // OH_WEB_RTC_OHOS_CAMERA_H
