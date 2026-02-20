#pragma once

#include <QString>

namespace AppConstants {
    inline constexpr const char* AppName = "FitViber";
    inline constexpr const char* AppVersion = "0.1.0";
    inline constexpr const char* OrgName = "FitViber";

    inline constexpr int DefaultWindowWidth = 1400;
    inline constexpr int DefaultWindowHeight = 900;

    // FIT epoch: 1989-12-31 00:00:00 UTC (631065600 seconds after Unix epoch)
    inline constexpr uint32_t FitEpochOffset = 631065600;

    // Default video FPS for playback timer
    inline constexpr double DefaultFps = 30.0;
}
