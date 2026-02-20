#pragma once

#include <cstdint>
#include <QString>
#include <QDateTime>
#include <QTimeZone>
#include <QFileInfo>
#include <QImage>
#include <QRegularExpression>
#include "AppConstants.h"
#include "MediaProbe.h"

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

// Parse DJI filename pattern: DJI_YYYYMMDDHHmmss_*.MP4 (local time, assumed UTC+8)
inline double parseFilenameTimestamp(const QString& filename, int utcOffsetHours = 8) {
    static QRegularExpression re(R"(DJI_(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2}))");
    auto match = re.match(filename);
    if (match.hasMatch()) {
        QDateTime dt(
            QDate(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt()),
            QTime(match.captured(4).toInt(), match.captured(5).toInt(), match.captured(6).toInt()),
            QTimeZone(utcOffsetHours * 3600));
        if (dt.isValid()) {
            return static_cast<double>(dt.toSecsSinceEpoch());
        }
    }
    return 0.0;
}

// Extract creation timestamp from a media file, trying multiple sources
inline double extractMediaTimestamp(const QString& filePath) {
    // 1. Try FFmpeg creation_time via MediaProbe (for video files)
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    QStringList videoExts = {"mp4", "avi", "mkv", "mov", "wmv"};
    if (videoExts.contains(suffix)) {
        MediaProbe probe;
        if (probe.probe(filePath) && probe.info().creationTimestamp > 0) {
            return probe.info().creationTimestamp;
        }
    }

    // 2. Try DJI filename pattern
    double fnTs = parseFilenameTimestamp(fi.fileName());
    if (fnTs > 0) return fnTs;

    // 3. Try JPEG EXIF DateTimeOriginal
    QStringList imageExts = {"jpg", "jpeg", "png", "bmp", "tiff", "tif"};
    if (imageExts.contains(suffix)) {
        QImage img(filePath);
        QString exifDate = img.text("DateTimeOriginal");
        if (exifDate.isEmpty()) exifDate = img.text("DateTime");
        if (!exifDate.isEmpty()) {
            // EXIF format: "YYYY:MM:DD HH:MM:SS"
            QDateTime dt = QDateTime::fromString(exifDate, "yyyy:MM:dd HH:mm:ss");
            if (dt.isValid()) {
                dt.setTimeZone(QTimeZone(8 * 3600));
                return static_cast<double>(dt.toSecsSinceEpoch());
            }
        }
    }

    // 4. Fall back to file creation/birth time
    QDateTime birthTime = fi.birthTime();
    if (birthTime.isValid()) {
        return static_cast<double>(birthTime.toSecsSinceEpoch());
    }

    // Last resort: last modified time
    return static_cast<double>(fi.lastModified().toSecsSinceEpoch());
}

// Convert Unix timestamp to local time string for ruler display
inline QString unixToLocalTimeStr(double unixTime, int utcOffsetHours = 8) {
    QDateTime dt = QDateTime::fromSecsSinceEpoch(
        static_cast<qint64>(unixTime), QTimeZone(utcOffsetHours * 3600));
    return dt.toString("HH:mm:ss");
}

} // namespace TimeUtil
