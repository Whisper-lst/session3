#pragma once

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

#include "Types.h"
#include "Logger.h"

struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct AVRational;
struct AVStream;

namespace ve {

class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();
    
    bool open(const std::string& filename, const VideoParams& videoParams, const AudioParams& audioParams);
    void close();
    
    bool writeVideoFrame(const VideoFrame& frame);
    bool writeAudioFrame(const AudioFrame& frame);
    bool flush();
    
    const VideoParams& getVideoParams() const { return m_videoParams; }
    const AudioParams& getAudioParams() const { return m_audioParams; }
    bool isOpen() const { return m_isOpen; }

private:
    bool initVideoCodec();
    bool initAudioCodec();
    bool openOutputFile(const std::string& filename);
    void cleanup();
    
    int convertPixelFormat(PixelFormat format);
    int convertAudioFormat(AudioFormat format);
    int64_t convertTimestamp(TimePoint timestamp, AVRational timebase);
    
    AVFormatContext* m_formatContext = nullptr;
    AVCodecContext* m_videoCodecContext = nullptr;
    AVCodecContext* m_audioCodecContext = nullptr;
    const AVCodec* m_videoCodec = nullptr;
    const AVCodec* m_audioCodec = nullptr;
    AVStream* m_videoStream = nullptr;
    AVStream* m_audioStream = nullptr;
    AVFrame* m_videoFrame = nullptr;
    AVFrame* m_audioFrame = nullptr;
    AVPacket* m_packet = nullptr;
    SwsContext* m_swsContext = nullptr;
    
    VideoParams m_videoParams;
    AudioParams m_audioParams;
    std::atomic<bool> m_isOpen{false};
    std::mutex m_mutex;
    
    int64_t m_videoFrameCount = 0;
    int64_t m_audioFrameCount = 0;
};

}
