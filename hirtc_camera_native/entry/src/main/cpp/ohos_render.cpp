//
// Created on 2024/6/24.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include <cstdint>
#include <ohos_render.h>
#include <sys/mman.h>
#include <native_buffer/native_buffer.h>

#include "hilog/log.h"

namespace webrtc {
namespace ohos {

void PluginManager::Export(napi_env env, napi_value exports) {
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "Export: PluginManager");
	if ((env == nullptr) || (exports == nullptr)) {
		OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_DOMAIN, "mytest", "Export: env or exports is null");
		return;
	}

	napi_value exportInstance = nullptr;
	if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) != napi_ok) {
		OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_DOMAIN, "mytest", "Export: napi_get_named_property fail");
		return;
	}

	OH_NativeXComponent *nativeXComponent = nullptr;
	if (napi_unwrap(env, exportInstance, reinterpret_cast<void **>(&nativeXComponent)) != napi_ok) {
		OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_DOMAIN, "mytest", "Export: napi_unwrap fail");
		return;
	}

	char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
	uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
	if (OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
		OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_DOMAIN, "mytest", "Export: OH_NativeXComponent_GetXComponentId fail");
		return;
	}

	std::string id(idStr);
	auto context = PluginManager::GetInstance();
	if ((context != nullptr) && (nativeXComponent != nullptr)) {
		context->SetNativeXComponent(id, nativeXComponent);
		//             auto render = context->GetRender(id);
		//             if (render != nullptr)
		//             {
		// //                 render->RegisterCallback(nativeXComponent);
		// //                 render->Export(env, exports);
		//             } else
		//             {
		// //                 DRAWING_LOGE("render is nullptr");
		//             }
	}
	// 初始化 OH_NativeXComponent_Callback
	// must use static, otherwise 'Ability onDestroy' will deconstruct this member, so xcomponent destroy
	// cannot callback on this function
	static OH_NativeXComponent_Callback callback;
	callback.OnSurfaceCreated = OnSurfaceCreatedCB;
	callback.OnSurfaceChanged = OnSurfaceChangedCB;
	callback.OnSurfaceDestroyed = OnSurfaceDestroyedCB;
	callback.DispatchTouchEvent = DispatchTouchEventCB;

	// 注册回调函数
	// after register this callback, kill app will not log 'Ability onWindowStageDestroy' and 'Ability
	// onDestroy' will crash after kill app
	OH_NativeXComponent_RegisterCallback(nativeXComponent, &callback);
}

void PluginManager::SetNativeXComponent(std::string &id, OH_NativeXComponent *nativeXComponent) {
	nativeXComponentMap_[id] = nativeXComponent;
}

void PluginManager::SetNativeWindow(std::string &id, OHNativeWindow *nativeWindow) {
	pluginRenderMap_[id] = nativeWindow;
}

OHNativeWindow *PluginManager::GetRender(std::string &id) { return pluginRenderMap_[id]; }

// 定义回调函数
void OnSurfaceCreatedCB(OH_NativeXComponent *component, void *window) {
	//         OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OnSurfaceCreatedCB");
	char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
	uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
	if (OH_NativeXComponent_GetXComponentId(component, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
		//             DRAWING_LOGE("Export: OH_NativeXComponent_GetXComponentId fail");
		return;
	}
	std::string id(idStr);
	// 可获取 OHNativeWindow 实例
	OHNativeWindow *nativeWindow = static_cast<OHNativeWindow *>(window);
	auto context = PluginManager::GetInstance();
	context->SetNativeWindow(id, nativeWindow);
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OnSurfaceCreatedCB, id=%{public}s", id.c_str());
}
void OnSurfaceChangedCB(OH_NativeXComponent *component, void *window) {
	// 可获取 OHNativeWindow 实例
	OHNativeWindow *nativeWindow = static_cast<OHNativeWindow *>(window);
	// ...
}
void OnSurfaceDestroyedCB(OH_NativeXComponent *component, void *window) {
	char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
	uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
	if (OH_NativeXComponent_GetXComponentId(component, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
		return;
	}
	OHNativeWindow *nativeWindow = static_cast<OHNativeWindow *>(window);
	// ...
	std::string id(idStr);
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "OnSurfaceDestroyedCB, id=%{public}s", id.c_str());
}
void DispatchTouchEventCB(OH_NativeXComponent *component, void *window) {
	// 可获取 OHNativeWindow 实例
	OHNativeWindow *nativeWindow = static_cast<OHNativeWindow *>(window);
	// ...
}

ohosRender::ohosRender(std::string &id, OHNativeWindow *nativeWindow) {
	// 设置 OHNativeWindowBuffer 的宽高
	int32_t code = SET_BUFFER_GEOMETRY;
	int32_t width = 1080;
	int32_t height = 1920;
	// 这里的nativeWindow是从上一步骤中的回调函数中获得的
	int32_t ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, width, height);

	code = SET_FORMAT;
	int32_t format = NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P;
	ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, format);

	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "set window format=%{public}d", ret);

	code = SET_TRANSFORM;
	int32_t transform = NATIVEBUFFER_ROTATE_NONE;
	ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, transform);
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "SET_TRANSFORM=%{public}d", ret);

	// SET_STRIDE will cause unknown error when OH_NativeWindow_NativeWindowRequestBuffer
	//         code = SET_STRIDE;
	//         int32_t stride = 1920;
	//         ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, code, &stride);
	//         OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "SET_STRIDE=%{public}d", ret);

	native_window_ = nativeWindow;
}

