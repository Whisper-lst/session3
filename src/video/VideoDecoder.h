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

namespace ve {

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();
    
    bool open(const std::string& filename);
    void close();
    
    bool seek(TimePoint timestamp);
    bool decodeNextFrame(VideoFrame& outFrame);
    bool decodeFrameAt(TimePoint timestamp, VideoFrame& outFrame);
    
    const MediaInfo& getMediaInfo() const { return m_mediaInfo; }
    bool isOpen() const { return m_isOpen; }
    
    int getWidth() const { return m_mediaInfo.video_params.width; }
    int getHeight() const { return m_mediaInfo.video_params.height; }
    double getFPS() const { return m_mediaInfo.video_params.fps; }
    Duration getDuration() const { return m_mediaInfo.duration; }
    
    PixelFormat getPixelFormat() const { return m_mediaInfo.video_params.format; }

private:
    bool initCodec();
    void cleanup();
    bool readPacket();
    bool sendPacket();
    bool receiveFrame(AVFrame* frame);
    
    PixelFormat convertPixelFormat(int avFormat);
    TimePoint convertTimestamp(int64_t pts, AVRational timebase);
    Duration convertDuration(int64_t duration, AVRational timebase);
    
    int calculateFrameSize(const VideoFrame& frame) const;
    void copyFrameData(VideoFrame& dest, const VideoFrame& src, std::vector<uint8_t>& buffer);
    
    AVFormatContext* m_formatContext = nullptr;
    AVCodecContext* m_codecContext = nullptr;
    const AVCodec* m_codec = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;
    SwsContext* m_swsContext = nullptr;
    
    int m_videoStreamIndex = -1;
    int m_audioStreamIndex = -1;
    
    MediaInfo m_mediaInfo;
    std::atomic<bool> m_isOpen{false};
    std::mutex m_mutex;
    
    std::vector<uint8_t> m_conversionBuffer;
    std::vector<uint8_t> m_frameBuffer;
    
    int getFrameBufferSize() const;
};

}
