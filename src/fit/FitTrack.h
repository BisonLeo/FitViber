#pragma once

#include <QObject>
#include <vector>
#include "FitData.h"

class FitTrack : public QObject {
    Q_OBJECT
public:
    explicit FitTrack(QObject* parent = nullptr);
    ~FitTrack();

    void loadSession(const FitSession& session);
    void clear();

    FitRecord getRecordAtTime(double unixTimestamp) const;
    int findLapAtTime(double unixTimestamp) const;

    double startTime() const;
    double endTime() const;
    double duration() const;
    bool isEmpty() const { return m_records.empty(); }

    const std::vector<FitRecord>& records() const { return m_records; }
    const std::vector<FitLap>& laps() const { return m_laps; }
    const FitSession& session() const { return m_session; }

private:
    FitRecord interpolate(const FitRecord& a, const FitRecord& b, double t) const;

    std::vector<FitRecord> m_records;
    std::vector<FitLap> m_laps;
    FitSession m_session;
};
