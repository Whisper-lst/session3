#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <chrono>

namespace ve {

using TimePoint = std::chrono::microseconds;
using Duration = std::chrono::microseconds;

enum class PixelFormat {
    Unknown,
    RGBA8888,
    BGRA8888,
    RGB888,
    YUV420P,
    YUV422P,
    YUV444P,
    NV12,
    NV21
};

enum class AudioFormat {
    Unknown,
    S16,
    S32,
    F32,
    U8
};

struct VideoFrame {
    uint8_t* data[4] = {nullptr, nullptr, nullptr, nullptr};
    int linesize[4] = {0, 0, 0, 0};
    int width = 0;
    int height = 0;
    PixelFormat format = PixelFormat::Unknown;
    TimePoint timestamp;
    Duration duration;
    bool keyframe = false;
};

struct AudioFrame {
    uint8_t* data[8] = {nullptr};
    int sample_rate = 0;
    int channels = 0;
    int samples = 0;
    AudioFormat format = AudioFormat::Unknown;
    TimePoint timestamp;
};

struct VideoParams {
    int width = 1920;
    int height = 1080;
    double fps = 30.0;
    PixelFormat format = PixelFormat::YUV420P;
    int64_t bitrate = 5000000;
};

struct AudioParams {
    int sample_rate = 48000;
    int channels = 2;
    AudioFormat format = AudioFormat::S16;
    int64_t bitrate = 192000;
};

struct MediaInfo {
    std::string filename;
    Duration duration;
    VideoParams video_params;
    AudioParams audio_params;
    bool has_video = false;
    bool has_audio = false;
};

}
