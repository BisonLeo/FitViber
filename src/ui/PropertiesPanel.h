#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QDateTime>
#include <QTimeZone>
#include <QRegularExpression>
#include <vector>
#include <cmath>
#include "OverlayPanel.h"

struct ClipTransform;

struct ClipInfo {
    QString path;
    QString type;       // "Video", "Image", "FIT Data"
    // Video/Image fields
    int width = 0;
    int height = 0;
    double fps = 0.0;
    int totalFrames = 0;
    double totalSeconds = 0.0;
    QString codec;
    // FIT fields
    int totalRecords = 0;
    double totalDistance = 0.0;  // meters
    QString firstTimestamp;
    // Detected timestamp (for all clip types)
    double detectedStartTimestamp = 0.0;  // Unix timestamp
    double detectedEndTimestamp = 0.0;    // Unix timestamp
};

struct ClipPlacement {
    double startPos = 0.0;   // timeline offset (seconds)
    double duration = 0.0;   // clip duration (seconds)
    double endPos = 0.0;     // startPos + duration
    double timeOrigin = 0.0; // timeline origin (Unix timestamp)
};

// QDoubleSpinBox that can display as either raw seconds or HH:MM:SS.sss.
// When timeOrigin is set (> 0) and HMS mode is active, start/end spinboxes
// show absolute clock time matching the timeline axis labels.
class TimeSpinBox : public QDoubleSpinBox {
    Q_OBJECT
public:
    explicit TimeSpinBox(QWidget* parent = nullptr) : QDoubleSpinBox(parent) {
        setRange(-1e9, 1e9);
        setDecimals(3);
        setSingleStep(0.1);
    }

    void setTimeMode(bool hms) {
        if (m_hmsMode == hms) return;
        m_hmsMode = hms;
        setValue(value()); // force redisplay
    }

    // Set the timeline origin for absolute clock time display.
    // When > 0 and HMS mode active, value is shown as timeOrigin + value
    // converted to local HH:MM:SS.sss.
    void setTimeOrigin(double origin) {
        m_timeOrigin = origin;
        if (m_hmsMode) setValue(value()); // force redisplay
    }

    bool isTimeMode() const { return m_hmsMode; }

    QString textFromValue(double val) const override {
        if (!m_hmsMode)
            return QString::number(val, 'f', 3) + " s";

        if (m_timeOrigin > 0) {
            // Absolute clock time matching timeline axis
            double absTime = m_timeOrigin + val;
            QDateTime dt = QDateTime::fromMSecsSinceEpoch(
                static_cast<qint64>(absTime * 1000.0), QTimeZone(8 * 3600));
            return dt.toString("HH:mm:ss.zzz");
        }
        // No time origin: show relative HH:MM:SS.sss
        return relativeHMS(val);
    }

    double valueFromText(const QString& text) const override {
        if (!m_hmsMode) {
            QString s = text;
            s.remove(QRegularExpression("\\s*s$"));
            return s.toDouble();
        }

        if (m_timeOrigin > 0) {
            // Parse absolute clock time back to relative offset
            double currentAbs = m_timeOrigin + value();
            QDateTime currentDt = QDateTime::fromSecsSinceEpoch(
                static_cast<qint64>(currentAbs), QTimeZone(8 * 3600));
            QTime parsed = QTime::fromString(text, "HH:mm:ss.zzz");
            if (!parsed.isValid()) parsed = QTime::fromString(text, "HH:mm:ss");
            if (!parsed.isValid()) parsed = QTime::fromString(text, "H:mm:ss");
            if (!parsed.isValid()) return value(); // keep current on bad input
            QDateTime newDt(currentDt.date(), parsed, QTimeZone(8 * 3600));
            return static_cast<double>(newDt.toMSecsSinceEpoch()) / 1000.0 - m_timeOrigin;
        }
        return parseRelativeHMS(text);
    }

    QValidator::State validate(QString& input, int& pos) const override {
        Q_UNUSED(pos);
        if (!m_hmsMode) {
            QString s = input;
            s.remove(QRegularExpression("\\s*s$"));
            bool ok;
            s.toDouble(&ok);
            return ok ? QValidator::Acceptable : QValidator::Intermediate;
        }
        // Accept partial HH:MM:SS.sss input (with or without leading minus for relative)
        static QRegularExpression hmsRe(R"(^-?\d{0,3}(:\d{0,2}(:\d{0,2}(\.\d{0,3})?)?)?$)");
        return hmsRe.match(input).hasMatch() ? QValidator::Acceptable : QValidator::Intermediate;
    }

private:
    bool m_hmsMode = true;
    double m_timeOrigin = 0.0;

    static QString relativeHMS(double totalSeconds) {
        bool negative = totalSeconds < 0;
        double a = std::abs(totalSeconds);
        int h = static_cast<int>(a) / 3600;
        int m = (static_cast<int>(a) % 3600) / 60;
        int s = static_cast<int>(a) % 60;
        int ms = static_cast<int>((a - std::floor(a)) * 1000 + 0.5);
        if (ms >= 1000) { ms -= 1000; s++; }
        QString result = QString("%1:%2:%3.%4")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'))
            .arg(ms, 3, 10, QChar('0'));
        if (negative) result.prepend('-');
        return result;
    }

    static double parseRelativeHMS(const QString& text) {
        QString s = text.trimmed();
        bool negative = s.startsWith('-');
        if (negative) s = s.mid(1);

        QStringList parts = s.split(':');
        double result = 0;
        if (parts.size() == 3)
            result = parts[0].toDouble() * 3600 + parts[1].toDouble() * 60 + parts[2].toDouble();
        else if (parts.size() == 2)
            result = parts[0].toDouble() * 60 + parts[1].toDouble();
        else
            result = parts[0].toDouble();
        return negative ? -result : result;
    }
};

class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);
    ~PropertiesPanel();

    void setPanelConfigs(const std::vector<PanelConfig>& configs);

    void setClipTransform(ClipTransform* transform);
    void updateTransformLabels();
    void setClipInfo(const ClipInfo& info);
    void clearClipInfo();
    void setClipPlacement(const ClipPlacement& placement);
    void clearClipPlacement();

signals:
    void configChanged(int panelIndex, const PanelConfig& config);
    void transformChanged();
    void placementChanged(double newStart, double newDuration);

private:
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;

    QGroupBox* m_clipInfoGroup = nullptr;
    QLabel* m_clipInfoLabel = nullptr;

    QGroupBox* m_placementGroup = nullptr;
    TimeSpinBox* m_startSpin = nullptr;
    TimeSpinBox* m_endSpin = nullptr;
    TimeSpinBox* m_durationSpin = nullptr;
    QCheckBox* m_timeFormatCheck = nullptr;
    bool m_placementUpdating = false;

    QGroupBox* m_transformGroup = nullptr;
    QLabel* m_scaleLabel = nullptr;
    QLabel* m_rotationLabel = nullptr;
    QLabel* m_panLabel = nullptr;
    ClipTransform* m_transform = nullptr;
};
