#pragma once
#include "OverlayPanel.h"

class HeartRatePanel : public OverlayPanel {
    Q_OBJECT
public:
    explicit HeartRatePanel(QObject* parent = nullptr);
    void paint(QPainter& painter, const QRect& rect, const FitRecord& record, const FitSession& session) override;
    QString defaultLabel() const override { return "HR"; }
};
