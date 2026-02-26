#include "PropertiesPanel.h"
#include "ClipTransform.h"
#include <QPushButton>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDateTime>
#include <QTimeZone>

PropertiesPanel::PropertiesPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);

    m_contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setAlignment(Qt::AlignTop);

    // Clip info group
    m_clipInfoGroup = new QGroupBox("Clip Info", m_contentWidget);
    auto* infoLayout = new QVBoxLayout(m_clipInfoGroup);
    m_clipInfoLabel = new QLabel(m_clipInfoGroup);
    m_clipInfoLabel->setWordWrap(true);
    m_clipInfoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    infoLayout->addWidget(m_clipInfoLabel);
    m_contentLayout->addWidget(m_clipInfoGroup);
    m_clipInfoGroup->setVisible(false);

    // Clip placement group
    m_placementGroup = new QGroupBox("Clip Placement", m_contentWidget);
    auto* placementLayout = new QVBoxLayout(m_placementGroup);

    auto* placementForm = new QFormLayout();

    m_startSpin = new TimeSpinBox(m_placementGroup);
    placementForm->addRow("Start:", m_startSpin);

    m_endSpin = new TimeSpinBox(m_placementGroup);
    placementForm->addRow("End:", m_endSpin);

    m_durationSpin = new TimeSpinBox(m_placementGroup);
    m_durationSpin->setRange(0.0, 1e9);
    m_durationSpin->setReadOnly(true);
    m_durationSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_durationSpin->setFocusPolicy(Qt::NoFocus);
    placementForm->addRow("Duration:", m_durationSpin);

    placementLayout->addLayout(placementForm);

    // Time format toggle — default unchecked = HH:MM:SS.sss (absolute clock time)
    m_timeFormatCheck = new QCheckBox("Show seconds", m_placementGroup);
    m_timeFormatCheck->setChecked(false);
    placementLayout->addWidget(m_timeFormatCheck);

    connect(m_timeFormatCheck, &QCheckBox::toggled, this, [this](bool showSeconds) {
        bool hms = !showSeconds;
        m_startSpin->setTimeMode(hms);
        m_endSpin->setTimeMode(hms);
        m_durationSpin->setTimeMode(hms);
    });

    m_contentLayout->addWidget(m_placementGroup);
    m_placementGroup->setVisible(false);

    // When user edits Start: move clip, keep duration, update end
    connect(m_startSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
        if (m_placementUpdating) return;
        m_placementUpdating = true;
        double dur = m_durationSpin->value();
        m_endSpin->setValue(val + dur);
        m_placementUpdating = false;
        emit placementChanged(val, dur);
    });

    // When user edits End: keep duration, adjust start (shift the clip)
    connect(m_endSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
        if (m_placementUpdating) return;
        m_placementUpdating = true;
        double dur = m_durationSpin->value();
        double newStart = val - dur;
        m_startSpin->setValue(newStart);
        m_placementUpdating = false;
        emit placementChanged(newStart, dur);
    });

    // Transform controls group
    m_transformGroup = new QGroupBox("Transform", m_contentWidget);
    auto* tLayout = new QVBoxLayout(m_transformGroup);

    auto* flipRow = new QHBoxLayout();
    auto* flipHBtn = new QPushButton("Flip H", m_transformGroup);
    auto* flipVBtn = new QPushButton("Flip V", m_transformGroup);
    flipRow->addWidget(flipHBtn);
    flipRow->addWidget(flipVBtn);
    tLayout->addLayout(flipRow);

    auto* rotRow = new QHBoxLayout();
    auto* rotCWBtn = new QPushButton("Rot CW 90", m_transformGroup);
    auto* rotCCWBtn = new QPushButton("Rot CCW 90", m_transformGroup);
    rotRow->addWidget(rotCCWBtn);
    rotRow->addWidget(rotCWBtn);
    tLayout->addLayout(rotRow);

    auto* resetBtn = new QPushButton("Reset Transform", m_transformGroup);
    tLayout->addWidget(resetBtn);

    m_scaleLabel = new QLabel("Scale: 1.00", m_transformGroup);
    m_rotationLabel = new QLabel("Rotation: 0.0", m_transformGroup);
    m_panLabel = new QLabel("Pan: 0, 0", m_transformGroup);
    tLayout->addWidget(m_scaleLabel);
    tLayout->addWidget(m_rotationLabel);
    tLayout->addWidget(m_panLabel);

    m_contentLayout->addWidget(m_transformGroup);
    m_transformGroup->setVisible(false);

    connect(flipHBtn, &QPushButton::clicked, this, [this]() {
        if (!m_transform) return;
        m_transform->flipH = !m_transform->flipH;
        updateTransformLabels();
        emit transformChanged();
    });
    connect(flipVBtn, &QPushButton::clicked, this, [this]() {
        if (!m_transform) return;
        m_transform->flipV = !m_transform->flipV;
        updateTransformLabels();
        emit transformChanged();
    });
    connect(rotCWBtn, &QPushButton::clicked, this, [this]() {
        if (!m_transform) return;
        m_transform->rotation += 90.0;
        updateTransformLabels();
        emit transformChanged();
    });
    connect(rotCCWBtn, &QPushButton::clicked, this, [this]() {
        if (!m_transform) return;
        m_transform->rotation -= 90.0;
        updateTransformLabels();
        emit transformChanged();
    });
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        if (!m_transform) return;
        m_transform->reset();
        updateTransformLabels();
        emit transformChanged();
    });

    auto* placeholder = new QLabel("Load a FIT file to configure overlays", m_contentWidget);
    placeholder->setWordWrap(true);
    placeholder->setAlignment(Qt::AlignCenter);
    m_contentLayout->addWidget(placeholder);

    m_scrollArea->setWidget(m_contentWidget);
    layout->addWidget(m_scrollArea);
}

