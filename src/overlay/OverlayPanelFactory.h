#pragma once

#include <memory>
#include "OverlayPanel.h"

class OverlayPanelFactory {
public:
    static std::unique_ptr<OverlayPanel> create(PanelType type, QObject* parent = nullptr);
    static std::unique_ptr<OverlayPanel> create(const PanelConfig& config, QObject* parent = nullptr);
};
