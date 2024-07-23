//
// Created on 2024/7/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef HIRTC_CAMERA_NATIVE_OHOS_AUDIO_H
#define HIRTC_CAMERA_NATIVE_OHOS_AUDIO_H

#include <cstdint>
#include <ohaudio/native_audiocapturer.h>
#include <ohaudio/native_audiostreambuilder.h>
#include <map>


int TestAudioStart();
int TestAudioStop();

int StartAudioReocrd();
int StopAudio();
int StartAudioPlay();

class BaseRenderer {
public:
    virtual int32_t writeData(void *buffer, int32_t length) = 0;
};

// 音频采集
class OhAudioCapturer {
private:
    OH_AudioStreamBuilder *builder = nullptr;
    OH_AudioCapturer *capturer = nullptr;
    OH_AudioStream_Type streamType;
    bool isStart = false;

public:
    static std::map<OH_AudioCapturer *, OhAudioCapturer *> capturerMap;
    static std::map<OhAudioCapturer *, BaseRenderer *> rendererMap;


public:
    static int32_t onReadData(OH_AudioCapturer *capturer, void *userData, void *buffer, int32_t length);
    static int32_t onStreamEvent(OH_AudioCapturer *capturer, void *userData, OH_AudioStream_Event event);
    static int32_t onInterruptEvent(OH_AudioCapturer *capturer, void *userData, OH_AudioInterrupt_ForceType type,
                                    OH_AudioInterrupt_Hint hint);
    static int32_t onError(OH_AudioCapturer *capturer, void *userData, OH_AudioStream_Result error);

public:
    void start();
    void stop();
};

// 音频播放
class OhAudioRenderer : public BaseRenderer {
private:
    OH_AudioStreamBuilder *builder = nullptr;
    OH_AudioRenderer *renderer = nullptr;
    OhAudioCapturer capturer;
    OH_AudioStream_Type streamType;
    bool isStart = false;

    int32_t bufferMax = 192000;
    uint8_t *buffer = new uint8_t[192000];
    int32_t pBegin = 0;
    int32_t pEnd = 0;

public:
    static std::map<OH_AudioRenderer *, OhAudioRenderer *> rendererMap;

public:
    static int32_t onWriteData(OH_AudioRenderer *renderer, void *userData, void *buffer, int32_t lenth);
    static int32_t onStreamEvent(OH_AudioRenderer *renderer, void *userData, OH_AudioStream_Event event);
    static int32_t onInterruptEvent(OH_AudioRenderer *renderer, void *userData, OH_AudioInterrupt_ForceType type,
                                    OH_AudioInterrupt_Hint hint);
    static int32_t onError(OH_AudioRenderer *renderer, void *userData, OH_AudioStream_Result error);

public:
    int32_t writeData(void *buffer, int32_t length);
    void start(OhAudioCapturer *capturer);
    void stop();
};

#endif //HIRTC_CAMERA_NATIVE_OHOS_AUDIO_H
