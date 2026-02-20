#pragma once
#include "OverlayPanel.h"

class ElevationPanel : public OverlayPanel {
    Q_OBJECT
public:
    explicit ElevationPanel(QObject* parent = nullptr);
    void paint(QPainter& painter, const QRect& rect, const FitRecord& record, const FitSession& session) override;
    QString defaultLabel() const override { return "ELEVATION"; }
};
