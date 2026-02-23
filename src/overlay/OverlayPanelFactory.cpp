#include "OverlayPanelFactory.h"
#include "SpeedPanel.h"
#include "HeartRatePanel.h"
#include "CadencePanel.h"
#include "PowerPanel.h"
#include "ElevationPanel.h"
#include "MiniMapPanel.h"
#include "DistancePanel.h"
#include "LapPanel.h"
#include "InclinationPanel.h"

std::unique_ptr<OverlayPanel> OverlayPanelFactory::create(PanelType type, QObject* parent) {
    switch (type) {
        case PanelType::Speed:       return std::make_unique<SpeedPanel>(parent);
        case PanelType::HeartRate:   return std::make_unique<HeartRatePanel>(parent);
        case PanelType::Cadence:     return std::make_unique<CadencePanel>(parent);
        case PanelType::Power:       return std::make_unique<PowerPanel>(parent);
        case PanelType::Elevation:   return std::make_unique<ElevationPanel>(parent);
        case PanelType::MiniMap:     return std::make_unique<MiniMapPanel>(parent);
        case PanelType::Distance:    return std::make_unique<DistancePanel>(parent);
        case PanelType::Lap:         return std::make_unique<LapPanel>(parent);
        case PanelType::Inclination: return std::make_unique<InclinationPanel>(parent);
    }
    return nullptr;
}

std::unique_ptr<OverlayPanel> OverlayPanelFactory::create(const PanelConfig& config, QObject* parent) {
    auto panel = create(config.type, parent);
    if (panel) {
        panel->setConfig(config);
    }
    return panel;
}
