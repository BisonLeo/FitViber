#pragma once

struct ClipTransform {
    double scale = 1.0;
    double rotation = 0.0;    // degrees, clockwise
    double panX = 0.0;        // pixel offset in output space
    double panY = 0.0;
    bool flipH = false;
    bool flipV = false;

    bool isIdentity() const {
        return scale == 1.0 && rotation == 0.0 &&
               panX == 0.0 && panY == 0.0 &&
               !flipH && !flipV;
    }

    void reset() {
        scale = 1.0;
        rotation = 0.0;
        panX = 0.0;
        panY = 0.0;
        flipH = false;
        flipV = false;
    }
};
