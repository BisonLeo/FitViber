#include "MediaProbe.h"

MediaProbe::MediaProbe(QObject* parent) : QObject(parent) {}
MediaProbe::~MediaProbe() = default;

bool MediaProbe::probe(const QString& filePath) {
    m_info.filePath = filePath;

#ifdef HAS_FFMPEG
    // TODO: Phase 3 - avformat_open_input, avformat_find_stream_info
    m_error = "MediaProbe not yet implemented";
    return false;
#else
    m_error = "FFmpeg not available";
    return false;
#endif
}
