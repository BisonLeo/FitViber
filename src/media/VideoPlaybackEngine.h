#pragma once

#include <QObject>
#include <QThread>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>
#include <memory>
#include "FrameQueue.h"
#include "VideoDecoder.h"

class VideoDecoder;

// Background decode thread that sequentially reads frames into a FrameQueue.
// Follows ffplay's architecture: sequential decode, seek only on request
// (flush queue, seek to keyframe, resume sequential decode).
class DecodeThread : public QThread {
    Q_OBJECT
public:
    explicit DecodeThread(VideoDecoder* decoder, FrameQueue* queue, QObject* parent = nullptr);

    void requestSeek(double seconds);
    void requestStop();
    bool isEof() const { return m_eof; }

protected:
    void run() override;

private:
    VideoDecoder* m_decoder;
    FrameQueue* m_queue;

    QMutex m_mutex;
    QWaitCondition m_seekCond;
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_seekRequested{false};
    std::atomic<bool> m_eof{false};
    double m_seekTarget = 0.0;
};

// High-level playback engine: owns a VideoDecoder, DecodeThread, and FrameQueue.
// Provides frame-pulling for the UI timer tick without per-frame seeking.
class VideoPlaybackEngine : public QObject {
    Q_OBJECT
public:
    explicit VideoPlaybackEngine(QObject* parent = nullptr);
    ~VideoPlaybackEngine();

    bool open(const QString& filePath);
    void close();
    bool isOpen() const;

    // Pull the next decoded frame (non-blocking). Returns null QImage if none ready.
    TimedFrame nextFrame();

    // True when decode thread finished all frames AND queue is empty.
    bool isFinished() const;

    // Seek: flushes queue, seeks decoder, resumes decode thread from new position.
    void seek(double seconds);

    // Single-frame decode without the thread (for thumbnails, hover scrub).
    QImage decodeSingleFrame();
    bool seekDirect(double seconds);

    const VideoInfo& info() const;
    VideoDecoder* decoder() const { return m_decoder.get(); }

private:
    std::unique_ptr<VideoDecoder> m_decoder;
    std::unique_ptr<FrameQueue> m_frameQueue;
    std::unique_ptr<DecodeThread> m_decodeThread;
};
