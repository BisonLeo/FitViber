#include "PropertiesPanel.h"
#include <QLabel>

PropertiesPanel::PropertiesPanel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);

    m_contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setAlignment(Qt::AlignTop);

    auto* placeholder = new QLabel("Load a FIT file to configure overlays", m_contentWidget);
    placeholder->setWordWrap(true);
    placeholder->setAlignment(Qt::AlignCenter);
    m_contentLayout->addWidget(placeholder);

    m_scrollArea->setWidget(m_contentWidget);
    layout->addWidget(m_scrollArea);
}

PropertiesPanel::~PropertiesPanel() = default;

void PropertiesPanel::setPanelConfigs(const std::vector<PanelConfig>& configs) {
    // TODO: Phase 9 - build UI for each panel config
    Q_UNUSED(configs);
}
