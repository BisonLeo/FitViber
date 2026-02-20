#pragma once

#include <QObject>
#include <QImage>
#include <vector>
#include <memory>
#include "OverlayPanel.h"
#include "FitData.h"

class OverlayRenderer : public QObject {
    Q_OBJECT
public:
    explicit OverlayRenderer(QObject* parent = nullptr);
    ~OverlayRenderer();

    void render(QImage& frame, const FitRecord& record, const FitSession& session);

    void addPanel(std::unique_ptr<OverlayPanel> panel);
    void removePanel(int index);
    void clearPanels();

    int panelCount() const { return static_cast<int>(m_panels.size()); }
    OverlayPanel* panel(int index);
    const std::vector<std::unique_ptr<OverlayPanel>>& panels() const { return m_panels; }

private:
    std::vector<std::unique_ptr<OverlayPanel>> m_panels;
};
