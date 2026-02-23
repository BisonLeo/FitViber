#include "HeartRatePanel.h"

HeartRatePanel::HeartRatePanel(QObject* parent) : OverlayPanel(PanelType::HeartRate, parent) {
    m_config.label = defaultLabel();
    m_config.x = 0.03;
    m_config.y = 0.10;
    m_config.width = 0.10;
    m_config.height = 0.10;
}

void HeartRatePanel::paint(QPainter& painter, const QRect& rect,
                            const FitRecord& record, const FitSession&) {
    if (!record.hasHeartRate) return;
    double scale = rect.height() / 100.0;
    QPointF base = rect.topLeft() + QPointF(0, 64 * scale);
    
    QString hrStr = QString::number(static_cast<int>(record.heartRate));
    
    double valW = drawSvgText(painter, base, hrStr, 64 * scale, Qt::white, Qt::AlignLeft, true);
    drawSvgText(painter, base + QPointF(valW + 15 * scale, 0), "Bpm", 36 * scale, Qt::white, Qt::AlignLeft, true);
    drawSvgText(painter, base + QPointF(0, 35 * scale), m_config.label, 22 * scale, QColor(221, 221, 221), Qt::AlignLeft, false);
}
