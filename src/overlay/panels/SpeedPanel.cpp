#include "SpeedPanel.h"

SpeedPanel::SpeedPanel(QObject* parent) : OverlayPanel(PanelType::Speed, parent) {
    m_config.x = 0.02;
    m_config.y = 0.85;
}

void SpeedPanel::paint(QPainter& painter, const QRect& rect,
                        const FitRecord& record, const FitSession&) {
    paintBackground(painter, rect);
    paintLabel(painter, rect, m_config.label);
    float kmh = record.speed * 3.6f;
    paintValue(painter, rect, QString::number(kmh, 'f', 1), "km/h");
}
