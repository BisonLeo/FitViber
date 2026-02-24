#pragma once

#include <QObject>
#include <vector>
#include <memory>
#include "Track.h"

class TimelineModel : public QObject {
    Q_OBJECT
public:
    explicit TimelineModel(QObject* parent = nullptr);
    ~TimelineModel();

    Track* addTrack(TrackType type, const QString& name);
    Track* insertTrack(int index, TrackType type, const QString& name);
    void removeTrack(int index);
    int trackCount() const { return static_cast<int>(m_tracks.size()); }
    Track* track(int index);

    double duration() const;   // max end time across all clips
    double minTime() const;    // min start time across all clips
    double playheadPosition() const { return m_playhead; }
    void setPlayheadPosition(double seconds);

    double zoom() const { return m_zoom; }
    void setZoom(double zoom);
    double scrollOffset() const { return m_scrollOffset; }
    void setScrollOffset(double offset);

    double timeOrigin() const { return m_timeOrigin; }
    void setTimeOrigin(double unixTimestamp);
    double absoluteToRelative(double absTime) const { return absTime - m_timeOrigin; }
    double relativeToAbsolute(double relTime) const { return relTime + m_timeOrigin; }

signals:
    void playheadChanged(double seconds);
    void trackAdded(int index);
    void trackRemoved(int index);
    void zoomChanged(double zoom);
    void layoutChanged();

private:
    std::vector<std::unique_ptr<Track>> m_tracks;
    double m_playhead = 0.0;
    double m_zoom = 1.0;         // pixels per second
    double m_scrollOffset = 0.0; // horizontal scroll in seconds
    double m_timeOrigin = 0.0;   // Unix timestamp of timeline zero
};
