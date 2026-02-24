#pragma once

#include <QObject>
#include <QTimer>

enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};

class PlaybackController : public QObject {
    Q_OBJECT
public:
    explicit PlaybackController(QObject* parent = nullptr);
    ~PlaybackController();

    void play();
    void pause();
    void stop();
    void togglePlayPause();

    void seek(double seconds);
    void stepForward();
    void stepBackward();

    void setFps(double fps);
    void setDuration(double duration);
    void setStartTime(double startTime);

    PlaybackState state() const { return m_state; }
    double currentTime() const { return m_currentTime; }
    double fps() const { return m_fps; }

signals:
    void tick(double currentTime);
    void stateChanged(PlaybackState state);
    void seekPerformed(double seconds);

private slots:
    void onTimer();

private:
    QTimer m_timer;
    PlaybackState m_state = PlaybackState::Stopped;
    double m_currentTime = 0.0;
    double m_startTime = 0.0;
    double m_duration = 0.0;
    double m_fps = 30.0;
};
