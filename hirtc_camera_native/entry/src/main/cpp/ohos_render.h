//
// Created on 2024/6/24.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef HIRTC_CAMERA_NATIVE_OHOS_RENDER_H
#define HIRTC_CAMERA_NATIVE_OHOS_RENDER_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include <ace/xcomponent/native_interface_xcomponent.h>
#include "napi/native_api.h"
#include <native_window/external_window.h>

namespace webrtc {
namespace ohos {

void OnSurfaceCreatedCB(OH_NativeXComponent *component, void *window);
void OnSurfaceChangedCB(OH_NativeXComponent *component, void *window);
void OnSurfaceDestroyedCB(OH_NativeXComponent *component, void *window);
void DispatchTouchEventCB(OH_NativeXComponent *component, void *window);

class PluginManager {
public:
	~PluginManager() {
		nativeXComponentMap_.clear();
		pluginRenderMap_.clear();
	};

	static PluginManager *GetInstance() {
		static PluginManager pluginManager;
		return &pluginManager;
	}

	void SetNativeXComponent(std::string &id, OH_NativeXComponent *nativeXComponent);
	void SetNativeWindow(std::string &id, OHNativeWindow *nativeWindow);
	OHNativeWindow *GetRender(std::string &id);
	void Export(napi_env env, napi_value exports);

private:
	PluginManager(){};
	std::unordered_map<std::string, OH_NativeXComponent *> nativeXComponentMap_;
	std::unordered_map<std::string, OHNativeWindow *> pluginRenderMap_;
};


class ohosRender {
public:
	ohosRender(std::string &id, OHNativeWindow *nativeWindow);

	void OnFrame(uint8_t *data, int width, int height, int stride, int size, uint32_t orientation);

	void ReConfigure();

private:
	int width_{0};
	int height_{0};
	uint32_t orientation_{0};
	OHNativeWindow *native_window_{nullptr};
};
} // namespace ohos
} // namespace webrtc

#endif // HIRTC_CAMERA_NATIVE_OHOS_RENDER_H
