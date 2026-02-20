#pragma once

#include <QString>

enum class ClipType {
    Video,
    Audio,
    FitData
};

struct Clip {
    QString sourcePath;
    ClipType type = ClipType::Video;
    double sourceIn = 0.0;    // start time in source (seconds)
    double sourceOut = 0.0;   // end time in source
    double timelineOffset = 0.0;  // position on timeline

    double duration() const { return sourceOut - sourceIn; }
};
