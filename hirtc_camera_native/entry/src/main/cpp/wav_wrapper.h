//
// Created on 2024/7/23.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef HIRTC_CAMERA_NATIVE_WAV_WRAPPER_H
#define HIRTC_CAMERA_NATIVE_WAV_WRAPPER_H

#include <string>
namespace AC {
class WavWapper {
public:
    WavWapper();
    ~WavWapper();
    /// <summary>
    /// 创建wav文件
    /// </summary>
    /// <param name="fileName">文件名</param>
    /// <param name="channels">声道数</param>
    /// <param name="sampleRate">采样率，单位hz</param>
    /// <param name="bitsPerSample">位深</param>
    void CreateWavFile(const std::string &fileName, int channels, int sampleRate, int bitsPerSample);
    /// <summary>
    /// 写入PCM数据
    /// </summary>
    /// <param name="data">PCM数据</param>
    /// <param name="dataLength">数据长度</param>
    void WriteToFile(unsigned char *data, int dataLength);
    /// <summary>
    /// 关闭文件
    /// </summary>
    void CloseFile();
    /// <summary>
    /// 打开wav文件
    /// </summary>
    /// <param name="fileName">文件名</param>
    int OpenWavFile(const std::string &fileName);
    /// <summary>
    /// 读取PCM数据
    /// </summary>
    /// <param name="data">待写入的buffer</param>
    /// <param name="dataLength">数据长度</param>
    void ReadToBuffer(void *data, int dataLength);
    /// <summary>
    /// 停止读取文件
    /// </summary>
    void StopReadFile();

private:
    void *_file = nullptr;
    uint32_t _totalDataLength = 0;
    int _channels;
    int _sampleRate;
    int _bitsPerSample;
    int _readCount = 0;
};
} // namespace AC

#endif //HIRTC_CAMERA_NATIVE_WAV_WRAPPER_H
