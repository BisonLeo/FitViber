#include <cassert>
#include <cstdio>
#include <cmath>
#include <QJsonObject>
#include "overlay/OverlayPanel.h"
#include "overlay/OverlayConfig.h"

void test_panel_config_json_roundtrip() {
    PanelConfig cfg;
    cfg.type = PanelType::Speed;
    cfg.visible = true;
    cfg.x = 0.1;
    cfg.y = 0.85;
    cfg.width = 0.15;
    cfg.height = 0.08;
    cfg.opacity = 0.9;
    cfg.label = "SPEED";

    QJsonObject json = OverlayConfig::panelToJson(cfg);
    PanelConfig restored = OverlayConfig::panelFromJson(json);

    assert(restored.type == PanelType::Speed);
    assert(restored.visible == true);
    assert(std::abs(restored.x - 0.1) < 1e-6);
    assert(std::abs(restored.y - 0.85) < 1e-6);
    assert(std::abs(restored.opacity - 0.9) < 1e-6);
    assert(restored.label == "SPEED");
    printf("PASS: test_panel_config_json_roundtrip\n");
}

int main() {
    test_panel_config_json_roundtrip();
    printf("All overlay config tests passed.\n");
    return 0;
}
