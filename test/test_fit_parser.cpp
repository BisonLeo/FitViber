#include <cassert>
#include <cstdio>
#include "fit/FitData.h"
#include "fit/FitTrack.h"

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

int main() {
    test_fit_record_defaults();
    test_fit_session_bounds();
    printf("All FIT parser tests passed.\n");
    return 0;
}
