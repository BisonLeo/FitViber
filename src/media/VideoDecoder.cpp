#include "VideoDecoder.h"

#ifdef HAS_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

struct VideoDecoder::FFmpegContext {
    AVFormatContext* fmtCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    SwsContext* swsCtx = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* rgbFrame = nullptr;
    AVPacket* packet = nullptr;
    int videoStreamIdx = -1;
    double timeBase = 0.0;
    uint8_t* rgbBuffer = nullptr;
    bool eofReached = false;   // av_read_frame returned EOF
    bool flushed = false;      // flush packet sent to codec

    ~FFmpegContext() {
        if (rgbBuffer) av_free(rgbBuffer);
        if (packet) av_packet_free(&packet);
        if (rgbFrame) av_frame_free(&rgbFrame);
        if (frame) av_frame_free(&frame);
        if (swsCtx) sws_freeContext(swsCtx);
        if (codecCtx) avcodec_free_context(&codecCtx);
        if (fmtCtx) avformat_close_input(&fmtCtx);
    }
};
#endif

VideoDecoder::VideoDecoder(QObject* parent) : QObject(parent) {}

VideoDecoder::~VideoDecoder() {
    close();
}

bool VideoDecoder::open(const QString& filePath) {
#ifdef HAS_FFMPEG
    close();

    m_ctx = std::make_unique<FFmpegContext>();

    int ret = avformat_open_input(&m_ctx->fmtCtx, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        m_ctx.reset();
        return false;
    }

    ret = avformat_find_stream_info(m_ctx->fmtCtx, nullptr);
    if (ret < 0) {
        m_ctx.reset();
        return false;
    }

    // Find best video stream
    m_ctx->videoStreamIdx = av_find_best_stream(m_ctx->fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_ctx->videoStreamIdx < 0) {
        m_ctx.reset();
        return false;
    }

    AVStream* stream = m_ctx->fmtCtx->streams[m_ctx->videoStreamIdx];
    AVCodecParameters* par = stream->codecpar;

    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(par->codec_id);
    if (!codec) {
        m_ctx.reset();
        return false;
    }

    // Allocate codec context
    m_ctx->codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_ctx->codecCtx, par);

    ret = avcodec_open2(m_ctx->codecCtx, codec, nullptr);
    if (ret < 0) {
        m_ctx.reset();
        return false;
    }

    // Time base for PTS conversion
    m_ctx->timeBase = av_q2d(stream->time_base);

    // Fill video info
    m_info.width = m_ctx->codecCtx->width;
    m_info.height = m_ctx->codecCtx->height;
    m_info.codecName = QString(codec->name);

    if (stream->avg_frame_rate.den > 0 && stream->avg_frame_rate.num > 0)
        m_info.fps = av_q2d(stream->avg_frame_rate);
    else if (stream->r_frame_rate.den > 0 && stream->r_frame_rate.num > 0)
        m_info.fps = av_q2d(stream->r_frame_rate);

    // Prefer video stream duration over container duration
    if (stream->duration != AV_NOPTS_VALUE && stream->duration > 0) {
        m_info.duration = static_cast<double>(stream->duration) * m_ctx->timeBase;
    } else if (m_ctx->fmtCtx->duration > 0) {
        m_info.duration = static_cast<double>(m_ctx->fmtCtx->duration) / AV_TIME_BASE;
    }

    if (m_info.fps > 0 && m_info.duration > 0)
        m_info.totalFrames = static_cast<int64_t>(m_info.duration * m_info.fps);

    // Allocate frames and packet
    m_ctx->frame = av_frame_alloc();
    m_ctx->rgbFrame = av_frame_alloc();
    m_ctx->packet = av_packet_alloc();

    // Allocate RGB buffer
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, m_info.width, m_info.height, 1);
    m_ctx->rgbBuffer = static_cast<uint8_t*>(av_malloc(numBytes));
    av_image_fill_arrays(m_ctx->rgbFrame->data, m_ctx->rgbFrame->linesize,
                         m_ctx->rgbBuffer, AV_PIX_FMT_RGB32,
                         m_info.width, m_info.height, 1);

    // Create scaler context
    m_ctx->swsCtx = sws_getContext(
        m_info.width, m_info.height,
        m_ctx->codecCtx->pix_fmt,
        m_info.width, m_info.height,
        AV_PIX_FMT_RGB32,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!m_ctx->swsCtx) {
        m_ctx.reset();
        return false;
    }

    m_isOpen = true;
    m_currentTime = 0.0;
    return true;
