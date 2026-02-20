#pragma once

#include <cstdint>
#include <QString>
#include "AppConstants.h"

namespace TimeUtil {

inline double fitTimestampToUnix(uint32_t fitTimestamp) {
    return static_cast<double>(fitTimestamp) + AppConstants::FitEpochOffset;
}

inline uint32_t unixToFitTimestamp(double unixTime) {
    return static_cast<uint32_t>(unixTime - AppConstants::FitEpochOffset);
}

inline QString secondsToHMS(double totalSeconds) {
    int hours = static_cast<int>(totalSeconds) / 3600;
    int minutes = (static_cast<int>(totalSeconds) % 3600) / 60;
    int seconds = static_cast<int>(totalSeconds) % 60;
    int millis = static_cast<int>((totalSeconds - static_cast<int>(totalSeconds)) * 1000);

    if (hours > 0) {
        return QString("%1:%2:%3.%4")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(millis, 3, 10, QChar('0'));
    }
    return QString("%1:%2.%3")
        .arg(minutes)
        .arg(seconds, 2, 10, QChar('0'))
        .arg(millis, 3, 10, QChar('0'));
}

inline QString secondsToHMSms(double totalSeconds) {
    int hours = static_cast<int>(totalSeconds) / 3600;
    int minutes = (static_cast<int>(totalSeconds) % 3600) / 60;
    int seconds = static_cast<int>(totalSeconds) % 60;
    int millis = static_cast<int>((totalSeconds - static_cast<int>(totalSeconds)) * 1000);

    return QString("%1:%2:%3.%4")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(millis, 3, 10, QChar('0'));
}

inline QString secondsToMMSS(double totalSeconds) {
    int minutes = static_cast<int>(totalSeconds) / 60;
    int seconds = static_cast<int>(totalSeconds) % 60;
    return QString("%1:%2")
        .arg(minutes)
        .arg(seconds, 2, 10, QChar('0'));
}

} // namespace TimeUtil
