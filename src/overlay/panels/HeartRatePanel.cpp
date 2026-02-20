#include "HeartRatePanel.h"

HeartRatePanel::HeartRatePanel(QObject* parent) : OverlayPanel(PanelType::HeartRate, parent) {
    m_config.label = defaultLabel();
    m_config.x = 0.18;
    m_config.y = 0.85;
    m_config.textColor = QColor(255, 80, 80);
}

void HeartRatePanel::paint(QPainter& painter, const QRect& rect,
                            const FitRecord& record, const FitSession&) {
    if (!record.hasHeartRate) return;
    paintBackground(painter, rect);
    paintLabel(painter, rect, m_config.label);
    paintValue(painter, rect, QString::number(static_cast<int>(record.heartRate)), "bpm");
}
