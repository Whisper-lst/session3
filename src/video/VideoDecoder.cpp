#include "VideoDecoder.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

namespace ve {

VideoDecoder::VideoDecoder() {
    LOG_DEBUG("VideoDecoder created");
}

VideoDecoder::~VideoDecoder() {
    close();
    LOG_DEBUG("VideoDecoder destroyed");
}

bool VideoDecoder::open(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_isOpen) {
        LOG_WARNING("Decoder already open, closing first");
        close();
    }
    
    m_mediaInfo.filename = filename;
    m_formatContext = avformat_alloc_context();
    
    if (!m_formatContext) {
        LOG_ERROR("Failed to allocate format context");
        return false;
    }
    
    if (avformat_open_input(&m_formatContext, filename.c_str(), nullptr, nullptr) != 0) {
        LOG_ERROR("Failed to open input file: %s", filename.c_str());
        cleanup();
        return false;
    }
    
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        LOG_ERROR("Failed to find stream info");
        cleanup();
        return false;
    }
    
    for (unsigned int i = 0; i < m_formatContext->nb_streams; ++i) {
        if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_videoStreamIndex = i;
            m_mediaInfo.has_video = true;
        } else if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audioStreamIndex = i;
            m_mediaInfo.has_audio = true;
        }
    }
    
    if (m_videoStreamIndex < 0) {
        LOG_ERROR("No video stream found");
        cleanup();
        return false;
    }
    
    if (!initCodec()) {
        LOG_ERROR("Failed to initialize codec");
        cleanup();
        return false;
    }
    
    AVStream* videoStream = m_formatContext->streams[m_videoStreamIndex];
    AVCodecParameters* codecParams = videoStream->codecpar;
    
    m_mediaInfo.video_params.width = codecParams->width;
    m_mediaInfo.video_params.height = codecParams->height;
    m_mediaInfo.video_params.format = convertPixelFormat(codecParams->format);
    m_mediaInfo.video_params.fps = av_q2d(videoStream->avg_frame_rate);
    m_mediaInfo.video_params.bitrate = codecParams->bit_rate;
    
    if (m_formatContext->duration != AV_NOPTS_VALUE) {
        m_mediaInfo.duration = Duration(m_formatContext->duration / (AV_TIME_BASE / 1000000));
    }
    
    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    
    if (!m_frame || !m_packet) {
        LOG_ERROR("Failed to allocate frame/packet");
        cleanup();
        return false;
    }
    
    m_isOpen = true;
    LOG_INFO("Successfully opened video: %s, %dx%d @ %.2f FPS", 
             filename.c_str(), 
             m_mediaInfo.video_params.width,
             m_mediaInfo.video_params.height,
             m_mediaInfo.video_params.fps);
    
    return true;
}

void VideoDecoder::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    cleanup();
}

void VideoDecoder::cleanup() {
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    
    if (m_frame) {
        av_frame_free(&m_frame);
    }
    
    if (m_packet) {
        av_packet_free(&m_packet);
    }
    
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
    }
    
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }
    
    m_videoStreamIndex = -1;
    m_audioStreamIndex = -1;
    m_isOpen = false;
}

bool VideoDecoder::initCodec() {
    if (m_videoStreamIndex < 0) return false;
    
    AVStream* videoStream = m_formatContext->streams[m_videoStreamIndex];
    const AVCodec* codec = avcodec_find_decoder(videoStream->codecpar->codec_id);
    
    if (!codec) {
        LOG_ERROR("Failed to find codec");
        return false;
    }
    
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        LOG_ERROR("Failed to allocate codec context");
        return false;
    }
    
    if (avcodec_parameters_to_context(m_codecContext, videoStream->codecpar) < 0) {
        LOG_ERROR("Failed to copy codec parameters");
        return false;
    }
    
    if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
        LOG_ERROR("Failed to open codec");
        return false;
    }
    
    m_codec = codec;
    return true;
}

bool VideoDecoder::seek(TimePoint timestamp) {
    if (!m_isOpen) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    AVStream* stream = m_formatContext->streams[m_videoStreamIndex];
    int64_t seekTarget = timestamp.count();
    seekTarget = av_rescale(seekTarget, stream->time_base.den, stream->time_base.num * 1000000);
    
    if (av_seek_frame(m_formatContext, m_videoStreamIndex, seekTarget, AVSEEK_FLAG_BACKWARD) < 0) {
        LOG_ERROR("Seek failed");
        return false;
    }
    
    avcodec_flush_buffers(m_codecContext);
    return true;
}

bool VideoDecoder::readPacket() {
    return av_read_frame(m_formatContext, m_packet) >= 0;
}

bool VideoDecoder::sendPacket() {
    int ret = avcodec_send_packet(m_codecContext, m_packet);
    if (ret < 0) {
        return false;
    }
    return true;
}

bool VideoDecoder::receiveFrame(AVFrame* frame) {
    int ret = avcodec_receive_frame(m_codecContext, frame);
    return ret == 0;
}

