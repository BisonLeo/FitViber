#pragma once

#include <QObject>
#include "FitData.h"

class FitTrack;

class TimeSync : public QObject {
    Q_OBJECT
public:
    explicit TimeSync(QObject* parent = nullptr);
    ~TimeSync();

    void setFitTimeOffset(double offsetSeconds);
    double fitTimeOffset() const { return m_fitTimeOffset; }

    double videoTimeToFitTime(double videoTime) const;
    double fitTimeToVideoTime(double fitTime) const;

    FitRecord getRecordAtVideoTime(double videoTime, const FitTrack& track) const;

signals:
    void offsetChanged(double offsetSeconds);

private:
    double m_fitTimeOffset = 0.0;  // FIT time = video time + offset
};
