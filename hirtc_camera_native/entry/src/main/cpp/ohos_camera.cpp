// Created on 2024/3/5.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "ohos_camera.h"
// #include "api/video/i420_buffer.h"
// #include "api/video/video_rotation.h"
#include "libyuv/convert.h"
#include "libyuv/rotate.h"
// #include "rtc_base/time_utils.h"
#include "hilog/log.h"
#include <arm-linux-ohos/bits/alltypes.h>
#include <cstdint>
#include <locale.h>
#include <multimedia/image_framework/image_mdk.h>
#include <multimedia/image_framework/image_pixel_map_mdk.h>
#include <multimedia/image_framework/image_pixel_map_napi.h>
#include <multimedia/image_framework/image_receiver_mdk.h>
#include <malloc.h>
#include <multimedia/player_framework/native_avbuffer.h>
#include <native_buffer/native_buffer.h>
#include <string>

#include <pthread.h>
#include <mutex>
#include <queue>
#include <thread>


namespace webrtc {
namespace ohos {

std::queue<VideoFrame *> buffers_;
std::mutex render_lock;
static uint32_t request_width_ = 1440;
static uint32_t request_height_ = 1080;
void *RnederProcess(void *args) {
	OhosCamera *context = reinterpret_cast<OhosCamera *>(args);
	context->RunRenderProcess();
	return NULL;
}

class ImageReceiverOption {
public:
	ImageReceiverOption(uint32_t width, uint32_t height, uint32_t capacity)
		: width_(width), height_(height), capacity_(capacity) {
		Image_ErrorCode ret = OH_ImageReceiverOptions_Create(&options_);
		if (ret != IMAGE_SUCCESS) {
			OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
						 "OH_ImageReceiverOptions_Create failed with code = %{public}d", ret);
			return;
		}

		ret = OH_ImageReceiverOptions_SetSize(options_, {width_, height_});
		if (ret != IMAGE_SUCCESS) {
			OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
						 "OH_ImageReceiverOptions_SetSize failed with code = %{public}d", ret);
			return;
		}

		ret = OH_ImageReceiverOptions_SetCapacity(options_, capacity_);
		if (ret != IMAGE_SUCCESS) {
			OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
						 "OH_ImageReceiverOptions_SetCapacity failed with code = %{public}d", ret);
			return;
		}
	};

	OH_ImageReceiverOptions *GetImageReceiverOptions() { return options_; }

	~ImageReceiverOption() { OH_ImageReceiverOptions_Release(options_); }

private:
	uint32_t width_;
	uint32_t height_;
	uint32_t capacity_;
	OH_ImageReceiverOptions *options_{nullptr};
};

void ImageReceiverCallback(OH_ImageReceiverNative *receiver) {
	OhosCamera *ohosCamera = GlobalOHOSCamera::GetInstance().GetOhosCamera();
	if (ohosCamera == nullptr) {
		return;
	}

	OH_ImageNative *image;
	Image_ErrorCode ret = OH_ImageReceiverNative_ReadLatestImage(receiver, &image);
	if (ret != IMAGE_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_ImageReceiverNative_ReadLatestImage failed with code = %{public}d", ret);
		return;
	}

	Image_Size imageSize;
	ret = OH_ImageNative_GetImageSize(image, &imageSize);
	if (ret != IMAGE_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_ImageNative_GetImageSize failed with code = %{public}d", ret);
		return;
	}

	uint32_t *types;
	size_t typeSize;
	ret = OH_ImageNative_GetComponentTypes(image, nullptr, &typeSize);
	if (ret != IMAGE_SUCCESS || typeSize <= 0) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_ImageNative_GetComponentTypes failed with code = %{public}d", ret);
		return;
	}

	types = new uint32_t[typeSize];
	ret = OH_ImageNative_GetComponentTypes(image, &types, &typeSize);
	if (ret != IMAGE_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_ImageNative_GetComponentTypes failed with code = %{public}d", ret);
		delete[] types;
		return;
	}

	int32_t rowStride;
	ret = OH_ImageNative_GetRowStride(image, types[0], &rowStride);
	if (ret != IMAGE_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_ImageNative_GetRowStride failed with code = %{public}d", ret);
		delete[] types;
		return;
	}

	uint8_t *addr = nullptr;
	OH_NativeBuffer *buffer;
	size_t bufferSize;
	ret = OH_ImageNative_GetByteBuffer(image, types[0], &buffer);
	if (ret != IMAGE_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_ImageNative_GetByteBuffer failed with code = %{public}d", ret);
		delete[] types;
		return;
	}
	ret = OH_ImageNative_GetBufferSize(image, types[0], &bufferSize);
	if (ret != IMAGE_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_ImageNative_GetBufferSize failed with code = %{public}d", ret);
		delete[] types;
		return;
	}

	delete[] types;
	int32_t retInt = OH_NativeBuffer_Map(buffer, (void **)&addr);
	if (retInt != 0) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_NativeBuffer_Map failed with code = %{public}d",
					 retInt);
		return;
	}

	ohosCamera->ImageReceiverOn(addr, imageSize.width, imageSize.height, rowStride, bufferSize);

	ret = OH_ImageNative_Release(image);
	if (ret != IMAGE_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_ImageNative_Release  failed with code = %{public}d",
					 ret);
		return;
	}
}

