#include "VideoPlaybackEngine.h"
#include "VideoDecoder.h"

// --- DecodeThread ---

DecodeThread::DecodeThread(VideoDecoder* decoder, FrameQueue* queue, QObject* parent)
    : QThread(parent), m_decoder(decoder), m_queue(queue) {}

void DecodeThread::requestSeek(double seconds) {
    QMutexLocker lock(&m_mutex);
    m_seekTarget = seconds;
    m_seekRequested = true;
    m_eof = false;
    // Wake thread if it's waiting on a full queue or EOF sleep
    m_queue->clear();
    m_seekCond.wakeOne();
}

void DecodeThread::requestStop() {
    m_stopRequested = true;
    m_queue->clear();  // Unblock any push() waiting on full queue
}

void DecodeThread::run() {
    while (!m_stopRequested) {
        // Check for pending seek
        if (m_seekRequested) {
            double target;
            {
                QMutexLocker lock(&m_mutex);
                target = m_seekTarget;
                m_seekRequested = false;
            }
            m_eof = false;
            m_queue->clear();
            m_decoder->seek(target);
        }

        // If we already hit EOF, wait for a seek or stop
        if (m_eof) {
            QMutexLocker lock(&m_mutex);
            if (!m_stopRequested && !m_seekRequested) {
                m_seekCond.wait(&m_mutex, 100);
            }
            continue;
        }

        // Decode next frame sequentially
        QImage frame = m_decoder->decodeNextFrame();
        if (frame.isNull()) {
            // End of stream — all frames (including flushed B-frames) consumed
            m_eof = true;
            continue;
        }

        TimedFrame tf;
        tf.image = frame;
        tf.pts = m_decoder->currentTime();

        // Push blocks if queue is full (backpressure)
        // But we need to check stop/seek periodically
        while (!m_stopRequested && !m_seekRequested) {
            if (m_queue->size() < 8) {
                m_queue->push(tf);
                break;
            }
            // Queue full — wait briefly
            QThread::msleep(2);
        }
    }
}

// --- VideoPlaybackEngine ---

VideoPlaybackEngine::VideoPlaybackEngine(QObject* parent)
    : QObject(parent)
    , m_decoder(std::make_unique<VideoDecoder>())
    , m_frameQueue(std::make_unique<FrameQueue>(8))
{}

VideoPlaybackEngine::~VideoPlaybackEngine() {
    close();
}

bool VideoPlaybackEngine::open(const QString& filePath) {
    close();

    if (!m_decoder->open(filePath))
        return false;

    // Start decode thread
    m_decodeThread = std::make_unique<DecodeThread>(m_decoder.get(), m_frameQueue.get());
    m_decodeThread->start();

    return true;
}

void VideoPlaybackEngine::close() {
    if (m_decodeThread) {
        m_decodeThread->requestStop();
        m_decodeThread->wait(2000);
        m_decodeThread.reset();
    }
    m_frameQueue->clear();
    m_decoder->close();
}

bool VideoPlaybackEngine::isOpen() const {
    return m_decoder->isOpen();
}

TimedFrame VideoPlaybackEngine::nextFrame() {
    TimedFrame tf;
    m_frameQueue->tryPop(tf);
    return tf;
}

bool VideoPlaybackEngine::isFinished() const {
    return m_decodeThread && m_decodeThread->isEof() && m_frameQueue->isEmpty();
}

void VideoPlaybackEngine::seek(double seconds) {
    if (!m_decodeThread) return;
    m_decodeThread->requestSeek(seconds);
}

QImage VideoPlaybackEngine::decodeSingleFrame() {
    return m_decoder->decodeNextFrame();
}

bool VideoPlaybackEngine::seekDirect(double seconds) {
    return m_decoder->seek(seconds);
}

const VideoInfo& VideoPlaybackEngine::info() const {
    return m_decoder->info();
}
