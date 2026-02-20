#pragma once

#include <QObject>
#include <QImage>
#include <QString>
#include <memory>

struct VideoInfo {
    int width = 0;
    int height = 0;
    double fps = 0.0;
    double duration = 0.0;  // seconds
    int64_t totalFrames = 0;
    QString codecName;
};

class VideoDecoder : public QObject {
    Q_OBJECT
public:
    explicit VideoDecoder(QObject* parent = nullptr);
    ~VideoDecoder();

    bool open(const QString& filePath);
    void close();
    bool isOpen() const { return m_isOpen; }

    QImage decodeNextFrame();
    bool seek(double seconds);
    double currentTime() const { return m_currentTime; }

    const VideoInfo& info() const { return m_info; }

signals:
    void frameDecoded(const QImage& frame, double pts);

private:
    bool m_isOpen = false;
    double m_currentTime = 0.0;
    VideoInfo m_info;

#ifdef HAS_FFMPEG
    struct FFmpegContext;
    std::unique_ptr<FFmpegContext> m_ctx;
#endif
};
