#include <cassert>
#include <cstdio>
#include <cmath>
#include "util/TimeUtil.h"
#include "app/AppConstants.h"

void test_fit_epoch_conversion() {
    uint32_t fitTs = 1000000000;
    double unixTs = TimeUtil::fitTimestampToUnix(fitTs);
    uint32_t roundtrip = TimeUtil::unixToFitTimestamp(unixTs);
    assert(roundtrip == fitTs);
    printf("PASS: test_fit_epoch_conversion\n");
}

void test_seconds_to_hms() {
    QString result = TimeUtil::secondsToHMS(3661.5);
    assert(result == "1:01:01.500");
    printf("PASS: test_seconds_to_hms\n");
}

void test_seconds_to_mmss() {
    QString result = TimeUtil::secondsToMMSS(125.0);
    assert(result == "2:05");
    printf("PASS: test_seconds_to_mmss\n");
}

int main() {
    test_fit_epoch_conversion();
    test_seconds_to_hms();
    test_seconds_to_mmss();
    printf("All time sync tests passed.\n");
    return 0;
}
