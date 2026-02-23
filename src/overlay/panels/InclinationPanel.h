#pragma once
#include "OverlayPanel.h"

class InclinationPanel : public OverlayPanel {
    Q_OBJECT
public:
    explicit InclinationPanel(QObject* parent = nullptr);
    void paint(QPainter& painter, const QRect& panelRect,
               const FitRecord& record, const FitSession& session) override;
    QString defaultLabel() const override { return "Inclination"; }
};
