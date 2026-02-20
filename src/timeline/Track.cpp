#include "Track.h"
#include <algorithm>

Track::Track(TrackType type, const QString& name, QObject* parent)
    : QObject(parent), m_type(type), m_name(name) {}

Track::~Track() = default;

void Track::addClip(const Clip& clip) {
    m_clips.push_back(clip);
}

void Track::removeClip(int index) {
    if (index >= 0 && index < static_cast<int>(m_clips.size())) {
        m_clips.erase(m_clips.begin() + index);
    }
}

double Track::duration() const {
    double maxEnd = 0.0;
    for (const auto& c : m_clips) {
        double end = c.timelineOffset + c.duration();
        if (end > maxEnd) maxEnd = end;
    }
    return maxEnd;
}
