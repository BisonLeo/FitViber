#pragma once

#include <QObject>
#include <QString>

struct MediaInfo {
    QString filePath;
    QString containerFormat;
    double duration = 0.0;
    int videoWidth = 0;
    int videoHeight = 0;
    double videoFps = 0.0;
    QString videoCodec;
    int audioSampleRate = 0;
    int audioChannels = 0;
    QString audioCodec;
    bool hasVideo = false;
    bool hasAudio = false;
};

class MediaProbe : public QObject {
    Q_OBJECT
public:
    explicit MediaProbe(QObject* parent = nullptr);
    ~MediaProbe();

    bool probe(const QString& filePath);
    const MediaInfo& info() const { return m_info; }
    QString errorString() const { return m_error; }

private:
    MediaInfo m_info;
    QString m_error;
};
