#include "MediaExporter.h"

MediaExporter::MediaExporter(QObject* parent) : QObject(parent) {}
MediaExporter::~MediaExporter() { cancel(); }

bool MediaExporter::startExport(const QString& inputPath, const ExportSettings& settings,
                                 OverlayCallback overlayFn) {
    Q_UNUSED(inputPath);
    Q_UNUSED(settings);
    Q_UNUSED(overlayFn);
    // TODO: Phase 10 - FFmpeg encode pipeline
    emit finished(false, "Export not yet implemented");
    return false;
}

void MediaExporter::cancel() {
    m_cancelled = true;
    m_exporting = false;
}