char *OhosCamera::x_component_surfaceId_ = nullptr;
OhosCamera::OhosCamera() {
	//     InitImageReceiver();
	//     InitCamera();
	//     GlobalOHOSCamera::GetInstance().SetOhosCamera(this);
}

bool OhosCamera::Init(napi_env env, napi_callback_info info) {
	size_t argc = 2;
	// 声明参数数组
	napi_value args[2] = {nullptr};

	// 获取传入的参数并依次放入参数数组中
	napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

	size_t result;
	napi_get_value_string_utf8(env, args[1], nullptr, 0, &result);
	if (result > 0) {
		x_component_surfaceId_ = new char[result + 1];
		napi_get_value_string_utf8(env, args[1], x_component_surfaceId_, result + 1, &result);
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "x_component_surfaceId_ = %{public}s",
					 x_component_surfaceId_);
	}

	return true;
}

int32_t OhosCamera::InitImageReceiver() {
	int32_t returnCode = -1;
	ImageReceiverOption imageReceiverOption(640, 480, 8);
	Image_ErrorCode ret =
		OH_ImageReceiverNative_Create(imageReceiverOption.GetImageReceiverOptions(), &imageReceiverNative_);
	if (ret != IMAGE_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_ImageReceiverNative_Create failed with code = %{public}d", ret);
		return returnCode;
	}

	ret = OH_ImageReceiverNative_On(imageReceiverNative_, ImageReceiverCallback);
	if (ret != IMAGE_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_ImageReceiverNative_On failed with code = %{public}d",
					 ret);
		return returnCode;
	}

	uint64_t id;
	ret = OH_ImageReceiverNative_GetReceivingSurfaceId(imageReceiverNative_, &id);
	std::string idStr = std::to_string(id);
	img_receive_surfaceId_ = new char[idStr.length() + 1];
	std::strcpy(img_receive_surfaceId_, idStr.c_str());
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_ImageReceiverNative_GetReceivingSurfaceId = %{public}s",
				 idStr.c_str());

	returnCode = 0;
	return returnCode;
}

int32_t OhosCamera::InitCamera() {
	int32_t return_code = -1;

	Camera_ErrorCode ret = OH_Camera_GetCameraManager(&camera_manager_);
	if (camera_manager_ == nullptr || ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_Camera_GetCameraManager failed, ret = %{public}d",
					 ret);
		return return_code;
	}

	ret = OH_CameraManager_GetSupportedCameras(camera_manager_, &cameras_, &cameras_size_);
	if (cameras_ == nullptr || ret != CAMERA_OK || cameras_size_ <= 0) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_CameraManager_GetSupportedCameras failed, ret = %{public}d", ret);
		return return_code;
	}

	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_CameraManager_GetSupportedCameras count = %{public}d",
				 cameras_size_);

	for (int i = 0; i < cameras_size_; i++) {
		char *id = cameras_[i].cameraId;
		int32_t pos = cameras_[i].cameraPosition;
		int32_t type = cameras_[i].cameraType;
		int32_t conn = cameras_[i].connectionType;
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "camera index=%{public}d, id=%{public}s, pos=%{public}d, type=%{public}d, conn=%{public}d", i, id,
					 pos, type, conn);
	}

	ret = OH_CameraManager_GetSupportedCameraOutputCapability(camera_manager_, &cameras_[camera_dev_index_],
															  &camera_output_capability_);
	if (camera_output_capability_ == nullptr || ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_CameraManager_GetSupportedCameraOutputCapability failed, ret = %{public}d", ret);
		return return_code;
	}

	if (camera_output_capability_->metadataProfilesSize <= 0) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "metadataProfilesSize <= 0");
		return return_code;
	}

	for (int i = 0; i < camera_output_capability_->previewProfilesSize; i++) {
		int width = camera_output_capability_->previewProfiles[i]->size.width;
		int height = camera_output_capability_->previewProfiles[i]->size.height;
		Camera_Format format = camera_output_capability_->previewProfiles[i]->format;
		if (width == request_width_ && height == request_height_ && format == CAMERA_FORMAT_YUV_420_SP) {
			profile_index_ = i;
		}
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "previewProfiles index: %{public}d, format: %{public}d, width: %{public}d, height:%{public}d", i,
					 format, width, height);
	}

	return_code = 0;
	return return_code;
}

