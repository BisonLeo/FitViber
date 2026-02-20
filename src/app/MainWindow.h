#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include <memory>

class MediaBrowser;
class PreviewWidget;
class PropertiesPanel;
class TimelineWidget;
class PlaybackController;
class OverlayRenderer;
class FitTrack;
class TimeSync;
class VideoPlaybackEngine;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onMediaSelected(const QString& path);
    void onFitFileOpened(const QString& path);
    void onExportRequested();
    void onPlaybackTick(double currentTime);
    void onTimelineSeek(double relativeSeconds);
    void onTimelineScrub(double relativeSeconds);

private:
    void setupUi();
    void setupMenuBar();
    void setupDockWidgets();
    void connectSignals();

    QDockWidget* m_mediaDock = nullptr;
    QDockWidget* m_propertiesDock = nullptr;
    QDockWidget* m_timelineDock = nullptr;

    MediaBrowser* m_mediaBrowser = nullptr;
    PreviewWidget* m_previewWidget = nullptr;
    PropertiesPanel* m_propertiesPanel = nullptr;
    TimelineWidget* m_timelineWidget = nullptr;
    PlaybackController* m_playbackController = nullptr;

    std::unique_ptr<OverlayRenderer> m_overlayRenderer;
    std::unique_ptr<FitTrack> m_fitTrack;
    std::unique_ptr<TimeSync> m_timeSync;
    std::unique_ptr<VideoPlaybackEngine> m_playbackEngine;
    double m_lastFramePts = 0.0;  // tracks actual video duration from decoded PTS
    QString m_currentClipPath;    // path of currently loaded clip in playback engine
};
