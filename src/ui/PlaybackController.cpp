#include "PlaybackController.h"

PlaybackController::PlaybackController(QObject* parent)
    : QObject(parent) {
    connect(&m_timer, &QTimer::timeout, this, &PlaybackController::onTimer);
}

PlaybackController::~PlaybackController() {
    m_timer.stop();
}

void PlaybackController::play() {
    if (m_state == PlaybackState::Playing) return;
    m_state = PlaybackState::Playing;
    m_timer.start(static_cast<int>(1000.0 / m_fps));
    emit stateChanged(m_state);
}

void PlaybackController::pause() {
    if (m_state != PlaybackState::Playing) return;
    m_state = PlaybackState::Paused;
    m_timer.stop();
    emit stateChanged(m_state);
}

void PlaybackController::stop() {
    m_timer.stop();
    m_state = PlaybackState::Stopped;
    m_currentTime = m_startTime;
    emit stateChanged(m_state);
    emit tick(m_currentTime);
}

void PlaybackController::togglePlayPause() {
    if (m_state == PlaybackState::Playing) {
        pause();
    } else {
        play();
    }
}

void PlaybackController::seek(double seconds) {
    m_currentTime = qBound(m_startTime, seconds, m_duration);
    emit seekPerformed(m_currentTime);
    emit tick(m_currentTime);
}

void PlaybackController::stepForward() {
    seek(m_currentTime + 1.0 / m_fps);
}

void PlaybackController::stepBackward() {
    seek(m_currentTime - 1.0 / m_fps);
}

void PlaybackController::setFps(double fps) {
    m_fps = fps;
    if (m_state == PlaybackState::Playing) {
        m_timer.setInterval(static_cast<int>(1000.0 / m_fps));
    }
}

void PlaybackController::setDuration(double duration) {
    m_duration = duration;
}

void PlaybackController::setStartTime(double startTime) {
    m_startTime = startTime;
}

void PlaybackController::onTimer() {
    m_currentTime += 1.0 / m_fps;
    if (m_currentTime >= m_duration) {
        m_currentTime = m_duration;
        pause();
    }
    emit tick(m_currentTime);
}
