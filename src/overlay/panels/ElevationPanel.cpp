#include "ElevationPanel.h"
#include <QPainterPath>

ElevationPanel::ElevationPanel(QObject* parent) : OverlayPanel(PanelType::Elevation, parent) {
    m_config.label = defaultLabel();
    m_config.x = 0.68;
    m_config.y = 0.10;
    m_config.width = 0.28;
    m_config.height = 0.24;
}

void ElevationPanel::paint(QPainter& painter, const QRect& rect,
                           const FitRecord& record, const FitSession& session) {
    // Split rect into text region (top ~35%) and graph region (bottom ~65%)
    int textHeight = static_cast<int>(rect.height() * 0.35);
    QRect textRect(rect.left(), rect.top(), rect.width(), textHeight);
    QRect graphRect(rect.left(), rect.top() + textHeight, rect.width(), rect.height() - textHeight);

    // --- Text + Bar section (right-aligned, matching inclination panel) ---
    double scale = textRect.height() / 100.0;
    QPointF base = textRect.topRight() + QPointF(-10 * scale, 64 * scale);

    QString eleStr = QString::number(static_cast<int>(record.altitude));

    // Draw Text (right-aligned from base)
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

    // Draw Bar Graph Background (next to text)
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

    // --- Elevation Profile Graph section (uses graphRect, full width, pushed down a bit) ---
    if (session.records.empty()) return;

    QRectF gRect(graphRect.left() + 10, graphRect.top() + 15,
                 graphRect.width() - 20, graphRect.height() - 20);

    if (gRect.width() <= 0 || gRect.height() <= 0) return;

    float maxDist = session.totalDistance > 0 ? session.totalDistance : 0.01f;
    if (session.records.back().distance > maxDist) maxDist = session.records.back().distance;
    if (maxDist <= 0) maxDist = 0.01f;

    QPainterPath fillPath;
    QPainterPath topPath;
    QPainterPath remainingPath;

    fillPath.moveTo(gRect.left(), gRect.bottom());

    bool topStarted = false;
    bool remStarted = false;
    QPointF lastPoint(gRect.left(), gRect.bottom());

    for (const auto& r : session.records) {
        double px = gRect.left() + gRect.width() * (r.distance / maxDist);
        double py = gRect.bottom() - gRect.height() * ((r.altitude - minAlt) / (maxAlt - minAlt));
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
        fillPath.lineTo(lastPoint.x(), gRect.bottom());
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
