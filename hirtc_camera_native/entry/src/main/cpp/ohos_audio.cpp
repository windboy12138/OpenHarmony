//
// Created on 2024/7/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".
#include "hilog/log.h"

#include "ohos_audio.h"
#include "wav_wrapper.h"
#include <cstdint>
#include <mutex>

#undef LOG_TAG
#define LOG_TAG "mytest"

#define RECORD_FILE 1

int32_t myIndex = 0;
std::mutex mtx; // 创建一个互斥量
std::map<OH_AudioCapturer *, OhAudioCapturer *> OhAudioCapturer::capturerMap;
std::map<OhAudioCapturer *, BaseRenderer *> OhAudioCapturer::rendererMap;

OhAudioCapturer *globalOhAudioCapturer;
OhAudioRenderer *globalOhAudioRenderer;

AC::WavWapper wav_file;
std::string file_name = "/data/storage/el2/base/haps/entry/files_wav/recorder.wav";

int TestAudioStart() {
    OH_LOG_INFO(LOG_APP, "%{public}s", "[TestAudio]");

    OhAudioCapturer *ohAudioCapturer = new OhAudioCapturer();
    OhAudioRenderer *ohAudioRenderer = new OhAudioRenderer();
    ohAudioCapturer->start();
    ohAudioRenderer->start(ohAudioCapturer);

    globalOhAudioCapturer = ohAudioCapturer;
    globalOhAudioRenderer = ohAudioRenderer;
    
#if RECORD_FILE
    wav_file.CreateWavFile(file_name, 2, 48000, 16);
#else
    int ret = wav_file.OpenWavFile(file_name);
    OH_LOG_INFO(LOG_APP, "OpenWavFile ret=%{public}d", ret);
#endif

    return 0;
}

int TestAudioStop() {
    OH_LOG_INFO(LOG_APP, "%{public}s", "[TestAudioStop]");
    globalOhAudioCapturer->stop();
    globalOhAudioRenderer->stop();

#if RECORD_FILE
    wav_file.CloseFile();
#else
    wav_file.StopReadFile();
#endif
    return 0;
}

int StartAudioReocrd()
{
    OhAudioCapturer *ohAudioCapturer = new OhAudioCapturer();
    ohAudioCapturer->start();
    globalOhAudioCapturer = ohAudioCapturer;
    wav_file.CreateWavFile(file_name, 2, 48000, 16);
    return 0;
}
int StopAudio()
{
    if (globalOhAudioCapturer)
    {
        OH_LOG_INFO(LOG_APP, "%{public}s", "[Stop Audio Capture]");
        globalOhAudioCapturer->stop();
        wav_file.CloseFile();
        delete globalOhAudioCapturer;
        globalOhAudioCapturer = nullptr;
    }
    
    if (globalOhAudioRenderer)
    {
        OH_LOG_INFO(LOG_APP, "%{public}s", "[Stop Audio Renderer]");
        globalOhAudioRenderer->stop();
        wav_file.StopReadFile();
        delete globalOhAudioRenderer;
        globalOhAudioRenderer = nullptr;
    }

    return 0;
}
int StartAudioPlay()
{
    OhAudioRenderer *ohAudioRenderer = new OhAudioRenderer();
    ohAudioRenderer->start(globalOhAudioCapturer);
    globalOhAudioRenderer = ohAudioRenderer;
    int ret = wav_file.OpenWavFile(file_name);
    OH_LOG_INFO(LOG_APP, "OpenWavFile ret=%{public}d", ret);
    return ret;
}


