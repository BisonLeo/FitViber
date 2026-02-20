#pragma once

#include <QObject>
#include <QString>
#include <QMap>

struct MediaInfo {
    QString filePath;
    QString containerFormat;
    double duration = 0.0;
    int videoWidth = 0;
    int videoHeight = 0;
    double videoFps = 0.0;
    QString videoCodec;
    int videoBitRate = 0;
    QString videoPixelFormat;
    int audioSampleRate = 0;
    int audioChannels = 0;
    QString audioCodec;
    int audioBitRate = 0;
    bool hasVideo = false;
    bool hasAudio = false;

    // Metadata for FIT alignment
    double creationTimestamp = 0.0;  // Unix timestamp from creation_time metadata
    QString creationTimeStr;         // Raw creation_time string

    // All metadata key-value pairs
    QMap<QString, QString> metadata;
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
