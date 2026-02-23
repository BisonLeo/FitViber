#pragma once

#include <cstdint>
#include <vector>
#include <QString>

struct FitRecord {
    double timestamp = 0.0;       // Unix timestamp (seconds)
    double latitude = 0.0;        // degrees
    double longitude = 0.0;       // degrees
    float altitude = 0.0f;        // meters
    float speed = 0.0f;           // m/s
    float heartRate = 0.0f;       // bpm
    float cadence = 0.0f;         // rpm
    float power = 0.0f;           // watts
    float distance = 0.0f;        // meters (cumulative)
    float temperature = 0.0f;     // celsius
    float grade = 0.0f;           // inclination / slope degree
    bool hasGps = false;
    bool hasHeartRate = false;
    bool hasCadence = false;
    bool hasPower = false;
    bool hasGrade = false;
};

struct FitLap {
    double startTime = 0.0;
    double endTime = 0.0;
    float totalDistance = 0.0f;
    float totalElapsedTime = 0.0f;
    float avgSpeed = 0.0f;
    float avgHeartRate = 0.0f;
    float avgCadence = 0.0f;
    float avgPower = 0.0f;
    int lapIndex = 0;
};

struct FitSession {
    QString sport;
    double startTime = 0.0;
    double endTime = 0.0;
    float totalDistance = 0.0f;
    float totalElapsedTime = 0.0f;
    float avgSpeed = 0.0f;
    float maxSpeed = 0.0f;
    float avgHeartRate = 0.0f;
    float maxHeartRate = 0.0f;
    float avgCadence = 0.0f;
    float avgPower = 0.0f;
    float totalAscent = 0.0f;
    float totalDescent = 0.0f;

    std::vector<FitRecord> records;
    std::vector<FitLap> laps;

    // GPS bounding box
    double minLat = 90.0, maxLat = -90.0;
    double minLon = 180.0, maxLon = -180.0;

    void updateBounds() {
        for (const auto& r : records) {
            if (!r.hasGps) continue;
            if (r.latitude < minLat) minLat = r.latitude;
            if (r.latitude > maxLat) maxLat = r.latitude;
            if (r.longitude < minLon) minLon = r.longitude;
            if (r.longitude > maxLon) maxLon = r.longitude;
        }
    }
};
