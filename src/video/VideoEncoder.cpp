#include "VideoEncoder.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

namespace ve {

VideoEncoder::VideoEncoder() {
    LOG_DEBUG("VideoEncoder created");
}

VideoEncoder::~VideoEncoder() {
    close();
    LOG_DEBUG("VideoEncoder destroyed");
}

bool VideoEncoder::open(const std::string& filename, 
                          const VideoParams& videoParams, 
                          const AudioParams& audioParams) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_isOpen) {
        LOG_WARNING("Encoder already open, closing first");
        close();
    }
    
    m_videoParams = videoParams;
    m_audioParams = audioParams;
    
    if (!openOutputFile(filename)) {
        LOG_ERROR("Failed to open output file");
        cleanup();
        return false;
    }
    
    m_isOpen = true;
    LOG_INFO("Successfully opened encoder for: %s, %dx%d @ %.2f FPS", 
             filename.c_str(), 
             videoParams.width,
             videoParams.height,
             videoParams.fps);
    
    return true;
}

void VideoEncoder::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isOpen) {
        flush();
    }
    cleanup();
}

void VideoEncoder::cleanup() {
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    
    if (m_packet) {
        av_packet_free(&m_packet);
    }
    
    if (m_videoFrame) {
        av_frame_free(&m_videoFrame);
    }
    
    if (m_audioFrame) {
        av_frame_free(&m_audioFrame);
    }
    
    if (m_videoCodecContext) {
        avcodec_free_context(&m_videoCodecContext);
    }
    
    if (m_audioCodecContext) {
        avcodec_free_context(&m_audioCodecContext);
    }
    
    if (m_formatContext) {
        if (!(m_formatContext->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&m_formatContext->pb);
        }
        avformat_free_context(m_formatContext);
        m_formatContext = nullptr;
    }
    
    m_videoFrameCount = 0;
    m_audioFrameCount = 0;
    m_isOpen = false;
}

bool VideoEncoder::openOutputFile(const std::string& filename) {
    avformat_alloc_output_context2(&m_formatContext, nullptr, nullptr, filename.c_str());
    if (!m_formatContext) {
        LOG_ERROR("Failed to allocate output context");
        return false;
    }
    
    if (m_videoParams.width > 0 && m_videoParams.height > 0) {
        if (!initVideoCodec()) {
            LOG_ERROR("Failed to initialize video codec");
            return false;
        }
    }
    
    if (m_audioParams.channels > 0) {
        if (!initAudioCodec()) {
            LOG_ERROR("Failed to initialize audio codec");
            return false;
        }
    }
    
    if (!(m_formatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_formatContext->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            LOG_ERROR("Failed to open output file");
            return false;
        }
    }
    
    if (avformat_write_header(m_formatContext, nullptr) < 0) {
        LOG_ERROR("Failed to write header");
        return false;
    }
    
    m_packet = av_packet_alloc();
    if (!m_packet) {
        LOG_ERROR("Failed to allocate packet");
        return false;
    }
    
    return true;
}

