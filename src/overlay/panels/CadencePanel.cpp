#include "CadencePanel.h"

CadencePanel::CadencePanel(QObject* parent) : OverlayPanel(PanelType::Cadence, parent) {
    m_config.label = defaultLabel();
    m_config.x = 0.34;
    m_config.y = 0.85;
    m_config.textColor = QColor(80, 200, 255);
}

void CadencePanel::paint(QPainter& painter, const QRect& rect,
                          const FitRecord& record, const FitSession&) {
    if (!record.hasCadence) return;
    paintBackground(painter, rect);
    paintLabel(painter, rect, m_config.label);
    paintValue(painter, rect, QString::number(static_cast<int>(record.cadence)), "rpm");
}
