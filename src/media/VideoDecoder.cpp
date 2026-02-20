#include "VideoDecoder.h"

VideoDecoder::VideoDecoder(QObject* parent) : QObject(parent) {}

VideoDecoder::~VideoDecoder() {
    close();
}

bool VideoDecoder::open(const QString& filePath) {
#ifdef HAS_FFMPEG
    // TODO: Phase 3 - FFmpeg av_open_input, find stream, open codec
    Q_UNUSED(filePath);
    return false;
#else
    Q_UNUSED(filePath);
    return false;
#endif
}

void VideoDecoder::close() {
    m_isOpen = false;
    m_currentTime = 0.0;
    m_info = VideoInfo{};
}

QImage VideoDecoder::decodeNextFrame() {
#ifdef HAS_FFMPEG
    // TODO: Phase 3 - av_read_frame, avcodec_send/receive, sws_scale -> QImage
#endif
    return QImage();
}

bool VideoDecoder::seek(double seconds) {
#ifdef HAS_FFMPEG
    // TODO: Phase 3 - av_seek_frame
    Q_UNUSED(seconds);
#else
    Q_UNUSED(seconds);
#endif
    return false;
}
