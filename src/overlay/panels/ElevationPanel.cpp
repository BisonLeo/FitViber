#include "ElevationPanel.h"
#include <QPainterPath>

ElevationPanel::ElevationPanel(QObject* parent) : OverlayPanel(PanelType::Elevation, parent) {
    m_config.label = defaultLabel();
    m_config.x = 0.68;
    m_config.y = 0.10;
    m_config.width = 0.28;
    m_config.height = 0.10;
}

void ElevationPanel::paint(QPainter& painter, const QRect& rect,
                           const FitRecord& record, const FitSession& session) {
    double scale = rect.height() / 100.0;
    // Align text/bar to the left side of this wider rect
    QPointF base = rect.topLeft() + QPointF(110 * scale, 64 * scale);
    
    QString eleStr = QString::number(static_cast<int>(record.altitude));

    // Draw Text
    double unitW = drawSvgText(painter, base + QPointF(-25 * scale, 0), "M", 36 * scale, Qt::white, Qt::AlignRight, true);
    drawSvgText(painter, base + QPointF(-25 * scale - unitW - 10 * scale, 0), eleStr, 64 * scale, Qt::white, Qt::AlignRight, true);
    drawSvgText(painter, base + QPointF(-25 * scale, 35 * scale), m_config.label, 22 * scale, QColor(221, 221, 221), Qt::AlignRight, false);  

    // Determine bounds
    float minAlt = -100;
    float maxAlt = 10000;
    if (session.records.size() > 1) {
        minAlt = 99999;
        maxAlt = -99999;
        for(const auto& r : session.records) {
            if(r.altitude < minAlt) minAlt = r.altitude;
            if(r.altitude > maxAlt) maxAlt = r.altitude;
        }
        if (maxAlt - minAlt < 10) maxAlt = minAlt + 10;
    }

    // Draw Bar Graph Background
    QRectF bgBar(base.x() - 8 * scale, base.y() - 45 * scale, 8 * scale, 90 * scale);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 76)); // 0.3 alpha
    painter.drawRoundedRect(bgBar, 4 * scale, 4 * scale);

    // Draw Bar Graph Foreground
    float progress = qBound(0.0f, (record.altitude - minAlt) / (maxAlt - minAlt), 1.0f);
    float fillHeight = progress * 90 * scale;
    if (progress > 0 && fillHeight < 4 * scale) fillHeight = 4 * scale;

    float yStart = base.y() + 45 * scale - fillHeight;

    if (progress > 0) {
        QRectF fgBar(base.x() - 8 * scale, yStart, 8 * scale, fillHeight);
        painter.setBrush(QColor("#88E6AE"));
        painter.drawRoundedRect(fgBar, 4 * scale, 4 * scale);
    }

    // Sketch Graph on the right
    if (session.records.empty()) return;

    QRectF graphRect(rect.left() + 130 * scale, rect.top() + 10 * scale, 
                     rect.width() - 140 * scale, rect.height() - 20 * scale);
    
    if (graphRect.width() <= 0) return;

    float maxDist = session.totalDistance > 0 ? session.totalDistance : 0.01f;
    if (session.records.back().distance > maxDist) maxDist = session.records.back().distance;
    if (maxDist <= 0) maxDist = 0.01f;

    QPainterPath fillPath;
    QPainterPath topPath;
    QPainterPath remainingPath;
    
    fillPath.moveTo(graphRect.left(), graphRect.bottom());
    
    bool topStarted = false;
    bool remStarted = false;
    QPointF lastPoint(graphRect.left(), graphRect.bottom());

    for (const auto& r : session.records) {
        double px = graphRect.left() + graphRect.width() * (r.distance / maxDist);
        double py = graphRect.bottom() - graphRect.height() * ((r.altitude - minAlt) / (maxAlt - minAlt));
        QPointF pt(px, py);
        
        if (r.timestamp <= record.timestamp) {
            fillPath.lineTo(pt);
            if (!topStarted) {
                topPath.moveTo(pt);
                topStarted = true;
            } else {
                topPath.lineTo(pt);
            }
            lastPoint = pt;
        } else {
            if (!remStarted) {
                if (topStarted) {
                    remainingPath.moveTo(lastPoint);
                    remainingPath.lineTo(pt);
                } else {
                    remainingPath.moveTo(pt);
                }
                remStarted = true;
            } else {
                remainingPath.lineTo(pt);
            }
        }
    }

    if (topStarted) {
        fillPath.lineTo(lastPoint.x(), graphRect.bottom());
        fillPath.closeSubpath();
        
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(136, 230, 174, 100)); // #88E6AE with opacity
        painter.drawPath(fillPath);

        painter.setPen(QPen(QColor("#88E6AE"), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(topPath);
    }

    painter.setPen(QPen(QColor(255, 255, 255, 100), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(remainingPath);
}