bool VideoEncoder::initVideoCodec() {
    const AVOutputFormat* outputFormat = m_formatContext->oformat;
    m_videoCodec = avcodec_find_encoder(outputFormat->video_codec);
    
    if (!m_videoCodec) {
        LOG_ERROR("Video codec not found");
        return false;
    }
    
    m_videoStream = avformat_new_stream(m_formatContext, nullptr);
    if (!m_videoStream) {
        LOG_ERROR("Failed to create video stream");
        return false;
    }
    
    m_videoCodecContext = avcodec_alloc_context3(m_videoCodec);
    if (!m_videoCodecContext) {
        LOG_ERROR("Failed to allocate video codec context");
        return false;
    }
    
    m_videoCodecContext->codec_id = m_videoCodec->id;
    m_videoCodecContext->width = m_videoParams.width;
    m_videoCodecContext->height = m_videoParams.height;
    m_videoCodecContext->time_base = AVRational{1, static_cast<int>(m_videoParams.fps)};
    m_videoCodecContext->framerate = av_d2q(m_videoParams.fps, 1001);
    m_videoCodecContext->pix_fmt = static_cast<AVPixelFormat>(convertPixelFormat(m_videoParams.format));
    m_videoCodecContext->bit_rate = m_videoParams.bitrate;
    m_videoCodecContext->gop_size = 30;
    m_videoCodecContext->max_b_frames = 2;
    
    if (m_videoCodec->id == AV_CODEC_ID_H264) {
        av_opt_set(m_videoCodecContext->priv_data, "preset", "medium", 0);
        av_opt_set(m_videoCodecContext->priv_data, "crf", "23", 0);
    }
    
    if (m_formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        m_videoCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    if (avcodec_open2(m_videoCodecContext, m_videoCodec, nullptr) < 0) {
        LOG_ERROR("Failed to open video codec");
        return false;
    }
    
    if (avcodec_parameters_from_context(m_videoStream->codecpar, m_videoCodecContext) < 0) {
        LOG_ERROR("Failed to copy video codec parameters");
        return false;
    }
    
    m_videoStream->time_base = m_videoCodecContext->time_base;
    
    m_videoFrame = av_frame_alloc();
    if (!m_videoFrame) {
        LOG_ERROR("Failed to allocate video frame");
        return false;
    }
    
    m_videoFrame->format = m_videoCodecContext->pix_fmt;
    m_videoFrame->width = m_videoCodecContext->width;
    m_videoFrame->height = m_videoCodecContext->height;
    
    if (av_frame_get_buffer(m_videoFrame, 32) < 0) {
        LOG_ERROR("Failed to allocate video frame buffer");
        return false;
    }
    
    return true;
}

bool VideoEncoder::initAudioCodec() {
    const AVOutputFormat* outputFormat = m_formatContext->oformat;
    m_audioCodec = avcodec_find_encoder(outputFormat->audio_codec);
    
    if (!m_audioCodec) {
        LOG_ERROR("Audio codec not found");
        return false;
    }
    
    m_audioStream = avformat_new_stream(m_formatContext, nullptr);
    if (!m_audioStream) {
        LOG_ERROR("Failed to create audio stream");
        return false;
    }
    
    m_audioCodecContext = avcodec_alloc_context3(m_audioCodec);
    if (!m_audioCodecContext) {
        LOG_ERROR("Failed to allocate audio codec context");
        return false;
    }
    
    m_audioCodecContext->codec_id = m_audioCodec->id;
    m_audioCodecContext->sample_rate = m_audioParams.sample_rate;
    m_audioCodecContext->ch_layout.nb_channels = m_audioParams.channels;
    m_audioCodecContext->sample_fmt = m_audioCodec->sample_fmts ? m_audioCodec->sample_fmts[0] : AV_SAMPLE_FMT_S16;
    m_audioCodecContext->bit_rate = m_audioParams.bitrate;
    m_audioCodecContext->time_base = AVRational{1, m_audioParams.sample_rate};
    
    if (m_formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        m_audioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    if (avcodec_open2(m_audioCodecContext, m_audioCodec, nullptr) < 0) {
        LOG_ERROR("Failed to open audio codec");
        return false;
    }
    
    if (avcodec_parameters_from_context(m_audioStream->codecpar, m_audioCodecContext) < 0) {
        LOG_ERROR("Failed to copy audio codec parameters");
        return false;
    }
    
    m_audioStream->time_base = m_audioCodecContext->time_base;
    
    m_audioFrame = av_frame_alloc();
    if (!m_audioFrame) {
        LOG_ERROR("Failed to allocate audio frame");
        return false;
    }
    
    return true;
}

bool VideoEncoder::writeVideoFrame(const VideoFrame& frame) {
    if (!m_isOpen || !m_videoCodecContext) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (av_frame_make_writable(m_videoFrame) < 0) {
        LOG_ERROR("Failed to make frame writable");
        return false;
    }
    
    AVPixelFormat srcFormat = static_cast<AVPixelFormat>(convertPixelFormat(frame.format));
    
    if (srcFormat != m_videoCodecContext->pix_fmt || 
        frame.width != m_videoCodecContext->width ||
        frame.height != m_videoCodecContext->height) {
        
        if (!m_swsContext ||
            frame.width != m_videoCodecContext->width ||
            frame.height != m_videoCodecContext->height) {
            
            if (m_swsContext) {
                sws_freeContext(m_swsContext);
            }
            
            m_swsContext = sws_getContext(
                frame.width, frame.height, srcFormat,
                m_videoCodecContext->width, m_videoCodecContext->height, m_videoCodecContext->pix_fmt,
                SWS_BICUBIC, nullptr, nullptr, nullptr
            );
            
            if (!m_swsContext) {
                LOG_ERROR("Failed to create sws context");
                return false;
            }
        }
        
        sws_scale(m_swsContext,
                  frame.data, frame.linesize, 0, frame.height,
                  m_videoFrame->data, m_videoFrame->linesize);
    } else {
        for (int i = 0; i < 4; ++i) {
            if (frame.data[i]) {
                memcpy(m_videoFrame->data[i], frame.data[i], 
                       frame.linesize[i] * frame.height);
            }
        }
    }
    
    m_videoFrame->pts = m_videoFrameCount++;
    
    int ret = avcodec_send_frame(m_videoCodecContext, m_videoFrame);
    if (ret < 0) {
        LOG_ERROR("Failed to send video frame");
        return false;
    }
    
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_videoCodecContext, m_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            LOG_ERROR("Error encoding video frame");
            return false;
        }
        
        av_packet_rescale_ts(m_packet, m_videoCodecContext->time_base, m_videoStream->time_base);
        m_packet->stream_index = m_videoStream->index;
        
        av_interleaved_write_frame(m_formatContext, m_packet);
        av_packet_unref(m_packet);
    }
    
    return true;
}