// 音频采集
void OhAudioCapturer::start() {
    OH_LOG_INFO(LOG_APP, "%{public}s", "OhAudioCapturer start begin");
    if (this->isStart) {
        return;
    }
    OH_AudioStream_Result ret;

    // 创建构造器
    ret = OH_AudioStreamBuilder_Create(&(this->builder), AUDIOSTREAM_TYPE_CAPTURER);

    if (ret != AUDIOSTREAM_SUCCESS) {
        OH_LOG_INFO(LOG_APP, "%{public}s", "OH_AudioStreamBuilder_Create fail");
        return;
    }

    // 设置音频采样率
    OH_AudioStreamBuilder_SetSamplingRate(this->builder, 48000);
    // 设置音频声道
    OH_AudioStreamBuilder_SetChannelCount(this->builder, 2);
    // 设置音频采样格式
    //     OH_AudioStreamBuilder_SetSampleFormat(this->builder, AUDIOSTREAM_SAMPLE_S24LE);
    //     OH_AudioStreamBuilder_SetSampleFormat(this->builder, AUDIOSTREAM_SAMPLE_S16LE);
    // AUDIOSTREAM_SAMPLE_U8
    OH_AudioStreamBuilder_SetSampleFormat(this->builder, AUDIOSTREAM_SAMPLE_S16LE);
    OH_AudioStreamBuilder_SetLatencyMode(this->builder, AUDIOSTREAM_LATENCY_MODE_FAST);
    // 设置音频流的编码类型
//    OH_AudioStreamBuilder_SetEncodingType(this->builder, AUDIOSTREAM_ENCODING_TYPE_RAW);
    // 设置输入音频流的工作场景
//    OH_AudioStreamBuilder_SetCapturerInfo(this->builder, AUDIOSTREAM_SOURCE_TYPE_MIC);

    // 设置回调函数
    OH_AudioCapturer_Callbacks callbacks;
    callbacks.OH_AudioCapturer_OnReadData = OhAudioCapturer::onReadData;
    callbacks.OH_AudioCapturer_OnStreamEvent = OhAudioCapturer::onStreamEvent;
    callbacks.OH_AudioCapturer_OnInterruptEvent = OhAudioCapturer::onInterruptEvent;
    callbacks.OH_AudioCapturer_OnError = OhAudioCapturer::onError;

    ret = OH_AudioStreamBuilder_SetCapturerCallback(this->builder, callbacks, nullptr);

    if (ret != AUDIOSTREAM_SUCCESS) {
        OH_LOG_INFO(LOG_APP, "%{public}s", "OH_AudioStreamBuilder_SetCapturerCallback fail");
        return;
    }

    ret = OH_AudioStreamBuilder_GenerateCapturer(this->builder, &(this->capturer));

    if (ret != AUDIOSTREAM_SUCCESS) {
        OH_LOG_INFO(LOG_APP, "%{public}s", "OH_AudioStreamBuilder_GenerateCapturer fail");
        return;
    }

    ret = OH_AudioCapturer_Start(this->capturer);

    if (ret != AUDIOSTREAM_SUCCESS) {
        OH_LOG_INFO(LOG_APP, "%{public}s", "OH_AudioCapturer_Start fail");
        return;
    }

    OhAudioCapturer::capturerMap[this->capturer] = this;

    this->isStart = true;

    OH_LOG_INFO(LOG_APP, "%{public}s, capturer: %{public}i", "OhAudioCapturer start end", this->capturer);
}

void OhAudioCapturer::stop() {
    if (this->isStart) {
        OH_AudioCapturer_Stop(this->capturer);
        OH_AudioStreamBuilder_Destroy(this->builder);
        this->isStart = false;
    }
}

int32_t OhAudioCapturer::onReadData(OH_AudioCapturer *capturer, void *userData, void *buffer, int32_t length) {
    OhAudioCapturer *ohAudioCapturer = OhAudioCapturer::capturerMap[capturer];
    BaseRenderer *renderer = OhAudioCapturer::rendererMap[ohAudioCapturer];

#if RECORD_FILE
    wav_file.WriteToFile(reinterpret_cast<unsigned char *>(buffer), length);
#endif
    if (renderer) {
        OH_LOG_INFO(LOG_APP, "can save: %{public}i, %{public}i", renderer, length);
//        renderer->writeData(buffer, length);
    }
    return 0;
}

int32_t OhAudioCapturer::onStreamEvent(OH_AudioCapturer *capturer, void *userData, OH_AudioStream_Event event) {
    OH_LOG_INFO(LOG_APP, "%{public}s", "OnStreamEvent");
    //     OH_LOG_INFO(LOG_APP, "OnStreamEvent %{public}i", event);
    //     static_cast<OhAudioCapturer>(*userData).buffer = buffer;
    //     *userData.bufferLength = length;
    return 0;
}