int32_t OhosCamera::CameraInputCreateAndOpen() {
	Camera_ErrorCode ret;
	ret = OH_CameraManager_CreateCameraInput(camera_manager_, &cameras_[camera_dev_index_], &camera_input_);
	if (camera_input_ == nullptr || ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_CameraManager_CreateCaptureSession failed, ret = %{public}d", ret);
		return -1;
	}

	ret = OH_CameraInput_Open(camera_input_);
	if (ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_CameraInput_Open failed, ret = %{public}d", ret);
		return -1;
	}

	return 0;
}

int32_t OhosCamera::CameraInputRelease() {
	Camera_ErrorCode ret;
	if (camera_input_ != nullptr) {
		ret = OH_CameraInput_Close(camera_input_);
		if (ret != CAMERA_OK) {
			OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_CameraInput_Close failed, ret = %{public}d", ret);
		}
		// no need to release, will cause 7400201 error, fatal service
		ret = OH_CameraInput_Release(camera_input_);
		if (ret != CAMERA_OK) {
			OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_CameraInput_Release failed, ret = %{public}d",
						 ret);
		}
	}

	camera_input_ = nullptr;
	return 0;
}

int32_t OhosCamera::PreviewOutputCreate() {
	Camera_ErrorCode ret;
	ret = OH_CameraManager_GetSupportedCameraOutputCapability(camera_manager_, &cameras_[camera_dev_index_],
															  &camera_output_capability_);
	if (camera_output_capability_ == nullptr || ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_CameraManager_GetSupportedCameraOutputCapability failed, ret = %{public}d", ret);
		return -1;
	}

	if (camera_output_capability_->metadataProfilesSize <= 0) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "metadataProfilesSize <= 0");
		return -1;
	}

	preview_profile_ = camera_output_capability_->previewProfiles[profile_index_];
	ret = OH_CameraManager_CreatePreviewOutput(camera_manager_, preview_profile_, img_receive_surfaceId_,
											   &preview_output_);
	if (preview_output_ == nullptr || ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_CameraManager_CreatePreviewOutput failed, ret = %{public}d", ret);
		return -1;
	}

	if (x_component_surfaceId_ != nullptr) {
		ret = OH_CameraManager_CreatePreviewOutput(camera_manager_, preview_profile_, x_component_surfaceId_,
												   &x_component_preview_);
		if (preview_output_ == nullptr || ret != CAMERA_OK) {
			OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
						 "OH_CameraManager_CreatePreviewOutput failed, ret = %{public}d", ret);
			return -1;
		}
	}

	return 0;
}

int32_t OhosCamera::PreviewOutputRelease() {
	Camera_ErrorCode ret;
	if (preview_output_ != nullptr) {
		ret = OH_PreviewOutput_Release(preview_output_);
		if (ret != CAMERA_OK) {
			OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_PreviewOutput_Release failed, ret = %{public}d",
						 ret);
		}
	}

	if (x_component_preview_ != nullptr) {
		ret = OH_PreviewOutput_Release(x_component_preview_);
		if (ret != CAMERA_OK) {
			OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_PreviewOutput_Release failed, ret = %{public}d",
						 ret);
		}
	}

	preview_output_ = nullptr;
	x_component_preview_ = nullptr;
	return 0;
}

