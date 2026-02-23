#include "InclinationPanel.h"
#include <QLinearGradient>
#include <QPolygonF>

InclinationPanel::InclinationPanel(QObject* parent) : OverlayPanel(PanelType::Inclination, parent) {
    m_config.label = defaultLabel();
    m_config.x = 0.86;
    m_config.y = 0.35;
    m_config.width = 0.10;
    m_config.height = 0.10;
}

void InclinationPanel::paint(QPainter& painter, const QRect& rect,
                             const FitRecord& record, const FitSession&) {
    double scale = rect.height() / 100.0;
    QPointF base = rect.topRight() + QPointF(-10 * scale, 64 * scale);
    
    float inclination = record.grade;
    QString incStr = QString::number(qRound(inclination));
    
    // Draw Text
    double unitW = drawSvgText(painter, base + QPointF(-25 * scale, -15 * scale), "°", 36 * scale, Qt::white, Qt::AlignRight, true);
    drawSvgText(painter, base + QPointF(-25 * scale - unitW - 10 * scale, 0), incStr, 64 * scale, Qt::white, Qt::AlignRight, true);
    drawSvgText(painter, base + QPointF(-25 * scale, 35 * scale), m_config.label, 22 * scale, QColor(221, 221, 221), Qt::AlignRight, false);
    
    // Draw Triangle Background
    double x = base.x() - 8 * scale;
    double y = base.y() + 80 * scale;
    double w = 80 * scale;
    double h = 35 * scale;
    
    QPolygonF bgPoly;
    bgPoly << QPointF(x, y) << QPointF(x - w, y) << QPointF(x, y - h);
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 76)); // 0.3 alpha
    painter.drawPolygon(bgPoly);
    
    // Draw Triangle Foreground
    float progress = qBound(0.0f, (inclination + 100.0f) / 200.0f, 1.0f);
    
    if (progress > 0) {
        QPolygonF fgPoly;
        fgPoly << QPointF(x, y) << QPointF(x - w * progress, y) << QPointF(x, y - h * progress);
        
        QLinearGradient grad(x - w, y, x, y);
        grad.setColorAt(0.0, QColor("#3BAEEB"));
        grad.setColorAt(1.0, QColor("#93EDAA"));
        
        painter.setBrush(grad);
        painter.drawPolygon(fgPoly);
    }
}
