#include "ElevationPanel.h"

ElevationPanel::ElevationPanel(QObject* parent) : OverlayPanel(PanelType::Elevation, parent) {
    m_config.x = 0.66;
    m_config.y = 0.85;
    m_config.textColor = QColor(100, 220, 100);
}

void ElevationPanel::paint(QPainter& painter, const QRect& rect,
                            const FitRecord& record, const FitSession&) {
    paintBackground(painter, rect);
    paintLabel(painter, rect, m_config.label);
    paintValue(painter, rect, QString::number(static_cast<int>(record.altitude)), "m");
}
