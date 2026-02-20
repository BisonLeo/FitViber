#pragma once
#include "OverlayPanel.h"

class SpeedPanel : public OverlayPanel {
    Q_OBJECT
public:
    explicit SpeedPanel(QObject* parent = nullptr);
    void paint(QPainter& painter, const QRect& rect, const FitRecord& record, const FitSession& session) override;
    QString defaultLabel() const override { return "SPEED"; }
};
