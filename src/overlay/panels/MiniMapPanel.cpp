#include "MiniMapPanel.h"
#include <QPainterPath>

MiniMapPanel::MiniMapPanel(QObject* parent) : OverlayPanel(PanelType::MiniMap, parent) {
    m_config.label = defaultLabel();
    m_config.x = 0.80;
    m_config.y = 0.02;
    m_config.width = 0.18;
    m_config.height = 0.25;
}

void MiniMapPanel::paint(QPainter& painter, const QRect& rect,
                          const FitRecord& record, const FitSession& session) {
    paintBackground(painter, rect);

    if (session.records.empty() || !record.hasGps) return;

    double latRange = session.maxLat - session.minLat;
    double lonRange = session.maxLon - session.minLon;
    if (latRange < 1e-9 || lonRange < 1e-9) return;

    // Add padding
    QRect mapRect = rect.adjusted(4, 4, -4, -4);
    double scaleX = mapRect.width() / lonRange;
    double scaleY = mapRect.height() / latRange;
    double scale = std::min(scaleX, scaleY);

    auto toPoint = [&](double lat, double lon) -> QPointF {
        double x = mapRect.left() + (lon - session.minLon) * scale;
        double y = mapRect.bottom() - (lat - session.minLat) * scale;
        return QPointF(x, y);
    };

    // Draw track polyline
    QPainterPath path;
    bool first = true;
    for (const auto& r : session.records) {
        if (!r.hasGps) continue;
        QPointF p = toPoint(r.latitude, r.longitude);
        if (first) { path.moveTo(p); first = false; }
        else path.lineTo(p);
    }

    painter.setPen(QPen(QColor(42, 130, 218), 2));
    painter.drawPath(path);

    // Draw current position
    QPointF pos = toPoint(record.latitude, record.longitude);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 80, 80));
    painter.drawEllipse(pos, 5, 5);
}
