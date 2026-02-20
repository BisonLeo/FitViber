#include "LapPanel.h"

LapPanel::LapPanel(QObject* parent) : OverlayPanel(PanelType::Lap, parent) {
    m_config.x = 0.02;
    m_config.y = 0.02;
}

void LapPanel::paint(QPainter& painter, const QRect& rect,
                      const FitRecord& record, const FitSession& session) {
    // Find current lap
    int lapIdx = -1;
    for (int i = 0; i < static_cast<int>(session.laps.size()); ++i) {
        if (record.timestamp >= session.laps[i].startTime &&
            record.timestamp <= session.laps[i].endTime) {
            lapIdx = i;
            break;
        }
    }

    paintBackground(painter, rect);
    paintLabel(painter, rect, m_config.label);

    if (lapIdx >= 0) {
        paintValue(painter, rect, QString::number(lapIdx + 1), "");
    } else {
        paintValue(painter, rect, "--", "");
    }
}
