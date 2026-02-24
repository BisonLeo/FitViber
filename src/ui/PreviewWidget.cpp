#include "PreviewWidget.h"
#include "PreviewCanvas.h"
#include "TimeUtil.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

PreviewWidget::PreviewWidget(QWidget* parent) : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Frame display
    m_canvas = new PreviewCanvas(this);
    mainLayout->addWidget(m_canvas, 1);

    // Controls bar
    m_controlsBar = new QWidget(this);
    auto* controlsLayout = new QHBoxLayout(m_controlsBar);
    controlsLayout->setContentsMargins(8, 4, 8, 4);

    m_stepBackButton = new QPushButton("\u25C0", m_controlsBar);
    m_stepBackButton->setFixedWidth(30);
    controlsLayout->addWidget(m_stepBackButton);

    m_playButton = new QPushButton("\u25B6", m_controlsBar);
    m_playButton->setFixedWidth(60);
    controlsLayout->addWidget(m_playButton);

    m_stepForwardButton = new QPushButton("\u25B6", m_controlsBar);
    m_stepForwardButton->setFixedWidth(30);
    controlsLayout->addWidget(m_stepForwardButton);

    m_seekSlider = new QSlider(Qt::Horizontal, m_controlsBar);
    m_seekSlider->setRange(0, 10000);
    controlsLayout->addWidget(m_seekSlider, 1);

    m_timeLabel = new QLabel("00:00:00.000 / 00:00:00.000  0.0%", m_controlsBar);
    m_timeLabel->setFixedWidth(300);
    controlsLayout->addWidget(m_timeLabel);

    mainLayout->addWidget(m_controlsBar);
    m_controlsBar->setVisible(false);

    // Connections
    connect(m_playButton, &QPushButton::clicked, this, &PreviewWidget::playPauseClicked);
    connect(m_stepBackButton, &QPushButton::clicked, this, &PreviewWidget::stepBackward);
    connect(m_stepForwardButton, &QPushButton::clicked, this, &PreviewWidget::stepForward);
    connect(m_seekSlider, &QSlider::sliderMoved, this, [this](int value) {
        double time = m_duration * value / 10000.0;
        emit seekRequested(time);
    });

    connect(m_canvas, &PreviewCanvas::clicked, this, &PreviewWidget::videoAreaClicked);
    connect(m_canvas, &PreviewCanvas::transformChanged, this, &PreviewWidget::transformChanged);
}

PreviewWidget::~PreviewWidget() = default;

void PreviewWidget::displayFrame(const QImage& frame) {
    m_currentFrame = frame;
    m_canvas->setFrame(frame);
}

void PreviewWidget::setDuration(double seconds) {
    m_duration = seconds;
}

void PreviewWidget::setCurrentTime(double seconds) {
    double pct = (m_duration > 0) ? (seconds / m_duration * 100.0) : 0.0;
    m_timeLabel->setText(QString("%1 / %2  %3%")
        .arg(TimeUtil::secondsToHMSms(seconds))
        .arg(TimeUtil::secondsToHMSms(m_duration))
        .arg(pct, 0, 'f', 1));

    if (!m_seekSlider->isSliderDown() && m_duration > 0) {
        m_seekSlider->setValue(static_cast<int>(seconds / m_duration * 10000));
    }
}

void PreviewWidget::showImage(const QImage& image) {
    m_videoMode = false;
    m_controlsBar->setVisible(false);
    m_duration = 0.0;
    displayFrame(image);
}

void PreviewWidget::showVideo() {
    m_videoMode = true;
    m_controlsBar->setVisible(true);
}

void PreviewWidget::setPlayingState(bool playing) {
    m_playButton->setText(playing ? "\u23F8" : "\u25B6");
}

void PreviewWidget::setClipTransform(ClipTransform* transform) {
    m_canvas->setTransform(transform);
}

void PreviewWidget::setHandlesVisible(bool visible) {
    m_canvas->setHandlesVisible(visible);
}

void PreviewWidget::setCanvasSize(QSize canvasSize) {
    m_canvas->setCanvasSize(canvasSize);
}

void PreviewWidget::setSourceSize(QSize sourceSize) {
    m_canvas->setSourceSize(sourceSize);
}

void PreviewWidget::setComposited(bool composited) {
    m_canvas->setComposited(composited);
}