int VideoDecoder::getFrameBufferSize() const {
    if (!m_isOpen) return 0;
    
    int width = m_mediaInfo.video_params.width;
    int height = m_mediaInfo.video_params.height;
    PixelFormat format = m_mediaInfo.video_params.format;
    
    switch (format) {
        case PixelFormat::YUV420P:
            return width * height * 3 / 2;
        case PixelFormat::YUV422P:
            return width * height * 2;
        case PixelFormat::YUV444P:
        case PixelFormat::RGB888:
            return width * height * 3;
        case PixelFormat::RGBA8888:
        case PixelFormat::BGRA8888:
            return width * height * 4;
        case PixelFormat::NV12:
        case PixelFormat::NV21:
            return width * height * 3 / 2;
        default:
            return width * height * 4;
    }
}

int VideoDecoder::calculateFrameSize(const VideoFrame& frame) const {
    int size = 0;
    int planeCount = 0;
    
    switch (frame.format) {
        case PixelFormat::YUV420P:
            planeCount = 3;
            break;
        case PixelFormat::YUV422P:
            planeCount = 3;
            break;
        case PixelFormat::YUV444P:
        case PixelFormat::RGB888:
            planeCount = 1;
            break;
        case PixelFormat::RGBA8888:
        case PixelFormat::BGRA8888:
            planeCount = 1;
            break;
        case PixelFormat::NV12:
        case PixelFormat::NV21:
            planeCount = 3;
            break;
        default:
            planeCount = 1;
            break;
    }
    
    for (int i = 0; i < planeCount && i < 4; ++i) {
        if (frame.data[i] == nullptr) continue;
        
        int planeSize = frame.linesize[i] * frame.height;
        if (frame.format == PixelFormat::YUV420P) {
            if (i == 1 || i == 2) {
                planeSize = frame.linesize[i] * (frame.height + 1) / 2;
            }
        } else if (frame.format == PixelFormat::YUV422P) {
            if (i == 1 || i == 2) {
                planeSize = frame.linesize[i] * frame.height / 2;
            }
        }
        size += planeSize;
    }
    
    return size;
}

void VideoDecoder::copyFrameData(VideoFrame& dest, const VideoFrame& src, std::vector<uint8_t>& buffer) {
    int requiredSize = calculateFrameSize(src);
    if (buffer.size() < static_cast<size_t>(requiredSize)) {
        buffer.resize(requiredSize);
    }
    
    dest = src;
    
    memset(dest.data, 0, sizeof(dest.data));
    memset(dest.linesize, 0, sizeof(dest.linesize));
    
    int planeCount = 0;
    switch (src.format) {
        case PixelFormat::YUV420P:
        case PixelFormat::YUV422P:
        case PixelFormat::YUV444P:
        case PixelFormat::NV12:
        case PixelFormat::NV21:
            planeCount = 3;
            break;
        case PixelFormat::RGB888:
        case PixelFormat::RGBA8888:
        case PixelFormat::BGRA8888:
        default:
            planeCount = 1;
            break;
    }
    
    int offset = 0;
    for (int i = 0; i < planeCount && i < 4; ++i) {
        if (src.data[i] == nullptr) continue;
        
        int planeSize = src.linesize[i] * src.height;
        if (src.format == PixelFormat::YUV420P) {
            if (i == 1 || i == 2) {
                planeSize = src.linesize[i] * (src.height + 1) / 2;
            }
        } else if (src.format == PixelFormat::YUV422P) {
            if (i == 1 || i == 2) {
                planeSize = src.linesize[i] * src.height / 2;
            }
        }
        
        if (offset + planeSize > static_cast<int>(buffer.size())) {
            planeSize = static_cast<int>(buffer.size()) - offset;
            if (planeSize <= 0) break;
        }
        
        memcpy(buffer.data() + offset, src.data[i], planeSize);
        dest.data[i] = buffer.data() + offset;
        dest.linesize[i] = src.linesize[i];
        offset += planeSize;
    }
}

