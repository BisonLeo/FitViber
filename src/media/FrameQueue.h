#pragma once

#include <QImage>
#include <QMutex>
#include <QWaitCondition>
#include <queue>

struct TimedFrame {
    QImage image;
    double pts = 0.0;  // presentation timestamp in seconds
};

class FrameQueue {
public:
    explicit FrameQueue(int maxSize = 30) : m_maxSize(maxSize) {}

    void push(const TimedFrame& frame) {
        QMutexLocker lock(&m_mutex);
        while (static_cast<int>(m_queue.size()) >= m_maxSize) {
            m_notFull.wait(&m_mutex);
        }
        m_queue.push(frame);
        m_notEmpty.wakeOne();
    }

    TimedFrame pop() {
        QMutexLocker lock(&m_mutex);
        while (m_queue.empty()) {
            m_notEmpty.wait(&m_mutex);
        }
        TimedFrame frame = m_queue.front();
        m_queue.pop();
        m_notFull.wakeOne();
        return frame;
    }

    bool tryPop(TimedFrame& frame) {
        QMutexLocker lock(&m_mutex);
        if (m_queue.empty()) return false;
        frame = m_queue.front();
        m_queue.pop();
        m_notFull.wakeOne();
        return true;
    }

    void clear() {
        QMutexLocker lock(&m_mutex);
        while (!m_queue.empty()) m_queue.pop();
        m_notFull.wakeAll();
    }

    int size() const {
        QMutexLocker lock(&m_mutex);
        return static_cast<int>(m_queue.size());
    }

    bool isEmpty() const {
        QMutexLocker lock(&m_mutex);
        return m_queue.empty();
    }

private:
    mutable QMutex m_mutex;
    QWaitCondition m_notEmpty;
    QWaitCondition m_notFull;
    std::queue<TimedFrame> m_queue;
    int m_maxSize;
};
