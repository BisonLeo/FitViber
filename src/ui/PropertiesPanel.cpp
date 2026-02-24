#include "PropertiesPanel.h"
#include "ClipTransform.h"
#include <QPushButton>
#include <QHBoxLayout>

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

    // Transform controls group
    m_transformGroup = new QGroupBox("Transform", m_contentWidget);
    auto* tLayout = new QVBoxLayout(m_transformGroup);

    // Flip buttons row
    auto* flipRow = new QHBoxLayout();
    auto* flipHBtn = new QPushButton("Flip H", m_transformGroup);
    auto* flipVBtn = new QPushButton("Flip V", m_transformGroup);
    flipRow->addWidget(flipHBtn);
    flipRow->addWidget(flipVBtn);
    tLayout->addLayout(flipRow);

    // Rotate buttons row
    auto* rotRow = new QHBoxLayout();
    auto* rotCWBtn = new QPushButton("Rot CW 90", m_transformGroup);
    auto* rotCCWBtn = new QPushButton("Rot CCW 90", m_transformGroup);
    rotRow->addWidget(rotCCWBtn);
    rotRow->addWidget(rotCWBtn);
    tLayout->addLayout(rotRow);

    // Reset button
    auto* resetBtn = new QPushButton("Reset Transform", m_transformGroup);
    tLayout->addWidget(resetBtn);

    // Read-only labels
    m_scaleLabel = new QLabel("Scale: 1.00", m_transformGroup);
    m_rotationLabel = new QLabel("Rotation: 0.0", m_transformGroup);
    m_panLabel = new QLabel("Pan: 0, 0", m_transformGroup);
    tLayout->addWidget(m_scaleLabel);
    tLayout->addWidget(m_rotationLabel);
    tLayout->addWidget(m_panLabel);

    m_contentLayout->addWidget(m_transformGroup);
    m_transformGroup->setVisible(false);

    // Button connections
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
    // TODO: Phase 9 - build UI for each panel config
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

    m_clipInfoLabel->setText(text);
}

void PropertiesPanel::clearClipInfo() {
    m_clipInfoGroup->setVisible(false);
    m_clipInfoLabel->clear();
}
