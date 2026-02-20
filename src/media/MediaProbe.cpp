#include "MediaProbe.h"
#include <QDateTime>
#include <QTimeZone>

#ifdef HAS_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/pixdesc.h>
}
#endif

MediaProbe::MediaProbe(QObject* parent) : QObject(parent) {}
MediaProbe::~MediaProbe() = default;

bool MediaProbe::probe(const QString& filePath) {
    m_info = MediaInfo{};
    m_info.filePath = filePath;

#ifdef HAS_FFMPEG
    AVFormatContext* fmtCtx = nullptr;
    int ret = avformat_open_input(&fmtCtx, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        char errBuf[256];
        av_strerror(ret, errBuf, sizeof(errBuf));
        m_error = QString("Cannot open file: %1 (%2)").arg(filePath, errBuf);
        return false;
    }

    ret = avformat_find_stream_info(fmtCtx, nullptr);
    if (ret < 0) {
        m_error = "Cannot find stream info";
        avformat_close_input(&fmtCtx);
        return false;
    }

    // Container info
    m_info.containerFormat = QString(fmtCtx->iformat->long_name);
    m_info.duration = (fmtCtx->duration > 0)
        ? static_cast<double>(fmtCtx->duration) / AV_TIME_BASE
        : 0.0;

    // Extract all container-level metadata
    AVDictionaryEntry* tag = nullptr;
    while ((tag = av_dict_get(fmtCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        m_info.metadata.insert(QString(tag->key), QString(tag->value));
    }

    // Parse creation_time for FIT alignment
    tag = av_dict_get(fmtCtx->metadata, "creation_time", nullptr, 0);
    if (tag) {
        m_info.creationTimeStr = QString(tag->value);
        QDateTime dt = QDateTime::fromString(m_info.creationTimeStr, Qt::ISODate);
        if (dt.isValid()) {
            dt.setTimeZone(QTimeZone::utc());
            m_info.creationTimestamp = static_cast<double>(dt.toSecsSinceEpoch());
        }
    }

    // Find video and audio streams
    for (unsigned i = 0; i < fmtCtx->nb_streams; ++i) {
        AVStream* stream = fmtCtx->streams[i];
        AVCodecParameters* par = stream->codecpar;

        // Extract per-stream metadata
        AVDictionaryEntry* stag = nullptr;
        while ((stag = av_dict_get(stream->metadata, "", stag, AV_DICT_IGNORE_SUFFIX))) {
            QString key = QString("stream%1_%2").arg(i).arg(stag->key);
            m_info.metadata.insert(key, QString(stag->value));
        }

        if (par->codec_type == AVMEDIA_TYPE_VIDEO && !m_info.hasVideo) {
            m_info.hasVideo = true;
            m_info.videoWidth = par->width;
            m_info.videoHeight = par->height;
            m_info.videoBitRate = static_cast<int>(par->bit_rate);
            m_info.videoPixelFormat = QString(av_get_pix_fmt_name(
                static_cast<AVPixelFormat>(par->format)));

            const AVCodecDescriptor* desc = avcodec_descriptor_get(par->codec_id);
            m_info.videoCodec = desc ? QString(desc->name) : "unknown";

            // FPS from stream time_base and avg_frame_rate
            if (stream->avg_frame_rate.den > 0 && stream->avg_frame_rate.num > 0) {
                m_info.videoFps = av_q2d(stream->avg_frame_rate);
            } else if (stream->r_frame_rate.den > 0 && stream->r_frame_rate.num > 0) {
                m_info.videoFps = av_q2d(stream->r_frame_rate);
            }

            // Stream-level creation_time (may differ from container)
            tag = av_dict_get(stream->metadata, "creation_time", nullptr, 0);
            if (tag) {
                m_info.metadata.insert("video_creation_time", QString(tag->value));
            }

            // Rotation from side data (FFmpeg 6.1+ uses codecpar side data)
            const AVPacketSideData* sd = av_packet_side_data_get(
                stream->codecpar->coded_side_data, stream->codecpar->nb_coded_side_data,
                AV_PKT_DATA_DISPLAYMATRIX);
            if (sd) {
                m_info.metadata.insert("video_rotation", "has_display_matrix");
            }
        }
        else if (par->codec_type == AVMEDIA_TYPE_AUDIO && !m_info.hasAudio) {
            m_info.hasAudio = true;
            m_info.audioSampleRate = par->sample_rate;
            m_info.audioChannels = par->ch_layout.nb_channels;
            m_info.audioBitRate = static_cast<int>(par->bit_rate);

            const AVCodecDescriptor* desc = avcodec_descriptor_get(par->codec_id);
            m_info.audioCodec = desc ? QString(desc->name) : "unknown";
        }
    }

    avformat_close_input(&fmtCtx);
    return true;
#else
    Q_UNUSED(filePath);
    m_error = "FFmpeg not available";
    return false;
#endif
}
