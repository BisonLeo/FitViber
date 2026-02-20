#pragma once
#include "OverlayPanel.h"

class PowerPanel : public OverlayPanel {
    Q_OBJECT
public:
    explicit PowerPanel(QObject* parent = nullptr);
    void paint(QPainter& painter, const QRect& rect, const FitRecord& record, const FitSession& session) override;
    QString defaultLabel() const override { return "POWER"; }
};