PropertiesPanel::~PropertiesPanel() = default;

void PropertiesPanel::setPanelConfigs(const std::vector<PanelConfig>& configs) {
    Q_UNUSED(configs);
}

void PropertiesPanel::setClipTransform(ClipTransform* transform) {
    m_transform = transform;
    m_transformGroup->setVisible(transform != nullptr);
    if (transform) {
        updateTransformLabels();
    }
}

void PropertiesPanel::updateTransformLabels() {
    if (!m_transform) return;
    m_scaleLabel->setText(QString("Scale: %1").arg(m_transform->scale, 0, 'f', 2));
    m_rotationLabel->setText(QString("Rotation: %1").arg(m_transform->rotation, 0, 'f', 1));
    m_panLabel->setText(QString("Pan: %1, %2")
        .arg(static_cast<int>(m_transform->panX))
        .arg(static_cast<int>(m_transform->panY)));
}

void PropertiesPanel::setClipInfo(const ClipInfo& info) {
    m_clipInfoGroup->setVisible(true);

    QString text;
    text += QString("<b>Type:</b> %1<br>").arg(info.type);
    text += QString("<b>Path:</b> %1<br>").arg(info.path);

    if (info.type == "Video" || info.type == "Image") {
        text += QString("<b>Size:</b> %1 x %2<br>").arg(info.width).arg(info.height);
        if (info.fps > 0)
            text += QString("<b>Frame Rate:</b> %1 fps<br>").arg(info.fps, 0, 'f', 2);
        if (info.totalFrames > 0)
            text += QString("<b>Total Frames:</b> %1<br>").arg(info.totalFrames);
        if (info.totalSeconds > 0) {
            int ts = static_cast<int>(info.totalSeconds);
            int h = ts / 3600, m = (ts % 3600) / 60, s = ts % 60;
            QString durStr = h > 0
                ? QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'))
                : QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
            text += QString("<b>Duration:</b> %1 (%2s)<br>").arg(durStr).arg(info.totalSeconds, 0, 'f', 2);
        }
        if (!info.codec.isEmpty())
            text += QString("<b>Codec:</b> %1<br>").arg(info.codec);
    } else if (info.type == "FIT Data") {
        if (!info.firstTimestamp.isEmpty())
            text += QString("<b>Start:</b> %1<br>").arg(info.firstTimestamp);
        text += QString("<b>Records:</b> %1<br>").arg(info.totalRecords);
        if (info.totalSeconds > 0) {
            int ts = static_cast<int>(info.totalSeconds);
            int h = ts / 3600, m = (ts % 3600) / 60, s = ts % 60;
            QString durStr = h > 0
                ? QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'))
                : QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
            text += QString("<b>Duration:</b> %1<br>").arg(durStr);
        }
        double distKm = info.totalDistance / 1000.0;
        if (distKm >= 1.0)
            text += QString("<b>Distance:</b> %1 km<br>").arg(distKm, 0, 'f', 2);
        else
            text += QString("<b>Distance:</b> %1 m<br>").arg(static_cast<int>(info.totalDistance));
    }

    if (info.detectedStartTimestamp > 0) {
        QDateTime startDt = QDateTime::fromSecsSinceEpoch(
            static_cast<qint64>(info.detectedStartTimestamp), QTimeZone(8 * 3600));
        text += QString("<b>Detected Start:</b> %1<br>").arg(startDt.toString("yyyy-MM-dd HH:mm:ss"));

        if (info.detectedEndTimestamp > info.detectedStartTimestamp) {
            QDateTime endDt = QDateTime::fromSecsSinceEpoch(
                static_cast<qint64>(info.detectedEndTimestamp), QTimeZone(8 * 3600));
            text += QString("<b>Detected End:</b> %1<br>").arg(endDt.toString("yyyy-MM-dd HH:mm:ss"));
        }
    }

    m_clipInfoLabel->setText(text);
}

void PropertiesPanel::clearClipInfo() {
    m_clipInfoGroup->setVisible(false);
    m_clipInfoLabel->clear();
}

void PropertiesPanel::setClipPlacement(const ClipPlacement& placement) {
    m_placementGroup->setVisible(true);
    m_placementUpdating = true;

    // Set timeOrigin on start/end for absolute clock time display.
    // Duration always shows relative (no timeOrigin offset).
    m_startSpin->setTimeOrigin(placement.timeOrigin);
    m_endSpin->setTimeOrigin(placement.timeOrigin);

    m_startSpin->setValue(placement.startPos);
    m_durationSpin->setValue(placement.duration);
    m_endSpin->setValue(placement.endPos);
    m_placementUpdating = false;
}

void PropertiesPanel::clearClipPlacement() {
    m_placementGroup->setVisible(false);
}
