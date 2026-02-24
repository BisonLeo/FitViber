#pragma once

#include <QWidget>
#include <QImage>
#include <QSize>
#include <QSlider>
#include <QPushButton>
#include <QLabel>

class PreviewCanvas;
struct ClipTransform;

class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget* parent = nullptr);
    ~PreviewWidget();

    void displayFrame(const QImage& frame);
    void setDuration(double seconds);
    void setStartTime(double seconds);
    void setCurrentTime(double seconds);

    void showImage(const QImage& image);
    void showVideo();
    void setPlayingState(bool playing);

    void setClipTransform(ClipTransform* transform);
    void setHandlesVisible(bool visible);
    void setCanvasSize(QSize canvasSize);
    void setSourceSize(QSize sourceSize);
    void setComposited(bool composited);

signals:
    void playPauseClicked();
    void seekRequested(double seconds);
    void stepForward();
    void stepBackward();
    void videoAreaClicked();
    void transformChanged();

private:
    PreviewCanvas* m_canvas;
    QWidget* m_controlsBar;
    QSlider* m_seekSlider;
    QPushButton* m_playButton;
    QPushButton* m_stepBackButton;
    QPushButton* m_stepForwardButton;
    QLabel* m_timeLabel;

    double m_startTime = 0.0;
    double m_duration = 0.0;
    QImage m_currentFrame;
    bool m_videoMode = false;
};
