#include "FrameConverter.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

namespace ve {

FrameConverter::FrameConverter() {
    LOG_DEBUG("FrameConverter created");
}

FrameConverter::~FrameConverter() {
    cleanup();
    LOG_DEBUG("FrameConverter destroyed");
}

void FrameConverter::cleanup() {
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    
    if (m_swrContext) {
        swr_free(&m_swrContext);
        m_swrContext = nullptr;
    }
    
    m_videoInitialized = false;
    m_audioInitialized = false;
}

void FrameConverter::reset() {
    cleanup();
}

bool FrameConverter::initVideoConversion(int srcWidth, int srcHeight, PixelFormat srcFormat,
                                           int dstWidth, int dstHeight, PixelFormat dstFormat) {
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    
    m_srcWidth = srcWidth;
    m_srcHeight = srcHeight;
    m_dstWidth = dstWidth;
    m_dstHeight = dstHeight;
    m_srcFormat = srcFormat;
    m_dstFormat = dstFormat;
    
    int srcAVFormat = toAVPixelFormat(srcFormat);
    int dstAVFormat = toAVPixelFormat(dstFormat);
    
    if (srcAVFormat == AV_PIX_FMT_NONE || dstAVFormat == AV_PIX_FMT_NONE) {
        LOG_ERROR("Unsupported pixel format");
        return false;
    }
    
    m_swsContext = sws_getContext(
        srcWidth, srcHeight, static_cast<AVPixelFormat>(srcAVFormat),
        dstWidth, dstHeight, static_cast<AVPixelFormat>(dstAVFormat),
        SWS_BICUBIC, nullptr, nullptr, nullptr
    );
    
    if (!m_swsContext) {
        LOG_ERROR("Failed to create sws context");
        return false;
    }
    
    int dstBufferSize = av_image_get_buffer_size(static_cast<AVPixelFormat>(dstAVFormat), 
                                                   dstWidth, dstHeight, 32);
    m_videoBuffer.resize(dstBufferSize);
    
    m_videoInitialized = true;
    LOG_INFO("Video converter initialized: %dx%d -> %dx%d", 
             srcWidth, srcHeight, dstWidth, dstHeight);
    
    return true;
}

bool FrameConverter::initAudioConversion(int srcSampleRate, int srcChannels, AudioFormat srcFormat,
                                           int dstSampleRate, int dstChannels, AudioFormat dstFormat) {
    if (m_swrContext) {
        swr_free(&m_swrContext);
        m_swrContext = nullptr;
    }
    
    m_srcSampleRate = srcSampleRate;
    m_dstSampleRate = dstSampleRate;
    m_srcChannels = srcChannels;
    m_dstChannels = dstChannels;
    m_srcAudioFormat = srcFormat;
    m_dstAudioFormat = dstFormat;
    
    int srcAVFormat = toAVSampleFormat(srcFormat);
    int dstAVFormat = toAVSampleFormat(dstFormat);
    
    if (srcAVFormat == AV_SAMPLE_FMT_NONE || dstAVFormat == AV_SAMPLE_FMT_NONE) {
        LOG_ERROR("Unsupported audio format");
        return false;
    }
    
    m_swrContext = swr_alloc();
    if (!m_swrContext) {
        LOG_ERROR("Failed to allocate swr context");
        return false;
    }
    
    av_opt_set_int(m_swrContext, "in_channel_count", srcChannels, 0);
    av_opt_set_int(m_swrContext, "in_sample_rate", srcSampleRate, 0);
    av_opt_set_sample_fmt(m_swrContext, "in_sample_fmt", static_cast<AVSampleFormat>(srcAVFormat), 0);
    
    av_opt_set_int(m_swrContext, "out_channel_count", dstChannels, 0);
    av_opt_set_int(m_swrContext, "out_sample_rate", dstSampleRate, 0);
    av_opt_set_sample_fmt(m_swrContext, "out_sample_fmt", static_cast<AVSampleFormat>(dstAVFormat), 0);
    
    if (swr_init(m_swrContext) < 0) {
        LOG_ERROR("Failed to initialize swr context");
        swr_free(&m_swrContext);
        return false;
    }
    
    m_audioInitialized = true;
    LOG_INFO("Audio converter initialized: %dHz %dch -> %dHz %dch", 
             srcSampleRate, srcChannels, dstSampleRate, dstChannels);
    
    return true;
}

bool FrameConverter::convertVideo(const VideoFrame& srcFrame, VideoFrame& dstFrame) {
    if (!m_videoInitialized || !m_swsContext) {
        LOG_ERROR("Video converter not initialized");
        return false;
    }
    
    if (srcFrame.width != m_srcWidth || srcFrame.height != m_srcHeight) {
        LOG_ERROR("Source frame dimensions mismatch");
        return false;
    }
    
    dstFrame.width = m_dstWidth;
    dstFrame.height = m_dstHeight;
    dstFrame.format = m_dstFormat;
    dstFrame.timestamp = srcFrame.timestamp;
    dstFrame.duration = srcFrame.duration;
    dstFrame.keyframe = srcFrame.keyframe;
    
    uint8_t* dstData[4] = {nullptr};
    int dstLinesize[4] = {0};
    
    int dstAVFormat = toAVPixelFormat(m_dstFormat);
    if (av_image_fill_arrays(dstData, dstLinesize, 
                              m_videoBuffer.data(), 
                              static_cast<AVPixelFormat>(dstAVFormat),
                              m_dstWidth, m_dstHeight, 32) < 0) {
        LOG_ERROR("Failed to fill image arrays");
        return false;
    }
    
    sws_scale(m_swsContext,
              srcFrame.data, srcFrame.linesize, 0, srcFrame.height,
              dstData, dstLinesize);
    
    for (int i = 0; i < 4; ++i) {
        dstFrame.data[i] = dstData[i];
        dstFrame.linesize[i] = dstLinesize[i];
    }
    
    return true;
}

bool FrameConverter::convertAudio(const AudioFrame& srcFrame, AudioFrame& dstFrame) {
    if (!m_audioInitialized || !m_swrContext) {
        LOG_ERROR("Audio converter not initialized");
        return false;
    }
    
    int outSamples = swr_get_out_samples(m_swrContext, srcFrame.samples);
    if (outSamples < 0) {
        LOG_ERROR("Failed to get output samples count");
        return false;
    }
    
    int dstAVFormat = toAVSampleFormat(m_dstAudioFormat);
    int bytesPerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(dstAVFormat));
    int bufferSize = outSamples * m_dstChannels * bytesPerSample;
    
