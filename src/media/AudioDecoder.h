#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>

struct AudioInfo {
    int sampleRate = 0;
    int channels = 0;
    int bitsPerSample = 16;
    double duration = 0.0;
    QString codecName;
};

class AudioDecoder : public QObject {
    Q_OBJECT
public:
    explicit AudioDecoder(QObject* parent = nullptr);
    ~AudioDecoder();

    bool open(const QString& filePath);
    void close();
    bool isOpen() const { return m_isOpen; }

    // Decode up to maxSeconds of audio, returns interleaved S16 PCM
    QByteArray decode(double maxSeconds = -1.0);
    bool seek(double seconds);
    const AudioInfo& info() const { return m_info; }

private:
    bool m_isOpen = false;
    AudioInfo m_info;

#ifdef HAS_FFMPEG
    struct FFmpegAudioContext;
    std::unique_ptr<FFmpegAudioContext> m_ctx;
#endif
};
