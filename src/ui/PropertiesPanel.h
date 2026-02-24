#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QGroupBox>
#include <vector>
#include "OverlayPanel.h"

struct ClipTransform;

struct ClipInfo {
    QString path;
    QString type;       // "Video", "Image", "FIT Data"
    // Video/Image fields
    int width = 0;
    int height = 0;
    double fps = 0.0;
    int totalFrames = 0;
    double totalSeconds = 0.0;
    QString codec;
    // FIT fields
    int totalRecords = 0;
    double totalDistance = 0.0;  // meters
    QString firstTimestamp;
};

class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);
    ~PropertiesPanel();

    void setPanelConfigs(const std::vector<PanelConfig>& configs);

    void setClipTransform(ClipTransform* transform);
    void updateTransformLabels();
    void setClipInfo(const ClipInfo& info);
    void clearClipInfo();

signals:
    void configChanged(int panelIndex, const PanelConfig& config);
    void transformChanged();

private:
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;

    QGroupBox* m_clipInfoGroup = nullptr;
    QLabel* m_clipInfoLabel = nullptr;

    QGroupBox* m_transformGroup = nullptr;
    QLabel* m_scaleLabel = nullptr;
    QLabel* m_rotationLabel = nullptr;
    QLabel* m_panLabel = nullptr;
    ClipTransform* m_transform = nullptr;
};