int32_t OhosCamera::CaptureSessionSetting() {
	Camera_ErrorCode ret;
	ret = OH_CameraManager_CreateCaptureSession(camera_manager_, &capture_session_);
	if (capture_session_ == nullptr || ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_CameraManager_CreateCaptureSession failed, ret = %{public}d", ret);
		return -1;
	}

	ret = OH_CaptureSession_BeginConfig(capture_session_);
	if (ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_CaptureSession_BeginConfig failed, ret = %{public}d",
					 ret);
		return -1;
	}

	if (camera_input_) {
		ret = OH_CaptureSession_AddInput(capture_session_, camera_input_);
		if (ret != CAMERA_OK) {
			OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_CaptureSession_AddInput failed, ret = %{public}d",
						 ret);
			return -1;
		}
	}

	if (preview_output_) {
		ret = OH_CaptureSession_AddPreviewOutput(capture_session_, preview_output_);
		if (ret != CAMERA_OK) {
			OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
						 "OH_CaptureSession_AddPreviewOutput failed, ret = %{public}d", ret);
			return -1;
		}
	}

	if (x_component_preview_) {
		ret = OH_CaptureSession_AddPreviewOutput(capture_session_, x_component_preview_);
		if (ret != CAMERA_OK) {
			OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
						 "OH_CaptureSession_AddPreviewOutput failed, ret = %{public}d", ret);
			return -1;
		}
	}

	ret = OH_CaptureSession_CommitConfig(capture_session_);
	if (ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_CaptureSession_CommitConfig failed, ret = %{public}d",
					 ret);
		return -1;
	}
	return 0;
}

int32_t OhosCamera::CaptureSessionUnsetting() {
	Camera_ErrorCode ret;
	if (capture_session_ == nullptr) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "CaptureSessionUnsetting failed, capture_session_ is nullptr");
		return -1;
	}

	ret = OH_CaptureSession_Release(capture_session_);
	capture_session_ = nullptr;
	if (ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_CaptureSession_Release failed, ret = %{public}d",
					 ret);
		return -1;
	}

	return 0;
}

int32_t OhosCamera::StartCamera() {
	if (is_camera_started_) {
		// 先关闭相机、会话、预览流，然后再重新打开
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "StartCamera: Camera has been started");
		StopCamera();
		CaptureSessionUnsetting();
		CameraInputRelease();
		PreviewOutputRelease();
	}
	Camera_ErrorCode ret = CAMERA_OK;
	InitImageReceiver();

	if (camera_input_ == nullptr) {
		CameraInputCreateAndOpen();
	}
	if (preview_output_ == nullptr) {
		PreviewOutputCreate();
	}
	if (capture_session_ == nullptr) {
		CaptureSessionSetting();
	}

	ret = OH_CaptureSession_Start(capture_session_);
	if (ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_CaptureSession_Start failed, ret = %{public}d", ret);
		return -1;
	}

	ret = OH_CameraDevice_GetCameraOrientation(&cameras_[camera_dev_index_], &camera_orientitaion_);
	if (ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_CameraDevice_GetCameraOrientation failed, ret = %{public}d", ret);
		return -1;
	}

	is_camera_started_ = true;

	pthread_t render_thr;
	int res = pthread_create(&render_thr, NULL, RnederProcess, this);
	if (res != 0) {
		OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_DOMAIN, "mytest", "pthread_create failed");
	}

	return 0;
}

int32_t OhosCamera::StopCamera() {
	Camera_ErrorCode ret = CAMERA_OK;

	if (is_camera_started_ == false) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "StopCamera: Camera not started");
		return 0;
	}

	if (capture_session_ == nullptr) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "StopCamera: capture_session_ is nullptr");
		return -1;
	}

	ret = OH_CaptureSession_Stop(capture_session_);
	if (ret != CAMERA_OK) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_CaptureSession_Stop failed, ret = %{public}d", ret);
		return -1;
	}

	is_camera_started_ = false;

	return 0;
}

int32_t OhosCamera::ImageReceiverRelease() {
	if (imageReceiverNative_ == nullptr) {
		return 0;
	}
	Image_ErrorCode ret = OH_ImageReceiverNative_Off(imageReceiverNative_);
	if (ret != IMAGE_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_ImageReceiverNative_Off failed, ret = %{public}d",
					 ret);
	}

	ret = OH_ImageReceiverNative_Release(imageReceiverNative_);
	if (ret != IMAGE_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_ImageReceiverNative_Release failed, ret = %{public}d",
					 ret);
	}

	imageReceiverNative_ = nullptr;
	return 0;
}
int32_t OhosCamera::DeleteCameraOutputCapability() {
	if (camera_output_capability_ != nullptr) {
		// napi暂不支持该操作
		// Camera_ErrorCode ret = OH_CameraManager_DeleteSupportedCameraOutputCapability(camera_manager_,
		// camera_output_capability_); if (ret != CAMERA_OK) {
		//   OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
		//   "OH_CameraManager_DeleteSupportedCameraOutputCapability\ failed, ret = %{public}d", ret);
		// }
		preview_profile_ = nullptr;
		camera_output_capability_ = nullptr;
	}

	return 0;
}

