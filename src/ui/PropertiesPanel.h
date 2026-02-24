#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QGroupBox>
#include <vector>
#include "OverlayPanel.h"

struct ClipTransform;

class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);
    ~PropertiesPanel();

    void setPanelConfigs(const std::vector<PanelConfig>& configs);

    void setClipTransform(ClipTransform* transform);
    void updateTransformLabels();

signals:
    void configChanged(int panelIndex, const PanelConfig& config);
    void transformChanged();

private:
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;

    QGroupBox* m_transformGroup = nullptr;
    QLabel* m_scaleLabel = nullptr;
    QLabel* m_rotationLabel = nullptr;
    QLabel* m_panLabel = nullptr;
    ClipTransform* m_transform = nullptr;
};
