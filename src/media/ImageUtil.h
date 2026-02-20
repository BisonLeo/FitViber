#pragma once

#include <QImage>

namespace ImageUtil {
    // Convert between QImage and FFmpeg AVFrame
    // Actual implementation requires HAS_FFMPEG
    QImage createPlaceholder(int width, int height);
}
