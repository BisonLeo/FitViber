#include "PreviewWidget.h"
#include "TimeUtil.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>

PreviewWidget::PreviewWidget(QWidget* parent) : QWidget(parent) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Frame display
    m_frameLabel = new QLabel(this);
    m_frameLabel->setAlignment(Qt::AlignCenter);
    m_frameLabel->setMinimumSize(320, 240);
    m_frameLabel->setStyleSheet("background-color: #1e1e1e;");
    m_frameLabel->setText("No Video");
    mainLayout->addWidget(m_frameLabel, 1);

    // Controls bar
    auto* controlsLayout = new QHBoxLayout();
    controlsLayout->setContentsMargins(8, 4, 8, 4);

    m_stepBackButton = new QPushButton("<", this);
    m_stepBackButton->setFixedWidth(30);
    controlsLayout->addWidget(m_stepBackButton);

    m_playButton = new QPushButton("Play", this);
    m_playButton->setFixedWidth(60);
    controlsLayout->addWidget(m_playButton);

    m_stepForwardButton = new QPushButton(">", this);
    m_stepForwardButton->setFixedWidth(30);
    controlsLayout->addWidget(m_stepForwardButton);

    m_seekSlider = new QSlider(Qt::Horizontal, this);
    m_seekSlider->setRange(0, 10000);
    controlsLayout->addWidget(m_seekSlider, 1);

    m_timeLabel = new QLabel("0:00 / 0:00", this);
    m_timeLabel->setFixedWidth(120);
    controlsLayout->addWidget(m_timeLabel);

    mainLayout->addLayout(controlsLayout);

    // Connections
    connect(m_playButton, &QPushButton::clicked, this, &PreviewWidget::playPauseClicked);
    connect(m_stepBackButton, &QPushButton::clicked, this, &PreviewWidget::stepBackward);
    connect(m_stepForwardButton, &QPushButton::clicked, this, &PreviewWidget::stepForward);
    connect(m_seekSlider, &QSlider::sliderMoved, this, [this](int value) {
        double time = m_duration * value / 10000.0;
        emit seekRequested(time);
    });
}

PreviewWidget::~PreviewWidget() = default;

void PreviewWidget::displayFrame(const QImage& frame) {
    m_currentFrame = frame;
    QPixmap pixmap = QPixmap::fromImage(frame).scaled(
        m_frameLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_frameLabel->setPixmap(pixmap);
}

void PreviewWidget::setDuration(double seconds) {
    m_duration = seconds;
}

void PreviewWidget::setCurrentTime(double seconds) {
    m_timeLabel->setText(QString("%1 / %2")
        .arg(TimeUtil::secondsToMMSS(seconds))
        .arg(TimeUtil::secondsToMMSS(m_duration)));

    if (!m_seekSlider->isSliderDown() && m_duration > 0) {
        m_seekSlider->setValue(static_cast<int>(seconds / m_duration * 10000));
    }
}
