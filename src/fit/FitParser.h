#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include "FitData.h"

class FitParser : public QObject {
    Q_OBJECT
public:
    explicit FitParser(QObject* parent = nullptr);
    ~FitParser();

    bool parse(const QString& filePath);
    const FitSession& session() const { return m_session; }
    QString errorString() const { return m_error; }

signals:
    void parsed(const FitSession& session);
    void error(const QString& message);

private:
    FitSession m_session;
    QString m_error;
};
