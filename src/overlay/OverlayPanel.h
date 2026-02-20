#pragma once

#include <QObject>
#include <QPainter>
#include <QRectF>
#include <QString>
#include <QColor>
#include <QFont>
#include "FitData.h"

enum class PanelType {
    Speed,
    HeartRate,
    Cadence,
    Power,
    Elevation,
    MiniMap,
    Distance,
    Lap
};

struct PanelConfig {
    PanelType type;
    bool visible = true;
    double x = 0.0;       // normalized 0.0-1.0
    double y = 0.0;
    double width = 0.15;
    double height = 0.08;
    QColor textColor = Qt::white;
    QColor bgColor = QColor(0, 0, 0, 128);
    QFont font = QFont("Arial", 16, QFont::Bold);
    double opacity = 0.85;
    QString label;         // optional custom label
};

class OverlayPanel : public QObject {
    Q_OBJECT
public:
    explicit OverlayPanel(PanelType type, QObject* parent = nullptr);
    virtual ~OverlayPanel();

    virtual void paint(QPainter& painter, const QRect& panelRect,
                       const FitRecord& record, const FitSession& session) = 0;

    virtual QString defaultLabel() const = 0;
    virtual PanelType panelType() const { return m_config.type; }

    PanelConfig& config() { return m_config; }
    const PanelConfig& config() const { return m_config; }
    void setConfig(const PanelConfig& cfg) { m_config = cfg; }

    QRect resolveRect(int videoWidth, int videoHeight) const;

protected:
    void paintBackground(QPainter& painter, const QRect& rect);
    void paintLabel(QPainter& painter, const QRect& rect, const QString& label);
    void paintValue(QPainter& painter, const QRect& rect,
                    const QString& value, const QString& unit);

    PanelConfig m_config;
};
