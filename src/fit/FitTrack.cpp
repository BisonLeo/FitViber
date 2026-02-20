#include "FitTrack.h"
#include <algorithm>
#include <cmath>

FitTrack::FitTrack(QObject* parent) : QObject(parent) {}
FitTrack::~FitTrack() = default;

void FitTrack::loadSession(const FitSession& session) {
    m_session = session;
    m_records = session.records;
    m_laps = session.laps;

    std::sort(m_records.begin(), m_records.end(),
        [](const FitRecord& a, const FitRecord& b) { return a.timestamp < b.timestamp; });
}

void FitTrack::clear() {
    m_records.clear();
    m_laps.clear();
    m_session = FitSession{};
}

double FitTrack::startTime() const {
    return m_records.empty() ? 0.0 : m_records.front().timestamp;
}

double FitTrack::endTime() const {
    return m_records.empty() ? 0.0 : m_records.back().timestamp;
}

double FitTrack::duration() const {
    return endTime() - startTime();
}

FitRecord FitTrack::getRecordAtTime(double unixTimestamp) const {
    if (m_records.empty()) return FitRecord{};
    if (unixTimestamp <= m_records.front().timestamp) return m_records.front();
    if (unixTimestamp >= m_records.back().timestamp) return m_records.back();

    // Binary search for the interval
    auto it = std::lower_bound(m_records.begin(), m_records.end(), unixTimestamp,
        [](const FitRecord& r, double t) { return r.timestamp < t; });

    if (it == m_records.begin()) return *it;
    if (it == m_records.end()) return m_records.back();

    const auto& b = *it;
    const auto& a = *(it - 1);

    double range = b.timestamp - a.timestamp;
    if (range < 1e-9) return a;

    double t = (unixTimestamp - a.timestamp) / range;
    return interpolate(a, b, t);
}

int FitTrack::findLapAtTime(double unixTimestamp) const {
    for (int i = 0; i < static_cast<int>(m_laps.size()); ++i) {
        if (unixTimestamp >= m_laps[i].startTime && unixTimestamp <= m_laps[i].endTime) {
            return i;
        }
    }
    return -1;
}

FitRecord FitTrack::interpolate(const FitRecord& a, const FitRecord& b, double t) const {
    FitRecord r;
    r.timestamp = a.timestamp + t * (b.timestamp - a.timestamp);
    r.latitude = a.latitude + t * (b.latitude - a.latitude);
    r.longitude = a.longitude + t * (b.longitude - a.longitude);
    r.altitude = a.altitude + static_cast<float>(t) * (b.altitude - a.altitude);
    r.speed = a.speed + static_cast<float>(t) * (b.speed - a.speed);
    r.heartRate = a.heartRate + static_cast<float>(t) * (b.heartRate - a.heartRate);
    r.cadence = a.cadence + static_cast<float>(t) * (b.cadence - a.cadence);
    r.power = a.power + static_cast<float>(t) * (b.power - a.power);
    r.distance = a.distance + static_cast<float>(t) * (b.distance - a.distance);
    r.temperature = a.temperature + static_cast<float>(t) * (b.temperature - a.temperature);
    r.hasGps = a.hasGps || b.hasGps;
    r.hasHeartRate = a.hasHeartRate || b.hasHeartRate;
    r.hasCadence = a.hasCadence || b.hasCadence;
    r.hasPower = a.hasPower || b.hasPower;
    return r;
}
