#include "OverlayConfig.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

OverlayConfig::OverlayConfig(QObject* parent) : QObject(parent) {}
OverlayConfig::~OverlayConfig() = default;

bool OverlayConfig::save(const QString& filePath, const std::vector<PanelConfig>& panels) {
    QJsonArray arr;
    for (const auto& p : panels) {
        arr.append(panelToJson(p));
    }

    QJsonObject root;
    root["version"] = 1;
    root["panels"] = arr;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_error = QString("Cannot write to: %1").arg(filePath);
        return false;
    }

    file.write(QJsonDocument(root).toJson());
    return true;
}

bool OverlayConfig::load(const QString& filePath, std::vector<PanelConfig>& panels) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_error = QString("Cannot read: %1").arg(filePath);
        return false;
    }

    auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        m_error = "Invalid config format";
        return false;
    }

    auto root = doc.object();
    auto arr = root["panels"].toArray();

    panels.clear();
    for (const auto& val : arr) {
        panels.push_back(panelFromJson(val.toObject()));
    }
    return true;
}

QJsonObject OverlayConfig::panelToJson(const PanelConfig& config) {
    QJsonObject obj;
    obj["type"] = static_cast<int>(config.type);
    obj["visible"] = config.visible;
    obj["x"] = config.x;
    obj["y"] = config.y;
    obj["width"] = config.width;
    obj["height"] = config.height;
    obj["textColor"] = config.textColor.name(QColor::HexArgb);
    obj["bgColor"] = config.bgColor.name(QColor::HexArgb);
    obj["fontSize"] = config.font.pointSize();
    obj["opacity"] = config.opacity;
    obj["label"] = config.label;
    return obj;
}

PanelConfig OverlayConfig::panelFromJson(const QJsonObject& obj) {
    PanelConfig cfg;
    cfg.type = static_cast<PanelType>(obj["type"].toInt());
    cfg.visible = obj["visible"].toBool(true);
    cfg.x = obj["x"].toDouble();
    cfg.y = obj["y"].toDouble();
    cfg.width = obj["width"].toDouble(0.15);
    cfg.height = obj["height"].toDouble(0.08);
    cfg.textColor = QColor(obj["textColor"].toString("#ffffffff"));
    cfg.bgColor = QColor(obj["bgColor"].toString("#80000000"));
    cfg.font.setPointSize(obj["fontSize"].toInt(16));
    cfg.opacity = obj["opacity"].toDouble(0.85);
    cfg.label = obj["label"].toString();
    return cfg;
}
