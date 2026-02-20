#pragma once

#include <QWidget>
#include <QImage>
#include <QLabel>
#include <QSlider>
#include <QPushButton>

class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget* parent = nullptr);
    ~PreviewWidget();

    void displayFrame(const QImage& frame);
    void setDuration(double seconds);
    void setCurrentTime(double seconds);

signals:
    void playPauseClicked();
    void seekRequested(double seconds);
    void stepForward();
    void stepBackward();

private:
    QLabel* m_frameLabel;
    QSlider* m_seekSlider;
    QPushButton* m_playButton;
    QPushButton* m_stepBackButton;
    QPushButton* m_stepForwardButton;
    QLabel* m_timeLabel;

    double m_duration = 0.0;
    QImage m_currentFrame;
};
