#pragma once

#include <string>
#include <memory>
#include <vector>

#include "Types.h"
#include "Logger.h"

struct SwsContext;
struct SwrContext;

namespace ve {

class FrameConverter {
public:
    FrameConverter();
    ~FrameConverter();
    
    bool initVideoConversion(int srcWidth, int srcHeight, PixelFormat srcFormat,
                              int dstWidth, int dstHeight, PixelFormat dstFormat);
    
    bool initAudioConversion(int srcSampleRate, int srcChannels, AudioFormat srcFormat,
                              int dstSampleRate, int dstChannels, AudioFormat dstFormat);
    
    bool convertVideo(const VideoFrame& srcFrame, VideoFrame& dstFrame);
    bool convertAudio(const AudioFrame& srcFrame, AudioFrame& dstFrame);
    
    void reset();
    
    bool isVideoInitialized() const { return m_videoInitialized; }
    bool isAudioInitialized() const { return m_audioInitialized; }

private:
    void cleanup();
    
    int toAVPixelFormat(PixelFormat format);
    int toAVSampleFormat(AudioFormat format);
    
    SwsContext* m_swsContext = nullptr;
    SwrContext* m_swrContext = nullptr;
    
    int m_srcWidth = 0;
    int m_srcHeight = 0;
    int m_dstWidth = 0;
    int m_dstHeight = 0;
    PixelFormat m_srcFormat = PixelFormat::Unknown;
    PixelFormat m_dstFormat = PixelFormat::Unknown;
    
    int m_srcSampleRate = 0;
    int m_dstSampleRate = 0;
    int m_srcChannels = 0;
    int m_dstChannels = 0;
    AudioFormat m_srcAudioFormat = AudioFormat::Unknown;
    AudioFormat m_dstAudioFormat = AudioFormat::Unknown;
    
    std::vector<uint8_t> m_videoBuffer;
    std::vector<uint8_t> m_audioBuffer;
    
    bool m_videoInitialized = false;
    bool m_audioInitialized = false;
};

}
