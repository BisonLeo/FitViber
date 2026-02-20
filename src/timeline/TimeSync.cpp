#include "TimeSync.h"
#include "FitTrack.h"

TimeSync::TimeSync(QObject* parent) : QObject(parent) {}
TimeSync::~TimeSync() = default;

void TimeSync::setFitTimeOffset(double offsetSeconds) {
    if (m_fitTimeOffset != offsetSeconds) {
        m_fitTimeOffset = offsetSeconds;
        emit offsetChanged(m_fitTimeOffset);
    }
}

double TimeSync::videoTimeToFitTime(double videoTime) const {
    return videoTime + m_fitTimeOffset;
}

double TimeSync::fitTimeToVideoTime(double fitTime) const {
    return fitTime - m_fitTimeOffset;
}

FitRecord TimeSync::getRecordAtVideoTime(double videoTime, const FitTrack& track) const {
    double fitTime = videoTimeToFitTime(videoTime);
    return track.getRecordAtTime(fitTime);
}