bool VideoDecoder::decodeNextFrame(VideoFrame& outFrame) {
    if (!m_isOpen) return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    while (true) {
        if (receiveFrame(m_frame)) {
            if (m_frame->width == 0 || m_frame->height == 0) {
                continue;
            }
            
            AVStream* stream = m_formatContext->streams[m_videoStreamIndex];
            
            outFrame.width = m_frame->width;
            outFrame.height = m_frame->height;
            outFrame.format = convertPixelFormat(m_frame->format);
            outFrame.keyframe = (m_frame->key_frame != 0);
            
            if (m_frame->pts != AV_NOPTS_VALUE) {
                outFrame.timestamp = convertTimestamp(m_frame->pts, stream->time_base);
            }
            
            if (m_frame->duration > 0) {
                outFrame.duration = convertDuration(m_frame->duration, stream->time_base);
            } else if (stream->avg_frame_rate.den > 0) {
                outFrame.duration = Duration(static_cast<int64_t>(1000000.0 / av_q2d(stream->avg_frame_rate)));
            }
            
            int requiredSize = m_frame->width * m_frame->height * 4;
            if (m_frameBuffer.size() < static_cast<size_t>(requiredSize)) {
                m_frameBuffer.resize(requiredSize);
            }
            
            memset(outFrame.data, 0, sizeof(outFrame.data));
            memset(outFrame.linesize, 0, sizeof(outFrame.linesize));
            
            int planeCount = 0;
            switch (outFrame.format) {
                case PixelFormat::YUV420P:
                case PixelFormat::YUV422P:
                case PixelFormat::YUV444P:
                case PixelFormat::NV12:
                case PixelFormat::NV21:
                    planeCount = 3;
                    break;
                case PixelFormat::RGB888:
                case PixelFormat::RGBA8888:
                case PixelFormat::BGRA8888:
                default:
                    planeCount = 1;
                    break;
            }
            
            int offset = 0;
            for (int i = 0; i < planeCount && i < 4; ++i) {
                if (m_frame->data[i] == nullptr) {
                    continue;
                }
                
                int planeSize = m_frame->linesize[i] * m_frame->height;
                if (outFrame.format == PixelFormat::YUV420P) {
                    if (i == 1 || i == 2) {
                        planeSize = m_frame->linesize[i] * (m_frame->height + 1) / 2;
                    }
                } else if (outFrame.format == PixelFormat::YUV422P) {
                    if (i == 1 || i == 2) {
                        planeSize = m_frame->linesize[i] * m_frame->height / 2;
                    }
                }
                
                if (offset + planeSize > static_cast<int>(m_frameBuffer.size())) {
                    planeSize = static_cast<int>(m_frameBuffer.size()) - offset;
                    if (planeSize <= 0) break;
                }
                
                memcpy(m_frameBuffer.data() + offset, m_frame->data[i], planeSize);
                outFrame.data[i] = m_frameBuffer.data() + offset;
                outFrame.linesize[i] = m_frame->linesize[i];
                offset += planeSize;
            }
            
            return true;
        }
        
        if (!readPacket()) {
            return false;
        }
        
        if (m_packet->stream_index != m_videoStreamIndex) {
            av_packet_unref(m_packet);
            continue;
        }
        
        if (!sendPacket()) {
            av_packet_unref(m_packet);
            continue;
        }
        
        av_packet_unref(m_packet);
    }
    
    return false;
}

bool VideoDecoder::decodeFrameAt(TimePoint timestamp, VideoFrame& outFrame) {
    if (!seek(timestamp)) {
        return false;
    }
    
    VideoFrame frame;
    VideoFrame bestFrame;
    std::vector<uint8_t> bestFrameBuffer;
    TimePoint bestTimestamp = TimePoint::max();
    bool found = false;
    
    while (decodeNextFrame(frame)) {
        if (frame.timestamp >= timestamp) {
            if (bestTimestamp == TimePoint::max() || 
                frame.timestamp - timestamp < bestTimestamp - timestamp) {
                copyFrameData(bestFrame, frame, bestFrameBuffer);
                bestTimestamp = frame.timestamp;
                found = true;
            }
            break;
        }
        
        if (bestTimestamp == TimePoint::max() || 
            timestamp - frame.timestamp < timestamp - bestTimestamp) {
            copyFrameData(bestFrame, frame, bestFrameBuffer);
            bestTimestamp = frame.timestamp;
            found = true;
        }
    }
    
    if (found) {
        outFrame = bestFrame;
    }
    
    return found;
}

PixelFormat VideoDecoder::convertPixelFormat(int avFormat) {
    switch (avFormat) {
        case AV_PIX_FMT_RGBA: return PixelFormat::RGBA8888;
        case AV_PIX_FMT_BGRA: return PixelFormat::BGRA8888;
        case AV_PIX_FMT_RGB24: return PixelFormat::RGB888;
        case AV_PIX_FMT_YUV420P: return PixelFormat::YUV420P;
        case AV_PIX_FMT_YUV422P: return PixelFormat::YUV422P;
        case AV_PIX_FMT_YUV444P: return PixelFormat::YUV444P;
        case AV_PIX_FMT_NV12: return PixelFormat::NV12;
        case AV_PIX_FMT_NV21: return PixelFormat::NV21;
        default: return PixelFormat::Unknown;
    }
}

TimePoint VideoDecoder::convertTimestamp(int64_t pts, AVRational timebase) {
    if (pts == AV_NOPTS_VALUE) {
        return TimePoint(0);
    }
    return TimePoint(av_rescale(pts, timebase.num * 1000000, timebase.den));
}

Duration VideoDecoder::convertDuration(int64_t duration, AVRational timebase) {
    return Duration(av_rescale(duration, timebase.num * 1000000, timebase.den));
}

}
