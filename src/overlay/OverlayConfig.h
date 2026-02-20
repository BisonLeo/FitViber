#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <vector>
#include "OverlayPanel.h"

class OverlayConfig : public QObject {
    Q_OBJECT
public:
    explicit OverlayConfig(QObject* parent = nullptr);
    ~OverlayConfig();

    bool save(const QString& filePath, const std::vector<PanelConfig>& panels);
    bool load(const QString& filePath, std::vector<PanelConfig>& panels);

    static QJsonObject panelToJson(const PanelConfig& config);
    static PanelConfig panelFromJson(const QJsonObject& obj);

    QString errorString() const { return m_error; }

private:
    QString m_error;
};
