#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <functional>

enum class ExportMode {
    BurnIn,         // Overlay rendered into video frames
    SubtitleTrack   // FIT data as ASS/SRT subtitle stream
};

struct ExportSettings {
    QString outputPath;
    ExportMode mode = ExportMode::BurnIn;
    int width = 0;          // 0 = same as source
    int height = 0;
    double fps = 0.0;       // 0 = same as source
    int videoBitrate = 8000000;
    QString videoCodec = "libx264";
    bool copyAudio = true;
    int audioBitrate = 128000;
};

class MediaExporter : public QObject {
    Q_OBJECT
public:
    explicit MediaExporter(QObject* parent = nullptr);
    ~MediaExporter();

    using OverlayCallback = std::function<QImage(const QImage& frame, double pts)>;

    bool startExport(const QString& inputPath, const ExportSettings& settings,
                     OverlayCallback overlayFn);
    void cancel();
    bool isExporting() const { return m_exporting; }

signals:
    void progress(double fraction);  // 0.0 to 1.0
    void finished(bool success, const QString& message);

private:
    bool m_exporting = false;
    bool m_cancelled = false;
};
