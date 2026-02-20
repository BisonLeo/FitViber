#include "PowerPanel.h"

PowerPanel::PowerPanel(QObject* parent) : OverlayPanel(PanelType::Power, parent) {
    m_config.label = defaultLabel();
    m_config.x = 0.50;
    m_config.y = 0.85;
    m_config.textColor = QColor(255, 200, 50);
}

void PowerPanel::paint(QPainter& painter, const QRect& rect,
                        const FitRecord& record, const FitSession&) {
    if (!record.hasPower) return;
    paintBackground(painter, rect);
    paintLabel(painter, rect, m_config.label);
    paintValue(painter, rect, QString::number(static_cast<int>(record.power)), "W");
}
