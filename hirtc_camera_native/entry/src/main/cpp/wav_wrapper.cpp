//
// Created on 2024/7/23.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "wav_wrapper.h"
#include <cstdint>
#include <hilog/log.h>
#include <stdio.h>

#undef LOG_TAG
#define LOG_TAG "mytest"

namespace AC {
// WAV头部结构-PCM格式
struct WavPCMFileHeader {
    struct RIFF {
        const char rift[4] = {'R', 'I', 'F', 'F'};
        uint32_t fileLength;
        const char wave[4] = {'W', 'A', 'V', 'E'};
    } riff;
    struct Format {
        const char fmt[4] = {'f', 'm', 't', ' '};
        uint32_t blockSize = 16;
        uint16_t formatTag;
        uint16_t channels;
        uint32_t samplesPerSec;
        uint32_t avgBytesPerSec;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
    } format;
    struct Data {
        const char data[4] = {'d', 'a', 't', 'a'};
        uint32_t dataLength;
    } data;
    WavPCMFileHeader() {}
    WavPCMFileHeader(int nCh, int nSampleRate, int bitsPerSample, int dataSize) {
        riff.fileLength = 36 + dataSize;
        format.formatTag = 1;
        format.channels = nCh;
        format.samplesPerSec = nSampleRate;
        format.avgBytesPerSec = nSampleRate * nCh * bitsPerSample / 8;
        format.blockAlign = nCh * bitsPerSample / 8;
        format.bitsPerSample = bitsPerSample;
        data.dataLength = dataSize;
    }
};
WavWapper::WavWapper() {}

WavWapper::~WavWapper() { CloseFile(); }

void WavWapper::CreateWavFile(const std::string &fileName, int channels, int sampleRate, int bitsPerSample) {
    if (! _file) {
        _channels = channels;
        _sampleRate = sampleRate;
        _bitsPerSample = bitsPerSample;
        _totalDataLength = 0;
        _file = fopen(fileName.c_str(), "wb+");
        // 预留头部位置
        fseek(static_cast<FILE *>(_file), sizeof(WavPCMFileHeader), SEEK_SET);
    }
}
void WavWapper::WriteToFile(unsigned char *data, int dataLength) {
    fwrite(data, 1, dataLength, static_cast<FILE *>(_file));
    _totalDataLength += dataLength;
}
void WavWapper::CloseFile() {
    if (_file) {
        if (_totalDataLength > 0) {
            // 写入头部信息
            fseek(static_cast<FILE *>(_file), 0, SEEK_SET);
            WavPCMFileHeader h(_channels, _sampleRate, _bitsPerSample, _totalDataLength);
            fwrite(&h, 1, sizeof(h), static_cast<FILE *>(_file));
        }
        fclose(static_cast<FILE *>(_file));
        _file = nullptr;
    }
}

int WavWapper::OpenWavFile(const std::string &fileName)
{
   	//第零步：从硬盘读取文件
    FILE* fp;
    fp = fopen(fileName.c_str(), "rb");
    if (!fp)
    {
        OH_LOG_INFO(LOG_APP, "fopen file failed");
        return -1;
    }
    
 
	//第一步：对RIFF区块的读取
	char RIFF_id[5];         // 5个字节存储空间存储'RIFF'和'\0'，这个是为方便利用strcmp
	fread(RIFF_id, sizeof(char), 4, fp); // 读取'RIFF'
	RIFF_id[4] = '\0';
	if (strcmp(RIFF_id, "RIFF"))//零	这两个字符串相等。
	{
        OH_LOG_INFO(LOG_APP, "strcmp RIFF failed");
        return -1;
	}
	//
	unsigned long RIFF_size;  // 存储文件大小
	fread(&RIFF_size, sizeof(uint32_t), 1, fp); // 读取文件大小
	//
	char RIFF_Type[5];         // 5个字节存储空间存储'RIFF'和'\0'，这个是为方便利用strcmp
	fread(RIFF_Type, sizeof(char), 4, fp); // 读取'RIFF'
	RIFF_Type[4] = '\0';
	if (strcmp(RIFF_Type, "WAVE"))
	{
        OH_LOG_INFO(LOG_APP, "strcmp WAVE failed");
        return -1;
	}
 
	//第二步：FORMAT区块
	short format_Audio, Format_channels, format_block_align, format_bits_per_sample;    // 16位数据
	unsigned long format_length, format_sample_rate, format_bytes_sec   ; // 32位数据
 
	char FORMAT_id[5];
	fread(FORMAT_id, sizeof(char), 4, fp);     // 读取4字节 "fmt ";
	FORMAT_id[4] = '\0';
	fread(&format_length, sizeof(uint32_t), 1, fp);//Size表示该区块数据的长度（不包含ID和Size的长度）
	fread(&format_Audio, sizeof(short), 1, fp); // 读取文件tag  音频格式
	fread(&Format_channels, sizeof(short), 1, fp);    // 读取通道数目
	fread(&format_sample_rate, sizeof(uint32_t), 1, fp);   // 读取采样率大小  
	fread(&format_bytes_sec, sizeof(uint32_t), 1, fp); // 每秒数据字节数= SampleRate * NumChannels * BitsPerSample / 8
	fread(&format_block_align, sizeof(short), 1, fp);     //每个采样所需的字节数 = NumChannels * BitsPerSample / 8
	fread(&format_bits_per_sample, sizeof(short), 1, fp);      // 每个采样存储的bit数，8：8bit，16：16bit，32：32bit
 
	//第三步： DATA区块
	char DATA_id[5];
	fread(DATA_id, sizeof(char), 4, fp);                     // 读入'data'
	DATA_id[4] = '\0';
	if (strcmp(DATA_id, "data"))
	{
        OH_LOG_INFO(LOG_APP, "strcmp data failed");
        return -1;
	}
	//
	unsigned long gsz_data_size;//
	fread(&gsz_data_size, sizeof(unsigned long), 1, fp);    // 读取数据大小
	_file = fp;
    _totalDataLength = gsz_data_size;
    return 0;
//	char* gsz_data_data = new char[gsz_data_size];// 读取数据
//	fread(gsz_data_data, sizeof(char), gsz_data_size, fp);      
//	gsz_data_data[gsz_data_size] = '\0';
}

void WavWapper::ReadToBuffer(void *data, int dataLength)
{
    if (_file) {
        if ((_readCount + dataLength) > _totalDataLength) {
            return;
        }
        fread(data, sizeof(char), dataLength, static_cast<FILE *>(_file));
        _readCount += dataLength;
    }
}

void WavWapper::StopReadFile()
{
    if (_file) {
        fclose(static_cast<FILE *>(_file));
        _file = nullptr;
        _readCount = 0;
    }
}

} // namespace AC