void ohosRender::OnFrame(uint8_t *data, int width, int height, int stride, int size, uint32_t orientation) {
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "ohosRender::OnFrame");
	if (width != width_ || height != height_ || orientation != orientation_) {
		width_ = width;
		height_ = height;
		orientation_ = orientation;
		ReConfigure();
	}

	OHNativeWindowBuffer *buffer = nullptr;
	int fenceFd;
	int32_t ret = -1;
	// 通过 OH_NativeWindow_NativeWindowRequestBuffer 获取 OHNativeWindowBuffer 实例
	ret = OH_NativeWindow_NativeWindowRequestBuffer(native_window_, &buffer, &fenceFd);
	if (ret != 0) {
		OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_DOMAIN, "mytest",
					 "OH_NativeWindow_NativeWindowRequestBuffer failed, ret=%{public}d", ret);
		return;
	}
	// 通过 OH_NativeWindow_GetBufferHandleFromNative 获取 buffer 的 handle
	BufferHandle *bufferHandle = OH_NativeWindow_GetBufferHandleFromNative(buffer);

	// 使用内存映射函数mmap将bufferHandle对应的共享内存映射到用户空间，可以通过映射出来的虚拟地址向bufferHandle中写入图像数据
	// bufferHandle->virAddr是bufferHandle在共享内存中的起始地址，bufferHandle->size是bufferHandle在共享内存中的内存占用大小
	void *mappedAddr =
		mmap(bufferHandle->virAddr, bufferHandle->size, PROT_READ | PROT_WRITE, MAP_SHARED, bufferHandle->fd, 0);
	if (mappedAddr == MAP_FAILED) {
		// mmap failed
	}

	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest",
				 "OnFrame::buffer width=%{public}d, height=%{public}d, stride=%{public}d, size=%{public}d",
				 bufferHandle->width, bufferHandle->height, bufferHandle->stride, bufferHandle->size);

	//         static uint32_t value = 0x00CBC0FF;
	//         value++;
	//         uint32_t *pixel = static_cast<uint32_t *>(mappedAddr); // 使用mmap获取到的地址来访问内存
	//         for (uint32_t x = 0; x < width; x++) {
	//             for (uint32_t y = 0;  y < height; y++) {
	//                 *pixel++ = value;
	//             }
	//         }
	uint8_t *pixel = static_cast<uint8_t *>(mappedAddr);
	uint8_t *src_data = static_cast<uint8_t *>(data);
	//         for (uint32_t i = 0; i < size; i++)
	//         {
	//             *pixel++ = *src_data++;
	//         }
	int byte_align = bufferHandle->stride - stride;
	for (uint32_t i = 0; i < height; i++) {
		uint8_t *pos = pixel + i * bufferHandle->stride;
		for (uint32_t j = 0; j < stride; j++) {
			*pos++ = *src_data++;
		}
	}

	for (uint32_t i = 0; i < height / 2; i++) {
		uint8_t *pos = pixel + (i + height) * bufferHandle->stride;
		for (uint32_t j = 0; j < stride / 2; j++) {
			*pos++ = *src_data++;
		}
		pos += byte_align / 2;
		for (uint32_t j = 0; j < stride / 2; j++) {
			*pos++ = *src_data++;
		}
	}

	// 设置刷新区域，如果Region中的Rect为nullptr,或者rectNumber为0，则认为OHNativeWindowBuffer全部有内容更改。
	Region region{nullptr, 0};
	// 通过OH_NativeWindow_NativeWindowFlushBuffer 提交给消费者使用，例如：显示在屏幕上。
	OH_NativeWindow_NativeWindowFlushBuffer(native_window_, buffer, fenceFd, region);

	// 内存使用完记得去掉内存映射
	int result = munmap(mappedAddr, bufferHandle->size);
	if (result == -1) {
		// munmap failed
	}
}

void ohosRender::ReConfigure() {
	int32_t code = SET_BUFFER_GEOMETRY;
	// 这里的nativeWindow是从上一步骤中的回调函数中获得的
	int32_t ret = OH_NativeWindow_NativeWindowHandleOpt(native_window_, code, width_, height_);

	code = SET_FORMAT;
	int32_t format = NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P;
	ret = OH_NativeWindow_NativeWindowHandleOpt(native_window_, code, format);
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "set window format=%{public}d", ret);

	code = SET_TRANSFORM;
	int32_t transform = NATIVEBUFFER_ROTATE_NONE;
	if (orientation_ == 90) {
		transform = NATIVEBUFFER_ROTATE_270;
	} else if (orientation_ == 270) {
		transform = NATIVEBUFFER_ROTATE_90;
	}
	// if mirror, use NATIVEBUFFER_FLIP_V_ROT90
	ret = OH_NativeWindow_NativeWindowHandleOpt(native_window_, code, transform);
	OH_LOG_Print(LOG_APP, LOG_INFO, LOG_DOMAIN, "mytest", "SET_TRANSFORM=%{public}d", ret);
}
} // namespace ohos
} // namespace webrtc