#else
    Q_UNUSED(filePath);
    return false;
#endif
}

void VideoDecoder::close() {
#ifdef HAS_FFMPEG
    m_ctx.reset();
#endif
    m_isOpen = false;
    m_currentTime = 0.0;
    m_info = VideoInfo{};
}

QImage VideoDecoder::decodeNextFrame() {
#ifdef HAS_FFMPEG
    if (!m_isOpen || !m_ctx) return QImage();

    while (true) {
        // Try to receive a frame from the codec first (handles buffered B-frames)
        int ret = avcodec_receive_frame(m_ctx->codecCtx, m_ctx->frame);
        if (ret == 0) {
            // Got a frame — convert and return
            sws_scale(m_ctx->swsCtx,
                      m_ctx->frame->data, m_ctx->frame->linesize,
                      0, m_info.height,
                      m_ctx->rgbFrame->data, m_ctx->rgbFrame->linesize);

            double pts = 0.0;
            if (m_ctx->frame->pts != AV_NOPTS_VALUE) {
                pts = static_cast<double>(m_ctx->frame->pts) * m_ctx->timeBase;
            }
            m_currentTime = pts;

            QImage img(m_ctx->rgbFrame->data[0],
                       m_info.width, m_info.height,
                       m_ctx->rgbFrame->linesize[0],
                       QImage::Format_RGB32);
            QImage result = img.copy();

            emit frameDecoded(result, pts);
            return result;
        }

        if (ret != AVERROR(EAGAIN)) {
            // AVERROR_EOF or real error — no more frames
            return QImage();
        }

        // Codec needs more input
        if (m_ctx->eofReached) {
            if (!m_ctx->flushed) {
                // Send flush packet to drain remaining buffered frames
                m_ctx->flushed = true;
                avcodec_send_packet(m_ctx->codecCtx, nullptr);
                continue;  // Loop back to receive_frame
            }
            // Already flushed and no more frames
            return QImage();
        }

        // Read next packet from file
        ret = av_read_frame(m_ctx->fmtCtx, m_ctx->packet);
        if (ret < 0) {
            // EOF or read error — start draining codec
            m_ctx->eofReached = true;
            m_ctx->flushed = true;
            avcodec_send_packet(m_ctx->codecCtx, nullptr);
            continue;  // Loop back to receive_frame to get buffered frames
        }

        if (m_ctx->packet->stream_index != m_ctx->videoStreamIdx) {
            av_packet_unref(m_ctx->packet);
            continue;
        }

        avcodec_send_packet(m_ctx->codecCtx, m_ctx->packet);
        av_packet_unref(m_ctx->packet);
        // Loop back to receive_frame
    }
#endif
    return QImage();
}

bool VideoDecoder::seek(double seconds) {
#ifdef HAS_FFMPEG
    if (!m_isOpen || !m_ctx) return false;

    int64_t timestamp = static_cast<int64_t>(seconds * AV_TIME_BASE);
    int ret = av_seek_frame(m_ctx->fmtCtx, -1, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) return false;

    avcodec_flush_buffers(m_ctx->codecCtx);
    m_ctx->eofReached = false;
    m_ctx->flushed = false;
    m_currentTime = seconds;
    return true;
#else
    Q_UNUSED(seconds);
    return false;
#endif
}