bool VideoEncoder::writeAudioFrame(const AudioFrame& frame) {
    if (!m_isOpen || !m_audioCodecContext) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_audioFrame->nb_samples = frame.samples;
    m_audioFrame->ch_layout.nb_channels = frame.channels;
    m_audioFrame->sample_rate = frame.sample_rate;
    m_audioFrame->format = static_cast<AVSampleFormat>(convertAudioFormat(frame.format));
    
    if (av_frame_get_buffer(m_audioFrame, 0) < 0) {
        LOG_ERROR("Failed to allocate audio frame buffer");
        return false;
    }
    
    for (int i = 0; i < frame.channels; ++i) {
        if (frame.data[i]) {
            memcpy(m_audioFrame->data[i], frame.data[i], 
                   av_samples_get_buffer_size(nullptr, frame.channels, frame.samples, 
                                              static_cast<AVSampleFormat>(m_audioFrame->format), 0));
        }
    }
    
    m_audioFrame->pts = m_audioFrameCount++;
    
    int ret = avcodec_send_frame(m_audioCodecContext, m_audioFrame);
    if (ret < 0) {
        LOG_ERROR("Failed to send audio frame");
        return false;
    }
    
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_audioCodecContext, m_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            LOG_ERROR("Error encoding audio frame");
            return false;
        }
        
        av_packet_rescale_ts(m_packet, m_audioCodecContext->time_base, m_audioStream->time_base);
        m_packet->stream_index = m_audioStream->index;
        
        av_interleaved_write_frame(m_formatContext, m_packet);
        av_packet_unref(m_packet);
    }
    
    return true;
}

bool VideoEncoder::flush() {
    if (!m_isOpen) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    int ret;
    
    if (m_videoCodecContext) {
        ret = avcodec_send_frame(m_videoCodecContext, nullptr);
        while (ret >= 0) {
            ret = avcodec_receive_packet(m_videoCodecContext, m_packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                LOG_ERROR("Error flushing video encoder");
                return false;
            }
            
            av_packet_rescale_ts(m_packet, m_videoCodecContext->time_base, m_videoStream->time_base);
            m_packet->stream_index = m_videoStream->index;
            av_interleaved_write_frame(m_formatContext, m_packet);
            av_packet_unref(m_packet);
        }
    }
    
    if (m_audioCodecContext) {
        ret = avcodec_send_frame(m_audioCodecContext, nullptr);
        while (ret >= 0) {
            ret = avcodec_receive_packet(m_audioCodecContext, m_packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                LOG_ERROR("Error flushing audio encoder");
                return false;
            }
            
            av_packet_rescale_ts(m_packet, m_audioCodecContext->time_base, m_audioStream->time_base);
            m_packet->stream_index = m_audioStream->index;
            av_interleaved_write_frame(m_formatContext, m_packet);
            av_packet_unref(m_packet);
        }
    }
    
    av_write_trailer(m_formatContext);
    
    return true;
}

int VideoEncoder::convertPixelFormat(PixelFormat format) {
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

int VideoEncoder::convertAudioFormat(AudioFormat format) {
    switch (format) {
        case AudioFormat::S16: return AV_SAMPLE_FMT_S16;
        case AudioFormat::S32: return AV_SAMPLE_FMT_S32;
        case AudioFormat::F32: return AV_SAMPLE_FMT_FLT;
        case AudioFormat::U8: return AV_SAMPLE_FMT_U8;
        default: return AV_SAMPLE_FMT_NONE;
    }
}

int64_t VideoEncoder::convertTimestamp(TimePoint timestamp, AVRational timebase) {
    return av_rescale(timestamp.count(), timebase.den, timebase.num * 1000000);
}

}
