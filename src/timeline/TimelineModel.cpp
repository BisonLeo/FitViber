#include "TimelineModel.h"
#include <algorithm>

TimelineModel::TimelineModel(QObject* parent) : QObject(parent) {}
TimelineModel::~TimelineModel() = default;

Track* TimelineModel::addTrack(TrackType type, const QString& name) {
    return insertTrack(static_cast<int>(m_tracks.size()), type, name);
}

Track* TimelineModel::insertTrack(int index, TrackType type, const QString& name) {
    if (index < 0 || index > static_cast<int>(m_tracks.size())) {
        index = static_cast<int>(m_tracks.size());
    }
    auto track = std::make_unique<Track>(type, name, this);
    Track* ptr = track.get();
    m_tracks.insert(m_tracks.begin() + index, std::move(track));
    emit trackAdded(index);
    emit layoutChanged();
    return ptr;
}

void TimelineModel::removeTrack(int index) {
    if (index >= 0 && index < static_cast<int>(m_tracks.size())) {
        m_tracks.erase(m_tracks.begin() + index);
        emit trackRemoved(index);
    }
}

Track* TimelineModel::track(int index) {
    if (index >= 0 && index < static_cast<int>(m_tracks.size())) {
        return m_tracks[index].get();
    }
    return nullptr;
}

double TimelineModel::duration() const {
    double maxDur = 0.0;
    for (const auto& t : m_tracks) {
        maxDur = std::max(maxDur, t->duration());
    }
    return maxDur;
}

void TimelineModel::setPlayheadPosition(double seconds) {
    if (m_playhead != seconds) {
        m_playhead = seconds;
        emit playheadChanged(seconds);
    }
}

void TimelineModel::setZoom(double zoom) {
    m_zoom = std::max(0.001, std::min(zoom, 500.0));
    emit zoomChanged(m_zoom);
}

void TimelineModel::setScrollOffset(double offset) {
    m_scrollOffset = std::max(0.0, offset);
}

void TimelineModel::setTimeOrigin(double unixTimestamp) {
    m_timeOrigin = unixTimestamp;
}
