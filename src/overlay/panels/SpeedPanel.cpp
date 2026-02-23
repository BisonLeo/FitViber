#include "SpeedPanel.h"
#include <QLinearGradient>
#include <QDateTime>

SpeedPanel::SpeedPanel(QObject* parent) : OverlayPanel(PanelType::Speed, parent) {
    m_config.label = defaultLabel();
    m_config.x = 0.75;
    m_config.y = 0.70;
    m_config.width = 0.20;
    m_config.height = 0.25;
}

void SpeedPanel::paint(QPainter& painter, const QRect& rect,
                        const FitRecord& record, const FitSession&) {
    double scale = rect.height() / 340.0;
    QPointF center(rect.center().x(), rect.top() + 170 * scale);
    
    double r = 170 * scale;
    // Qt arcs: 0=3 o'clock, 90=12 o'clock, 180=9 o'clock.
    // SVG start: bottom-left (-130, 110). atan2(-110, -130) * 180/PI = 220.2 deg.
    // End: bottom-right (130, 110). atan2(-110, 130) * 180/PI = 319.8 deg (or -40.2 deg).
    // Sweep is clockwise, so negative in Qt.
    double startAngle = 220.0;
    double totalSpan = -260.0;
    
    // Background arc
    drawSvgArc(painter, center, r, startAngle, totalSpan, 24 * scale, QColor(255, 255, 255, 75));
    
    // Foreground arc
    float maxSpeed = 200.0f; // max km/h
    float speed = record.speed * 3.6f;
    float progress = qBound(0.0f, speed / maxSpeed, 1.0f);
    
    if (progress > 0) {
        QLinearGradient grad(center.x() - r, center.y() + r, center.x() + r, center.y() - r);
        grad.setColorAt(0.0, QColor("#3BAEEB"));
        grad.setColorAt(1.0, QColor("#93EDAA"));
        drawSvgArc(painter, center, r, startAngle, progress * totalSpan, 24 * scale, Qt::white, &grad);
    }
    
    // Text
    drawSvgText(painter, center + QPointF(0, 30 * scale), QString::number(qRound(speed)), 110 * scale, Qt::white, Qt::AlignHCenter, true);
    drawSvgText(painter, center + QPointF(0, 80 * scale), "KM/H", 22 * scale, Qt::white, Qt::AlignHCenter, true);

    // Timestamp (local timezone) for sync verification
    if (record.timestamp > 0) {
        QDateTime dt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(record.timestamp), Qt::LocalTime);
        QString tsStr = dt.toString("yyyy-MM-dd HH:mm:ss");
        drawSvgText(painter, center + QPointF(0, 115 * scale), tsStr, 18 * scale, QColor(200, 200, 200), Qt::AlignHCenter, false);
    }
}