    if (m_audioBuffer.size() < static_cast<size_t>(bufferSize)) {
        m_audioBuffer.resize(bufferSize);
    }
    
    uint8_t* outData = m_audioBuffer.data();
    const uint8_t* inData = srcFrame.data[0];
    
    int converted = swr_convert(m_swrContext,
                                  &outData, outSamples,
                                  &inData, srcFrame.samples);
    
    if (converted < 0) {
        LOG_ERROR("Failed to convert audio");
        return false;
    }
    
    dstFrame.sample_rate = m_dstSampleRate;
    dstFrame.channels = m_dstChannels;
    dstFrame.samples = converted;
    dstFrame.format = m_dstAudioFormat;
    dstFrame.timestamp = srcFrame.timestamp;
    
    for (int i = 0; i < m_dstChannels && i < 8; ++i) {
        dstFrame.data[i] = outData + i * converted * bytesPerSample;
    }
    
    return true;
}

int FrameConverter::toAVPixelFormat(PixelFormat format) {
    switch (format) {
        case PixelFormat::RGBA8888: return AV_PIX_FMT_RGBA;
        case PixelFormat::BGRA8888: return AV_PIX_FMT_BGRA;
        case PixelFormat::RGB888: return AV_PIX_FMT_RGB24;
        case PixelFormat::YUV420P: return AV_PIX_FMT_YUV420P;
        case PixelFormat::YUV422P: return AV_PIX_FMT_YUV422P;
        case PixelFormat::YUV444P: return AV_PIX_FMT_YUV444P;
        case PixelFormat::NV12: return AV_PIX_FMT_NV12;
        case PixelFormat::NV21: return AV_PIX_FMT_NV21;
        default: return AV_PIX_FMT_NONE;
    }
}

int FrameConverter::toAVSampleFormat(AudioFormat format) {
    switch (format) {
        case AudioFormat::S16: return AV_SAMPLE_FMT_S16;
        case AudioFormat::S32: return AV_SAMPLE_FMT_S32;
        case AudioFormat::F32: return AV_SAMPLE_FMT_FLT;
        case AudioFormat::U8: return AV_SAMPLE_FMT_U8;
        default: return AV_SAMPLE_FMT_NONE;
    }
}

}
