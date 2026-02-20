#include "AudioDecoder.h"

AudioDecoder::AudioDecoder(QObject* parent) : QObject(parent) {}
AudioDecoder::~AudioDecoder() { close(); }

bool AudioDecoder::open(const QString& filePath) {
    Q_UNUSED(filePath);
    // TODO: Phase 11 - FFmpeg audio decode
    return false;
}

void AudioDecoder::close() {
    m_isOpen = false;
    m_info = AudioInfo{};
}

QByteArray AudioDecoder::decodeAll() {
    return QByteArray();
}

bool AudioDecoder::seek(double seconds) {
    Q_UNUSED(seconds);
    return false;
}