int32_t OhAudioCapturer::onInterruptEvent(OH_AudioCapturer *capturer, void *userData, OH_AudioInterrupt_ForceType type,
                                          OH_AudioInterrupt_Hint hint) {
    OH_LOG_INFO(LOG_APP, "%{public}s", "onInterruptEvent");
    //     OH_LOG_INFO(LOG_APP, "onInterruptEvent, type: %{public}i, hint: %{public}i", type, hint);
    //     static_cast<OhAudioCapturer>(*userData).buffer = buffer;
    //     *userData.bufferLength = length;
    return 0;
}

int32_t OhAudioCapturer::onError(OH_AudioCapturer *capturer, void *userData, OH_AudioStream_Result error) {
    OH_LOG_INFO(LOG_APP, "%{public}s", "onError");
    //     OH_LOG_INFO(LOG_APP, "onError %{public}i", error);
    //     static_cast<OhAudioCapturer>(*userData).buffer = buffer;
    //     *userData.bufferLength = length;
    return 0;
}

// 音频播放
std::map<OH_AudioRenderer *, OhAudioRenderer *> OhAudioRenderer::rendererMap;

void OhAudioRenderer::start(OhAudioCapturer *capturer) {
    OH_LOG_INFO(LOG_APP, "%{public}s", "OhAudioRenderer start begin");

    if (this->isStart) {
        OH_LOG_INFO(LOG_APP, "%{public}s", "OhAudioRenderer start begin");
        return;
    }

    // 创建构造器
    OH_AudioStream_Result ret;
    ret = OH_AudioStreamBuilder_Create(&(this->builder), AUDIOSTREAM_TYPE_RENDERER);

    if (ret != AUDIOSTREAM_SUCCESS) {
        OH_LOG_INFO(LOG_APP, "%{public}s", "OH_AudioStreamBuilder_Create fail");
        return;
    }

    // 设置音频采样率
    OH_AudioStreamBuilder_SetSamplingRate(this->builder, 48000);
    // 设置音频声道
    OH_AudioStreamBuilder_SetChannelCount(this->builder, 2);
    // 设置音频采样格式
    //     OH_AudioStreamBuilder_SetSampleFormat(this->builder, AUDIOSTREAM_SAMPLE_S24LE);
    //     OH_AudioStreamBuilder_SetSampleFormat(this->builder, AUDIOSTREAM_SAMPLE_S16LE);
    // AUDIOSTREAM_SAMPLE_U8
    OH_AudioStreamBuilder_SetSampleFormat(this->builder, AUDIOSTREAM_SAMPLE_S16LE);
    OH_AudioStreamBuilder_SetLatencyMode(this->builder, AUDIOSTREAM_LATENCY_MODE_FAST);
    // 设置音频流的编码类型
    OH_AudioStreamBuilder_SetEncodingType(this->builder, AUDIOSTREAM_ENCODING_TYPE_RAW);
    // 设置输出音频流的工作场景
             OH_AudioStreamBuilder_SetRendererInfo(builder, AUDIOSTREAM_USAGE_MUSIC);
//    OH_AudioStreamBuilder_SetRendererInfo(this->builder, AUDIOSTREAM_USAGE_VOICE_COMMUNICATION);

    OH_AudioRenderer_Callbacks callbacks;
    // 配置回调函数
    callbacks.OH_AudioRenderer_OnWriteData = OhAudioRenderer::onWriteData;
    callbacks.OH_AudioRenderer_OnStreamEvent = OhAudioRenderer::onStreamEvent;
    callbacks.OH_AudioRenderer_OnInterruptEvent = OhAudioRenderer::onInterruptEvent;
    callbacks.OH_AudioRenderer_OnError = OhAudioRenderer::onError;

    // 设置输出音频流的回调
    ret = OH_AudioStreamBuilder_SetRendererCallback(this->builder, callbacks, nullptr);

    if (ret != AUDIOSTREAM_SUCCESS) {
        OH_LOG_INFO(LOG_APP, "%{public}s", "OH_AudioStreamBuilder_SetRendererCallback fail");
        return;
    }

    ret = OH_AudioStreamBuilder_GenerateRenderer(this->builder, &(this->renderer));

    if (ret != AUDIOSTREAM_SUCCESS) {
        OH_LOG_INFO(LOG_APP, "%{public}s", "OH_AudioStreamBuilder_GenerateRenderer fail");
        return;
    }

    ret = OH_AudioRenderer_Start(this->renderer);

    if (ret != AUDIOSTREAM_SUCCESS) {
        OH_LOG_INFO(LOG_APP, "%{public}s", "OH_AudioRenderer_Start fail");
        return;
    }

    OhAudioRenderer::rendererMap[this->renderer] = this;
    OhAudioCapturer::rendererMap[capturer] = this;

    this->isStart = true;

    OH_LOG_INFO(LOG_APP, "%{public}s %{public}i", "OhAudioRenderer start end", this->renderer);
}

