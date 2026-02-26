#include "MiniMapPanel.h"
#include <QPainterPath>

MiniMapPanel::MiniMapPanel(QObject* parent) : OverlayPanel(PanelType::MiniMap, parent) {
    m_config.label = defaultLabel();
    m_config.x = 0.02;
    m_config.y = 0.72;
    m_config.width = 0.125;   // ~240px out of 1920
    m_config.height = 0.222;  // ~240px out of 1080
}

void MiniMapPanel::paint(QPainter& painter, const QRect& rect,
                          const FitRecord& record, const FitSession& session) {
    // Optional background
    // paintBackground(painter, rect);

    if (session.records.empty() || !record.hasGps) return;

    // Use a square bounding box centered in rect
    int side = std::min(rect.width(), rect.height());
    QRect mapRect(rect.center().x() - side / 2 + 10, rect.center().y() - side / 2 + 10, side - 20, side - 20);

    // Approximate meters per degree at this latitude
    double metersPerLat = 111320.0;
    double metersPerLon = 111320.0 * std::cos(record.latitude * 3.14159265358979323846 / 180.0);

    // Speed in m/s. Base zoom level (half-width of map in meters)
    // Speed 0: 50m half-width (100m total width)
    // Speed 10 m/s (36 km/h): 150m half-width
    double speedMs = std::max(0.0f, record.speed);
    double targetHalfWidthMeters = 50.0 + (speedMs * 10.0);
    
    // Optional: clamp maximum zoom out to prevent seeing too much of the map at extreme speeds
    targetHalfWidthMeters = std::min(targetHalfWidthMeters, 2000.0);

    // Convert target width in meters to degrees
    double halfLatRange = targetHalfWidthMeters / metersPerLat;
    double halfLonRange = targetHalfWidthMeters / metersPerLon;

    // We want mapRect.width() to represent (2 * halfLonRange) degrees
    double scaleX = mapRect.width() / (2.0 * halfLonRange);
    double scaleY = mapRect.height() / (2.0 * halfLatRange);
    double mapScale = std::min(scaleX, scaleY);

    // Center the map on the current record's position
    auto toPoint = [&](double lat, double lon) -> QPointF {
        double x = mapRect.center().x() + (lon - record.longitude) * mapScale;
        double y = mapRect.center().y() - (lat - record.latitude) * mapScale;
        return QPointF(x, y);
    };

    QPainterPath completedPath;
    QPainterPath remainingPath;

    bool needMoveCompleted = true;
    bool needMoveRemaining = true;
    QPointF lastCompletedPoint;

    for (const auto& r : session.records) {
        if (!r.hasGps) continue;

        // Optimization: skip points outside the visual area (with a small margin)
        bool outOfBounds = std::abs(r.latitude - record.latitude) > halfLatRange * 2.0 ||
                           std::abs(r.longitude - record.longitude) > halfLonRange * 2.0;

        if (outOfBounds) {
            // Next in-bounds point must start a new sub-path
            needMoveCompleted = true;
            needMoveRemaining = true;
            if (r.timestamp <= record.timestamp) {
                lastCompletedPoint = toPoint(r.latitude, r.longitude);
            }
            continue;
        }

        QPointF p = toPoint(r.latitude, r.longitude);

        if (r.timestamp <= record.timestamp) {
            if (needMoveCompleted) {
                completedPath.moveTo(p);
                needMoveCompleted = false;
            } else {
                completedPath.lineTo(p);
            }
            lastCompletedPoint = p;
        } else {
            if (needMoveRemaining) {
                // Connect from last completed point if this is the first remaining segment
                if (!lastCompletedPoint.isNull() && remainingPath.isEmpty()) {
                    remainingPath.moveTo(lastCompletedPoint);
                    remainingPath.lineTo(p);
                } else {
                    remainingPath.moveTo(p);
                }
                needMoveRemaining = false;
            } else {
                remainingPath.lineTo(p);
            }
        }
    }

    // Draw remaining path
    painter.setPen(QPen(QColor(255, 255, 255, 80), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(remainingPath);

    // Draw completed path
    painter.setPen(QPen(QColor("#3BAEEB"), 7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(completedPath);

    // Draw current position
    QPointF pos = toPoint(record.latitude, record.longitude);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255));
    painter.drawEllipse(pos, 8, 8);
    painter.setBrush(QColor("#3BAEEB"));
    painter.drawEllipse(pos, 5, 5);
}
