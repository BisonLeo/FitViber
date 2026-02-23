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
    
    calculateInclination();
}

void FitTrack::appendSession(const FitSession& session) {
    m_records.insert(m_records.end(), session.records.begin(), session.records.end());
    m_laps.insert(m_laps.end(), session.laps.begin(), session.laps.end());

    std::sort(m_records.begin(), m_records.end(),
        [](const FitRecord& a, const FitRecord& b) { return a.timestamp < b.timestamp; });
    std::sort(m_laps.begin(), m_laps.end(),
        [](const FitLap& a, const FitLap& b) { return a.startTime < b.startTime; });
        
    calculateInclination();
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
    r.grade = a.grade + static_cast<float>(t) * (b.grade - a.grade);
    r.hasGps = a.hasGps || b.hasGps;
    r.hasHeartRate = a.hasHeartRate || b.hasHeartRate;
    r.hasCadence = a.hasCadence || b.hasCadence;
    r.hasPower = a.hasPower || b.hasPower;
    r.hasGrade = a.hasGrade || b.hasGrade;
    return r;
}

void FitTrack::calculateInclination() {
    if (m_records.size() < 2) return;

    // Check if grade already exists in the data
    bool hasGradeData = false;
    for (const auto& r : m_records) {
        if (r.hasGrade) {
            hasGradeData = true;
            break;
        }
    }
    if (hasGradeData) return;

    double maxDist = m_records.back().distance;
    if (maxDist <= 0.0) return;

    // Step 1: Resample with even delta x (5.0 meters)
    double dx = 5.0;
    int numSamples = static_cast<int>(std::ceil(maxDist / dx)) + 1;
    std::vector<double> evenAlt(numSamples, 0.0);

    size_t recIdx = 0;
    for (int i = 0; i < numSamples; ++i) {
        double d = i * dx;
        
        while (recIdx < m_records.size() - 1 && m_records[recIdx + 1].distance < d) {
            recIdx++;
        }
        
        if (recIdx >= m_records.size() - 1) {
            evenAlt[i] = m_records.back().altitude;
        } else {
            const auto& a = m_records[recIdx];
            const auto& b = m_records[recIdx + 1];
            double distRange = b.distance - a.distance;
            if (distRange < 1e-6) {
                evenAlt[i] = b.altitude;
            } else {
                double t = (d - a.distance) / distRange;
                evenAlt[i] = a.altitude + t * (b.altitude - a.altitude);
            }
        }
    }

    // Step 2: Low-pass filter (moving average)
    int window = 10; // 50 meters (10 * 5m)
    std::vector<double> smoothAlt(numSamples, 0.0);
    for (int i = 0; i < numSamples; ++i) {
        double sum = 0;
        int count = 0;
        for (int j = std::max(0, i - window); j <= std::min(numSamples - 1, i + window); ++j) {
            sum += evenAlt[j];
            count++;
        }
        smoothAlt[i] = sum / count;
    }

    // Step 3: Compute inclination and map back to m_records
    for (auto& r : m_records) {
        double d = r.distance;
        int i = static_cast<int>(d / dx);
        
        double dz = 0.0;
        double currentDx = dx;
        
        if (i > 0 && i < numSamples - 1) {
            dz = smoothAlt[i+1] - smoothAlt[i-1];
            currentDx = 2.0 * dx;
        } else if (i == 0 && numSamples > 1) {
            dz = smoothAlt[1] - smoothAlt[0];
        } else if (i >= numSamples - 1 && numSamples > 1) {
            dz = smoothAlt[numSamples-1] - smoothAlt[numSamples-2];
        }
        
        double angle = std::atan2(dz, currentDx) * 180.0 / 3.14159265358979323846;
        r.grade = static_cast<float>(angle);
    }
}
