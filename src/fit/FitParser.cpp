#include "FitParser.h"
#include <QFile>

FitParser::FitParser(QObject* parent) : QObject(parent) {}
FitParser::~FitParser() = default;

bool FitParser::parse(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_error = QString("Cannot open file: %1").arg(filePath);
        emit error(m_error);
        return false;
    }

#ifdef HAS_FIT_SDK
    // TODO: Phase 2 - Implement Garmin FIT SDK decode with MesgListeners
    m_error = "FIT SDK parsing not yet implemented";
    emit error(m_error);
    return false;
#else
    m_error = "FIT SDK not available - build with HAS_FIT_SDK";
    emit error(m_error);
    return false;
#endif
}