int32_t OhosCamera::DeleteCameras() {
	if (cameras_ != nullptr) {
		// napi暂不支持该操作
		// Camera_ErrorCode ret = OH_CameraManager_DeleteSupportedCameras(camera_manager_, cameras_,
		// cameras_size_); if (ret != CAMERA_OK) {
		//   OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
		//                "OH_CameraManager_DeleteSupportedCameras failed, ret = %{public}d", ret);
		// }
		cameras_ = nullptr;
	}

	return 0;
}

int32_t OhosCamera::DeleteCameraManage() {
	if (camera_manager_ != nullptr) {
		// napi暂不支持该操作
		// Camera_ErrorCode ret = OH_Camera_DeleteCameraManager(camera_manager_);
		// if(ret != CAMERA_OK) {
		//   OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_Camera_DeleteCameraManager failed, ret =
		//   %{public}d", ret);
		// }
		camera_manager_ = nullptr;
	}

	return 0;
}

int32_t OhosCamera::CameraRelease() {
	if (is_camera_started_) {
		StopCamera();
	}
	CaptureSessionUnsetting();
	CameraInputRelease();
	PreviewOutputRelease();
	ImageReceiverRelease();

	DeleteCameraOutputCapability();
	DeleteCameras();
	DeleteCameraManage();
	return 0;
}

uint32_t OhosCamera::GetCameraIndex() { return camera_dev_index_; }

int32_t OhosCamera::SetCameraIndex(uint32_t camera_index) {
	if (camera_index >= 0 && camera_index <= cameras_size_) {
		camera_dev_index_ = camera_index;
	}
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "camera_dev_index_ = %{public}d", camera_dev_index_);
	return 0;
}

bool OhosCamera::ImageReceiverOn(uint8_t *buffer, int32_t width, int32_t height, int32_t stride, size_t bufferSize) {
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
				 "imageReceiverOn started, width=%{public}d, height=%{public}d, orientitaion=%{public}d", width, height,
				 camera_orientitaion_);
	height = (height > 0) ? height : -height; // abs
	width = stride;
	size_t size = stride * height + stride * height / 2;
	if (bufferSize < size) {
		return false;
	}

	uint8_t *i420_buffer = new uint8_t[size];
	uint8_t *i420_u = i420_buffer + stride * height;
	uint8_t *i420_v = i420_u + stride * height / 4;

	int res = libyuv::NV21ToI420(buffer, width, buffer + width * height, width, i420_buffer, width, i420_u, width / 2,
								 i420_v, width / 2, width, height);


	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "NV21ToI420 res=%{public}d", res);

	//			uint8_t *i420_data = new uint8_t[size];
	//			uint8_t *i420_data_u = i420_data + height * width;
	//			uint8_t *i420_data_v = i420_data_u + width * height / 4;
	//			libyuv::I420Rotate(i420_buffer, width, i420_u, width / 2, i420_v, width / 2, i420_data, height,
	//i420_data_u, 							   height / 2, i420_data_v, height / 2, width, height, libyuv::kRotate90);

	//     uint8_t* bgra_buffer = new uint8_t[width * height * 4];
	//     res = libyuv::I420ToARGB(i420_buffer, width, i420_u, width / 2, i420_v, width / 2, bgra_buffer, width
	//     * 4, width, height);

	{
		std::lock_guard<std::mutex> lock(render_lock);
		VideoFrame *frame = new VideoFrame;
		frame->buffer = i420_buffer;
		frame->width = width;
		frame->height = height;
		frame->stride = width;
		frame->orientation = camera_orientitaion_;
		buffers_.push(frame);
	}

	//     if (ohos_render_)
	//     {
	//         //ohos_render_->OnFrame(bgra_buffer, width, height, width * 4, width * height * 4);
	//     }
	//     else
	//     {
	//         std::string id = "12345";
	//         OHNativeWindow* native_window = PluginManager::GetInstance()->GetRender(id);
	//         ohos_render_ = new ohosRender(id, native_window);
	//     }

	//   rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer = webrtc::I420Buffer::Create(width, height);
	//   libyuv::NV21ToI420(buffer, width, buffer + width * height, width, i420_buffer.get()->MutableDataY(),
	//                      i420_buffer.get()->StrideY(), i420_buffer.get()->MutableDataU(),
	//                      i420_buffer.get()->StrideU(), i420_buffer.get()->MutableDataV(),
	//                      i420_buffer.get()->StrideV(), width, height);
	//
	//   webrtc::VideoFrame video_frame = webrtc::VideoFrame::Builder()
	//                                      .set_video_frame_buffer(i420_buffer)
	//                                      .set_timestamp_rtp(0)
	//                                      .set_timestamp_ms(rtc::TimeMillis())
	//                                      .set_rotation(webrtc::kVideoRotation_90)
	//                                      .build();
	//   if (data_callback_) {
	//     data_callback_->OnFrame(video_frame);
	//   }
	//     delete[] bgra_buffer;
	//     delete[] i420_buffer;
	return true;
}

