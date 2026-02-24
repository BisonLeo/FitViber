#pragma once

#include <QString>
#include "ClipTransform.h"

enum class ClipType {
    Video,
    Audio,
    Image,
    FitData
};

struct Clip {
    QString sourcePath;
    QString displayName;
    ClipType type = ClipType::Video;
    double sourceIn = 0.0;    // start time in source (seconds)
    double sourceOut = 0.0;   // end time in source
    double timelineOffset = 0.0;  // position on timeline (relative to time origin)
    double absoluteStartTime = 0.0;  // Unix timestamp of clip start
    bool locked = false;              // locked clips cannot be moved
    ClipTransform transform;

    double duration() const { return sourceOut - sourceIn; }
};
