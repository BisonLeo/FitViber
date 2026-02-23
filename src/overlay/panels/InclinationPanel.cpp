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
    
    // Draw Half-Circle Wedge Background
    QPointF arcCenter = base + QPointF(-40 * scale, 60 * scale);
    double r = 45 * scale;
    
    // Qt: 0 is 3 o'clock, 90 is 12 o'clock, 180 is 9 o'clock, 270 is 6 o'clock.
    // "goes from 6 o'clock counterclockwise to 2 o'clock if it is 30 degree"
    // 6 o'clock is 270 degrees in Qt. 
    // Wait, Qt angles are counter-clockwise from 3 o'clock.
    // 3 o'clock = 0
    // 12 o'clock = 90
    // 9 o'clock = 180
    // 6 o'clock = 270
    // Counter-clockwise means positive angle in Qt.
    // If it's 30 degree (inclination = 30):
    // Horizontal right (3 o'clock) is 0. 
    // 30 degrees up from horizontal right would be 30 degrees. (Between 3 and 12, closer to 3).
    // Let's assume 0 degree inclination = horizontal = 0 Qt angle (3 o'clock).
    // Then 30 degree inclination = 30 Qt angle (2 o'clock).
    // And "goes from 6 o'clock counterclockwise to 2 o'clock":
    // This means the wedge starts at 6 o'clock (270) and fills counter-clockwise up to 30.
    // Wait, counter-clockwise from 270 to 30 goes: 270 -> 360(0) -> 30.
    // That's a span of 90 + 30 = 120 degrees.
    // And if inclination is -90 (straight down): 
    // It would be at 6 o'clock (270). Span = 0.
    // If inclination is 90 (straight up):
    // It would be at 12 o'clock (90). Span = 90 + 90 = 180 degrees.
    // So the background should probably be the whole right-half circle, from 6 o'clock to 12 o'clock.
    // 6 o'clock = 270. 12 o'clock = 90.
    // Wait, counter-clockwise from 270 to 90 is 180 degrees span.
    
    // Draw background wedge
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 76)); // 0.3 alpha
    painter.drawPie(QRectF(arcCenter.x() - r, arcCenter.y() - r, r * 2, r * 2), 270 * 16, 180 * 16);
    
    // Draw Foreground Wedge
    float clampedInc = qBound(-90.0f, inclination, 90.0f);
    // Span starts at 270. Ends at clampedInc (where 0 is horizontal right).
    // Span = clampedInc - 270. But since 270 is equivalent to -90, 
    // Span = clampedInc - (-90) = clampedInc + 90.
    float spanDegrees = clampedInc + 90.0f;
    
    if (spanDegrees > 0) {
        QLinearGradient grad(arcCenter.x(), arcCenter.y() + r, arcCenter.x(), arcCenter.y() - r);
        grad.setColorAt(0.0, QColor("#3BAEEB"));
        grad.setColorAt(1.0, QColor("#93EDAA"));
        
        painter.setBrush(grad);
        painter.drawPie(QRectF(arcCenter.x() - r, arcCenter.y() - r, r * 2, r * 2), 270 * 16, spanDegrees * 16);
    }
    
    // Draw slope line indicator (thicker line from origin to angle)
    double targetAngleRad = clampedInc * M_PI / 180.0;
    // Qt: Y is down. Standard math: Y is up.
    // So vector is (cos(angle), -sin(angle)).
    QPointF endPt(arcCenter.x() + std::cos(targetAngleRad) * r,
                  arcCenter.y() - std::sin(targetAngleRad) * r);
                  
    QPen linePen(Qt::white);
    linePen.setWidthF(3 * scale);
    linePen.setCapStyle(Qt::RoundCap);
    painter.setPen(linePen);
    painter.drawLine(arcCenter, endPt);
    
    // Draw dot at end of slope line
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawEllipse(endPt, 4 * scale, 4 * scale);
}
