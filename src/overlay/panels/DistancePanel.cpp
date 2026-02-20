#include "DistancePanel.h"

DistancePanel::DistancePanel(QObject* parent) : OverlayPanel(PanelType::Distance, parent) {
    m_config.x = 0.82;
    m_config.y = 0.85;
}

void DistancePanel::paint(QPainter& painter, const QRect& rect,
                           const FitRecord& record, const FitSession&) {
    paintBackground(painter, rect);
    paintLabel(painter, rect, m_config.label);
    float km = record.distance / 1000.0f;
    paintValue(painter, rect, QString::number(km, 'f', 2), "km");
}
