#include "DistancePanel.h"

DistancePanel::DistancePanel(QObject* parent) : OverlayPanel(PanelType::Distance, parent) {
    m_config.label = defaultLabel();
    m_config.x = 0.03;
    m_config.y = 0.40;
    m_config.width = 0.10;
    m_config.height = 0.10;
}

void DistancePanel::paint(QPainter& painter, const QRect& rect,
                           const FitRecord& record, const FitSession&) {
    double scale = rect.height() / 100.0;
    QPointF base = rect.topLeft() + QPointF(0, 64 * scale);
    
    float km = record.distance / 1000.0f;
    QString distStr = QString::number(km, 'f', 1);
    
    double valW = drawSvgText(painter, base, distStr, 64 * scale, Qt::white, Qt::AlignLeft, true);
    drawSvgText(painter, base + QPointF(valW + 15 * scale, 0), "Km", 36 * scale, Qt::white, Qt::AlignLeft, true);
    drawSvgText(painter, base + QPointF(0, 35 * scale), m_config.label, 22 * scale, QColor(221, 221, 221), Qt::AlignLeft, false);
}
