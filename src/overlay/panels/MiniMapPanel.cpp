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

    double latRange = session.maxLat - session.minLat;
    double lonRange = session.maxLon - session.minLon;
    if (latRange < 1e-9 || lonRange < 1e-9) return;

    // Use a square bounding box centered in rect
    int side = std::min(rect.width(), rect.height());
    QRect mapRect(rect.center().x() - side / 2 + 10, rect.center().y() - side / 2 + 10, side - 20, side - 20);

    double scaleX = mapRect.width() / lonRange;
    double scaleY = mapRect.height() / latRange;
    double mapScale = std::min(scaleX, scaleY);

    double xOffset = mapRect.center().x() - (lonRange / 2.0) * mapScale;
    double yOffset = mapRect.center().y() + (latRange / 2.0) * mapScale;

    auto toPoint = [&](double lat, double lon) -> QPointF {
        double x = xOffset + (lon - session.minLon) * mapScale;
        double y = yOffset - (lat - session.minLat) * mapScale;
        return QPointF(x, y);
    };

    QPainterPath completedPath;
    QPainterPath remainingPath;

    bool completedStarted = false;
    bool remainingStarted = false;
    QPointF lastPoint;

    for (const auto& r : session.records) {
        if (!r.hasGps) continue;
        QPointF p = toPoint(r.latitude, r.longitude);

        if (r.timestamp <= record.timestamp) {
            if (!completedStarted) {
                completedPath.moveTo(p);
                completedStarted = true;
            } else {
                completedPath.lineTo(p);
            }
            lastPoint = p;
        } else {
            if (!remainingStarted) {
                if (completedStarted) {
                    remainingPath.moveTo(lastPoint);
                    remainingPath.lineTo(p);
                } else {
                    remainingPath.moveTo(p);
                }
                remainingStarted = true;
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
