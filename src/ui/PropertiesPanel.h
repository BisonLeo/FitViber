#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <vector>
#include "OverlayPanel.h"

class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);
    ~PropertiesPanel();

    void setPanelConfigs(const std::vector<PanelConfig>& configs);

signals:
    void configChanged(int panelIndex, const PanelConfig& config);

private:
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
};