void OhAudioRenderer::stop() {
    if (this->isStart) {
        OH_AudioRenderer_Stop(this->renderer);
        OH_AudioStreamBuilder_Destroy(this->builder);
        this->isStart = false;
    }
}

int32_t OhAudioRenderer::writeData(void *buffer, int32_t length) {
    mtx.lock();
    OH_LOG_INFO(LOG_APP, "save data begin %{public}i-%{public}i-%{public}i-%{public}i", this->pBegin, this->pEnd,
                length, this->buffer);
    if (this->pEnd + length > this->bufferMax) {
        OH_LOG_INFO(LOG_APP, "save data full %{public}i-%{public}i", this->pBegin, this->pEnd);

        memcpy(this->buffer, this->buffer + this->pBegin, this->pEnd - this->pBegin);
        this->pEnd -= this->pBegin;
        this->pBegin = 0;
    }
    memcpy(this->buffer + this->pEnd, buffer, length);
    this->pEnd += length;
    OH_LOG_INFO(LOG_APP, "save data end %{public}i-%{public}i", this->pBegin, this->pEnd);
    mtx.unlock();
    return 0;
}

int32_t OhAudioRenderer::onWriteData(OH_AudioRenderer *renderer, void *userData, void *buffer, int32_t length) {
//    OH_LOG_INFO(LOG_APP, "can write to %{public}d, len=%{public}d", renderer, length);
    OhAudioRenderer *ohAudioRenderer = OhAudioRenderer::rendererMap[renderer];

    wav_file.ReadToBuffer(buffer, length);
    return 0;
#if ! RECORD_FILE
    wav_file.ReadToBuffer(buffer, length);
    return 0;
#endif

    if (ohAudioRenderer) {
        mtx.lock();
        OH_LOG_INFO(LOG_APP, "can write %{public}i, %{public}s, %{public}i", ohAudioRenderer,
                    ohAudioRenderer->isStart ? "true" : "false", ohAudioRenderer->buffer);
        if (! ohAudioRenderer->isStart) {
            return 0;
        }
            
//        length = length * sizeof(int16_t);
        int32_t remain = ohAudioRenderer->pEnd - ohAudioRenderer->pBegin;
        if (remain >= length) {
            OH_LOG_INFO(
                LOG_APP,
                "write data begin, pBegin: %{public}i, pEnd: %{public}i, remain: %{public}i, length: %{public}i",
                ohAudioRenderer->pBegin, ohAudioRenderer->pEnd, remain, length);
            memcpy(buffer, ohAudioRenderer->buffer + ohAudioRenderer->pBegin, length);
            ohAudioRenderer->pBegin += length;
            OH_LOG_INFO(LOG_APP,
                        "write data end,  pBegin: %{public}i, pEnd: %{public}i, remain: %{public}i, length: %{public}i",
                        ohAudioRenderer->pBegin, ohAudioRenderer->pEnd, ohAudioRenderer->pEnd - ohAudioRenderer->pBegin,
                        length);
        } else {
            OH_LOG_INFO(LOG_APP, "can write not enough data: %{public}i - %{public}i", remain, length);
        }
        mtx.unlock();
    }

    return 0;
}

int32_t OhAudioRenderer::onStreamEvent(OH_AudioRenderer *renderer, void *userData, OH_AudioStream_Event event) {
    OH_LOG_INFO(LOG_APP, "%{public}s", "onStreamEvent");
    return 0;
}

int32_t OhAudioRenderer::onInterruptEvent(OH_AudioRenderer *renderer, void *userData, OH_AudioInterrupt_ForceType type,
                                          OH_AudioInterrupt_Hint hint) {
    OH_LOG_INFO(LOG_APP, "%{public}s", "onInterruptEvent");
    return 0;
}

int32_t OhAudioRenderer::onError(OH_AudioRenderer *renderer, void *userData, OH_AudioStream_Result error) {
    OH_LOG_INFO(LOG_APP, "%{public}s", "onError");
    return 0;
}