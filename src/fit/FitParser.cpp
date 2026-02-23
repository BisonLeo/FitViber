#include "FitParser.h"
#include "TimeUtil.h"
#include <fstream>

#ifdef HAS_FIT_SDK
#include "fit_decode.hpp"
#include "fit_mesg_broadcaster.hpp"
#include "fit_record_mesg.hpp"
#include "fit_record_mesg_listener.hpp"
#include "fit_session_mesg.hpp"
#include "fit_session_mesg_listener.hpp"
#include "fit_lap_mesg.hpp"
#include "fit_lap_mesg_listener.hpp"
#include "fit_date_time.hpp"

namespace {

constexpr double SEMICIRCLES_TO_DEGREES = 180.0 / 2147483648.0;

class FitListener
    : public fit::RecordMesgListener
    , public fit::SessionMesgListener
    , public fit::LapMesgListener
{
public:
    FitSession session;
    int lapIndex = 0;

    void OnMesg(fit::RecordMesg& mesg) override {
        FitRecord r;

        if (mesg.IsTimestampValid()) {
            fit::DateTime dt(mesg.GetTimestamp());
            r.timestamp = static_cast<double>(dt.GetTimeT());
        }

        if (mesg.IsPositionLatValid() && mesg.IsPositionLongValid()) {
            r.latitude = static_cast<double>(mesg.GetPositionLat()) * SEMICIRCLES_TO_DEGREES;
            r.longitude = static_cast<double>(mesg.GetPositionLong()) * SEMICIRCLES_TO_DEGREES;
            r.hasGps = true;
        }

        if (mesg.IsEnhancedSpeedValid())
            r.speed = mesg.GetEnhancedSpeed();
        else if (mesg.IsSpeedValid())
            r.speed = mesg.GetSpeed();

        if (mesg.IsEnhancedAltitudeValid())
            r.altitude = mesg.GetEnhancedAltitude();
        else if (mesg.IsAltitudeValid())
            r.altitude = mesg.GetAltitude();

        if (mesg.IsHeartRateValid()) {
            r.heartRate = static_cast<float>(mesg.GetHeartRate());
            r.hasHeartRate = true;
        }

        if (mesg.IsCadenceValid()) {
            r.cadence = static_cast<float>(mesg.GetCadence());
            r.hasCadence = true;
        }

        if (mesg.IsPowerValid()) {
            r.power = static_cast<float>(mesg.GetPower());
            r.hasPower = true;
        }

        if (mesg.IsDistanceValid())
            r.distance = mesg.GetDistance();

        if (mesg.IsTemperatureValid())
            r.temperature = static_cast<float>(mesg.GetTemperature());

        if (mesg.IsGradeValid()) {
            r.grade = mesg.GetGrade();
            r.hasGrade = true;
        }

        session.records.push_back(r);
    }

    void OnMesg(fit::SessionMesg& mesg) override {
        if (mesg.IsStartTimeValid()) {
            fit::DateTime dt(mesg.GetStartTime());
            session.startTime = static_cast<double>(dt.GetTimeT());
        }
        if (mesg.IsTimestampValid()) {
            fit::DateTime dt(mesg.GetTimestamp());
            session.endTime = static_cast<double>(dt.GetTimeT());
        }
        if (mesg.IsTotalDistanceValid())
            session.totalDistance = mesg.GetTotalDistance();
        if (mesg.IsTotalElapsedTimeValid())
            session.totalElapsedTime = mesg.GetTotalElapsedTime();
        if (mesg.IsAvgSpeedValid())
            session.avgSpeed = mesg.GetAvgSpeed();
        else if (mesg.IsEnhancedAvgSpeedValid())
            session.avgSpeed = mesg.GetEnhancedAvgSpeed();
        if (mesg.IsMaxSpeedValid())
            session.maxSpeed = mesg.GetMaxSpeed();
        else if (mesg.IsEnhancedMaxSpeedValid())
            session.maxSpeed = mesg.GetEnhancedMaxSpeed();
        if (mesg.IsAvgHeartRateValid())
            session.avgHeartRate = static_cast<float>(mesg.GetAvgHeartRate());
        if (mesg.IsMaxHeartRateValid())
            session.maxHeartRate = static_cast<float>(mesg.GetMaxHeartRate());
        if (mesg.IsAvgCadenceValid())
            session.avgCadence = static_cast<float>(mesg.GetAvgCadence());
        if (mesg.IsAvgPowerValid())
            session.avgPower = static_cast<float>(mesg.GetAvgPower());
        if (mesg.IsTotalAscentValid())
            session.totalAscent = static_cast<float>(mesg.GetTotalAscent());
        if (mesg.IsTotalDescentValid())
            session.totalDescent = static_cast<float>(mesg.GetTotalDescent());
    }

    void OnMesg(fit::LapMesg& mesg) override {
        FitLap lap;
        lap.lapIndex = lapIndex++;

        if (mesg.IsStartTimeValid()) {
            fit::DateTime dt(mesg.GetStartTime());
            lap.startTime = static_cast<double>(dt.GetTimeT());
        }
        if (mesg.IsTimestampValid()) {
            fit::DateTime dt(mesg.GetTimestamp());
            lap.endTime = static_cast<double>(dt.GetTimeT());
        }
        if (mesg.IsTotalDistanceValid())
            lap.totalDistance = mesg.GetTotalDistance();
        if (mesg.IsTotalElapsedTimeValid())
            lap.totalElapsedTime = mesg.GetTotalElapsedTime();
        if (mesg.IsAvgSpeedValid())
            lap.avgSpeed = mesg.GetAvgSpeed();
        if (mesg.IsAvgHeartRateValid())
            lap.avgHeartRate = static_cast<float>(mesg.GetAvgHeartRate());
        if (mesg.IsAvgCadenceValid())
            lap.avgCadence = static_cast<float>(mesg.GetAvgCadence());
        if (mesg.IsAvgPowerValid())
            lap.avgPower = static_cast<float>(mesg.GetAvgPower());

        session.laps.push_back(lap);
    }
};

} // anonymous namespace
#endif // HAS_FIT_SDK

