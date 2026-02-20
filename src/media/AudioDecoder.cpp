#include "AudioDecoder.h"

#ifdef HAS_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

struct AudioDecoder::FFmpegAudioContext {
    AVFormatContext* fmtCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    SwrContext* swrCtx = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;
    int audioStreamIdx = -1;
    double timeBase = 0.0;

    ~FFmpegAudioContext() {
        if (packet) av_packet_free(&packet);
        if (frame) av_frame_free(&frame);
        if (swrCtx) swr_free(&swrCtx);
        if (codecCtx) avcodec_free_context(&codecCtx);
        if (fmtCtx) avformat_close_input(&fmtCtx);
    }
};
#endif

AudioDecoder::AudioDecoder(QObject* parent) : QObject(parent) {}
AudioDecoder::~AudioDecoder() { close(); }

bool AudioDecoder::open(const QString& filePath) {
#ifdef HAS_FFMPEG
    close();

    m_ctx = std::make_unique<FFmpegAudioContext>();

    int ret = avformat_open_input(&m_ctx->fmtCtx, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) { m_ctx.reset(); return false; }

    ret = avformat_find_stream_info(m_ctx->fmtCtx, nullptr);
    if (ret < 0) { m_ctx.reset(); return false; }

    m_ctx->audioStreamIdx = av_find_best_stream(m_ctx->fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (m_ctx->audioStreamIdx < 0) { m_ctx.reset(); return false; }

    AVStream* stream = m_ctx->fmtCtx->streams[m_ctx->audioStreamIdx];
    AVCodecParameters* par = stream->codecpar;

    const AVCodec* codec = avcodec_find_decoder(par->codec_id);
    if (!codec) { m_ctx.reset(); return false; }

    m_ctx->codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_ctx->codecCtx, par);

    ret = avcodec_open2(m_ctx->codecCtx, codec, nullptr);
    if (ret < 0) { m_ctx.reset(); return false; }

    m_ctx->timeBase = av_q2d(stream->time_base);

    // Fill audio info
    m_info.sampleRate = m_ctx->codecCtx->sample_rate;
    m_info.channels = m_ctx->codecCtx->ch_layout.nb_channels;
    m_info.bitsPerSample = 16; // We always output S16
    m_info.codecName = QString(codec->name);
    m_info.duration = (m_ctx->fmtCtx->duration > 0)
        ? static_cast<double>(m_ctx->fmtCtx->duration) / AV_TIME_BASE
        : 0.0;

    // Setup resampler to output S16 interleaved
    ret = swr_alloc_set_opts2(&m_ctx->swrCtx,
        &m_ctx->codecCtx->ch_layout, AV_SAMPLE_FMT_S16, m_info.sampleRate,
        &m_ctx->codecCtx->ch_layout, m_ctx->codecCtx->sample_fmt, m_info.sampleRate,
        0, nullptr);
    if (ret < 0 || !m_ctx->swrCtx) { m_ctx.reset(); return false; }

    ret = swr_init(m_ctx->swrCtx);
    if (ret < 0) { m_ctx.reset(); return false; }

    m_ctx->frame = av_frame_alloc();
    m_ctx->packet = av_packet_alloc();

    m_isOpen = true;
    return true;
#else
    Q_UNUSED(filePath);
    return false;
#endif
}

void AudioDecoder::close() {
#ifdef HAS_FFMPEG
    m_ctx.reset();
#endif
    m_isOpen = false;
    m_info = AudioInfo{};
}

QByteArray AudioDecoder::decode(double maxSeconds) {
    QByteArray result;
#ifdef HAS_FFMPEG
    if (!m_isOpen || !m_ctx) return result;

    int64_t maxSamples = -1;
    if (maxSeconds > 0)
        maxSamples = static_cast<int64_t>(maxSeconds * m_info.sampleRate);

    int64_t totalSamples = 0;

    while (av_read_frame(m_ctx->fmtCtx, m_ctx->packet) >= 0) {
        if (m_ctx->packet->stream_index != m_ctx->audioStreamIdx) {
            av_packet_unref(m_ctx->packet);
            continue;
        }

        int ret = avcodec_send_packet(m_ctx->codecCtx, m_ctx->packet);
        av_packet_unref(m_ctx->packet);
        if (ret < 0) continue;

        while (avcodec_receive_frame(m_ctx->codecCtx, m_ctx->frame) >= 0) {
            int outSamples = m_ctx->frame->nb_samples;
            int bytesPerSample = 2 * m_info.channels; // S16 interleaved

            QByteArray outBuf(outSamples * bytesPerSample, 0);
            uint8_t* outPtr = reinterpret_cast<uint8_t*>(outBuf.data());

            int converted = swr_convert(m_ctx->swrCtx,
                &outPtr, outSamples,
                const_cast<const uint8_t**>(m_ctx->frame->data), m_ctx->frame->nb_samples);

            if (converted > 0) {
                result.append(outBuf.data(), converted * bytesPerSample);
                totalSamples += converted;
            }

            if (maxSamples > 0 && totalSamples >= maxSamples) {
                return result;
            }
        }
    }
#else
    Q_UNUSED(maxSeconds);
#endif
    return result;
}

bool AudioDecoder::seek(double seconds) {
#ifdef HAS_FFMPEG
    if (!m_isOpen || !m_ctx) return false;

    int64_t timestamp = static_cast<int64_t>(seconds * AV_TIME_BASE);
    int ret = av_seek_frame(m_ctx->fmtCtx, -1, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) return false;

    avcodec_flush_buffers(m_ctx->codecCtx);
    return true;
#else
    Q_UNUSED(seconds);
    return false;
#endif
}
