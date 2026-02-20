#include <cassert>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include "fit/FitData.h"
#include "fit/FitTrack.h"
#include "fit/FitParser.h"

void test_fit_record_defaults() {
    FitRecord r;
    assert(r.timestamp == 0.0);
    assert(r.speed == 0.0f);
    assert(!r.hasGps);
    assert(!r.hasHeartRate);
    printf("PASS: test_fit_record_defaults\n");
}

void test_fit_session_bounds() {
    FitSession s;
    FitRecord r1;
    r1.latitude = 47.0; r1.longitude = 8.0; r1.hasGps = true;
    FitRecord r2;
    r2.latitude = 48.0; r2.longitude = 9.0; r2.hasGps = true;
    s.records.push_back(r1);
    s.records.push_back(r2);
    s.updateBounds();

    assert(s.minLat == 47.0);
    assert(s.maxLat == 48.0);
    assert(s.minLon == 8.0);
    assert(s.maxLon == 9.0);
    printf("PASS: test_fit_session_bounds\n");
}

void test_fit_track_interpolation() {
    FitSession s;
    FitRecord r1; r1.timestamp = 100.0; r1.speed = 5.0f;
    FitRecord r2; r2.timestamp = 200.0; r2.speed = 10.0f;
    s.records.push_back(r1);
    s.records.push_back(r2);

    FitTrack track;
    track.loadSession(s);

    assert(!track.isEmpty());
    assert(track.duration() == 100.0);

    // Interpolate at midpoint
    FitRecord mid = track.getRecordAtTime(150.0);
    assert(std::abs(mid.speed - 7.5f) < 0.01f);

    // Before start returns first
    FitRecord before = track.getRecordAtTime(50.0);
    assert(std::abs(before.speed - 5.0f) < 0.01f);

    // After end returns last
    FitRecord after = track.getRecordAtTime(300.0);
    assert(std::abs(after.speed - 10.0f) < 0.01f);

    printf("PASS: test_fit_track_interpolation\n");
}

#ifdef HAS_FIT_SDK
void test_parse_real_fit_file() {
    FitParser parser;
    bool ok = parser.parse("../testdata/2026-02-10-14-36-07.fit");

    if (!ok) {
        printf("FAIL: test_parse_real_fit_file - %s\n", parser.errorString().toUtf8().constData());
        assert(false);
    }

    const FitSession& session = parser.session();

    printf("  Records: %zu\n", session.records.size());
    printf("  Laps: %zu\n", session.laps.size());
    printf("  Duration: %.1f s\n", session.totalElapsedTime);
    printf("  Distance: %.1f m\n", session.totalDistance);
    printf("  Avg Speed: %.2f m/s (%.1f km/h)\n", session.avgSpeed, session.avgSpeed * 3.6);
    printf("  Avg HR: %.0f bpm\n", session.avgHeartRate);
    printf("  GPS bounds: lat[%.6f, %.6f] lon[%.6f, %.6f]\n",
           session.minLat, session.maxLat, session.minLon, session.maxLon);

    assert(session.records.size() > 0);
    assert(session.startTime > 0.0);
    assert(session.totalElapsedTime > 0.0f);

    // Verify first record has reasonable data
    const FitRecord& first = session.records.front();
    assert(first.timestamp > 0.0);

    // Check GPS coordinates are in valid range (if present)
    bool anyGps = false;
    for (const auto& r : session.records) {
        if (r.hasGps) {
            anyGps = true;
            assert(r.latitude >= -90.0 && r.latitude <= 90.0);
            assert(r.longitude >= -180.0 && r.longitude <= 180.0);
        }
    }
    printf("  Has GPS: %s\n", anyGps ? "yes" : "no");

    // Check what data fields are present
    int hrCount = 0, cadCount = 0, pwrCount = 0, spdCount = 0;
    for (const auto& r : session.records) {
        if (r.hasHeartRate) hrCount++;
        if (r.hasCadence) cadCount++;
        if (r.hasPower) pwrCount++;
        if (r.speed > 0) spdCount++;
    }
    printf("  Records with speed: %d\n", spdCount);
    printf("  Records with HR: %d\n", hrCount);
    printf("  Records with cadence: %d\n", cadCount);
    printf("  Records with power: %d\n", pwrCount);

    // Print first few records for debugging
    int showCount = std::min(300, static_cast<int>(session.records.size()));
    for (int i = 0; i < showCount; i++) {
        const auto& r = session.records[i];
        printf("  Record[%d]: ts=%.0f spd=%.2f alt=%.1f hr=%.0f lat=%.6f lon=%.6f\n",
               i, r.timestamp, r.speed, r.altitude, r.heartRate, r.latitude, r.longitude);
    }

    // Test FitTrack with real data
    FitTrack track;
    track.loadSession(session);
    assert(!track.isEmpty());

    double midTime = track.startTime() + track.duration() / 2.0;
    FitRecord midRecord = track.getRecordAtTime(midTime);
    assert(midRecord.timestamp > 0.0);

    printf("PASS: test_parse_real_fit_file\n");
}
#endif

int main() {
    test_fit_record_defaults();
    test_fit_session_bounds();
    test_fit_track_interpolation();
#ifdef HAS_FIT_SDK
    test_parse_real_fit_file();
#else
    printf("SKIP: test_parse_real_fit_file (no FIT SDK)\n");
#endif
    printf("All FIT parser tests passed.\n");
    return 0;
}