FitParser::FitParser(QObject* parent) : QObject(parent) {}
FitParser::~FitParser() = default;

bool FitParser::parse(const QString& filePath) {
#ifdef HAS_FIT_SDK
    std::fstream file(filePath.toStdString(), std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        m_error = QString("Cannot open file: %1").arg(filePath);
        emit error(m_error);
        return false;
    }

    fit::Decode decode;
    if (!decode.IsFIT(file)) {
        m_error = QString("Not a valid FIT file: %1").arg(filePath);
        emit error(m_error);
        return false;
    }

    file.clear();
    file.seekg(0);

    // Integrity check is optional — file may still decode even if it fails
    decode.CheckIntegrity(file);
    file.clear();
    file.seekg(0);

    fit::MesgBroadcaster broadcaster;
    FitListener listener;

    broadcaster.AddListener(static_cast<fit::RecordMesgListener&>(listener));
    broadcaster.AddListener(static_cast<fit::SessionMesgListener&>(listener));
    broadcaster.AddListener(static_cast<fit::LapMesgListener&>(listener));

    try {
        decode.Read(file, broadcaster, broadcaster);
    } catch (const fit::RuntimeException& e) {
        m_error = QString("FIT decode error: %1").arg(e.what());
        emit error(m_error);
        return false;
    } catch (const std::exception& e) {
        m_error = QString("Error reading FIT file: %1").arg(e.what());
        emit error(m_error);
        return false;
    }

    m_session = std::move(listener.session);
    m_session.updateBounds();

    // Compute session averages from records if session message didn't provide them
    if (!m_session.records.empty()) {
        float totalSpeed = 0, totalHr = 0;
        int speedCount = 0, hrCount = 0;
        for (const auto& r : m_session.records) {
            if (r.speed > 0) { totalSpeed += r.speed; speedCount++; }
            if (r.hasHeartRate && r.heartRate > 0) { totalHr += r.heartRate; hrCount++; }
        }
        if (m_session.avgSpeed == 0 && speedCount > 0)
            m_session.avgSpeed = totalSpeed / speedCount;
        if (m_session.avgHeartRate == 0 && hrCount > 0)
            m_session.avgHeartRate = totalHr / hrCount;
        if (m_session.startTime == 0 && !m_session.records.empty())
            m_session.startTime = m_session.records.front().timestamp;
        if (m_session.endTime == 0 && !m_session.records.empty())
            m_session.endTime = m_session.records.back().timestamp;
        if (m_session.totalElapsedTime == 0)
            m_session.totalElapsedTime = static_cast<float>(m_session.endTime - m_session.startTime);
    }

    emit parsed(m_session);
    return true;
#else
    Q_UNUSED(filePath);
    m_error = "FIT SDK not available - build with HAS_FIT_SDK";
    emit error(m_error);
    return false;
#endif
}