napi_value OhosCamera::GetImageData(napi_env env, ImageReceiverNative *image_receiver_c) {
	int32_t ret;
	napi_value next_image;
	// 或调用 OH_Image_Receiver_ReadNextImage(imgReceiver_c, &nextImage);
	ret = OH_Image_Receiver_ReadLatestImage(image_receiver_c, &next_image);
	if (ret != IMAGE_RESULT_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
					 "OH_Image_Receiver_ReadLatestImage failed, ret = %{public}d", ret);
		return nullptr;
	}

	ImageNative *next_image_native = OH_Image_InitImageNative(env, next_image);

	OhosImageSize img_size;
	ret = OH_Image_Size(next_image_native, &img_size);
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_Image_Size  width: %{public}d, height:%{public}d",
				 img_size.width, img_size.height);
	if (ret != IMAGE_RESULT_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_Image_Size failed, ret = %{public}d", ret);
		return nullptr;
	}

	OhosImageComponent imgComponent;
	ret = OH_Image_GetComponent(next_image_native, 4, &imgComponent); // 4=jpeg
	if (ret != IMAGE_RESULT_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_Image_GetComponent failed, ret = %{public}d", ret);
		return nullptr;
	}

	//  uint8_t *img_buffer = imgComponent.byteBuffer;
	//  if (ImageReceiverOn(img_buffer, img_size.width, img_size.height) == false) {
	//    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "ImageReceiverOn failed");
	//    return nullptr;
	//  }

	ret = OH_Image_Release(next_image_native);
	if (ret != IMAGE_RESULT_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OH_Image_Release failed, ret = %{public}d", ret);
		return nullptr;
	}
	return next_image;
}

// void OhosCamera::RegisterCaptureDataCallback(rtc::VideoSinkInterface<webrtc::VideoFrame> *data_callback) {
//   data_callback_ = data_callback;
// }

// void OhosCamera::UnregisterCaptureDataCallback() {
//   data_callback_ = nullptr;
// }

OhosCamera::~OhosCamera() {
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OhosCamera::~OhosCamera start");

	//   GlobalOHOSCamera::GetInstance().UnsetOhosCamera();
	CameraRelease();
	if (img_receive_surfaceId_ != nullptr) {
		delete[] img_receive_surfaceId_;
		img_receive_surfaceId_ = nullptr;
	}

	if (x_component_preview_ != nullptr) {
		delete[] x_component_surfaceId_;
		x_component_surfaceId_ = nullptr;
	}

	camera_dev_index_ = 0;
	profile_index_ = 0;
}

void OhosCamera::RunRenderProcess() {
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "RunRenderProcess");
	while (is_camera_started_) {
		VideoFrame *frame = nullptr;
		{
			std::lock_guard<std::mutex> lock(render_lock);
			if (buffers_.empty()) {
				// OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "RunRenderProcess with null buffers");
				// std::this_thread::sleep_for(std::chrono::nanoseconds(30 * 1000 * 1000));
				continue;
			}
			frame = buffers_.front();
			buffers_.pop();
		}
		if (ohos_render_) {
			int width = frame->width;
			int height = frame->height;
			// ohos_render_->OnFrame(data, width, height, width * 4, width * height * 4);
			ohos_render_->OnFrame(frame->buffer, width, height, width, width * height * 3 / 2, camera_orientitaion_);
			delete[] frame->buffer;
			delete[] frame;
		} else {
			std::string id = "12345";
			OHNativeWindow *native_window = PluginManager::GetInstance()->GetRender(id);
			ohos_render_ = new ohosRender(id, native_window);
		}
	}
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "Exit RunRenderProcess");
}

} // namespace ohos
} // namespace webrtc