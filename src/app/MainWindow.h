#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include <QSize>
#include <map>
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
class ProjectManager;
struct ClipTransform;
struct ProjectSettings;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onMediaSelected(const QString& path);
    void onFitFileOpened(const QString& path);
    void onExportRequested();
    void onPlaybackTick(double currentTime);
    void onTimelineSeek(double relativeSeconds);
    void onTimelineScrub(double relativeSeconds);
    void onClipSelectionChanged(int trackIndex, int clipIndex);
    void onCanvasSettings();
    
    // Project file operations
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onAutosaveTriggered(const QString& autosavePath);

private:
    void setupUi();
    void setupMenuBar();
    void setupDockWidgets();
    void connectSignals();
    void renderOverlay(QImage& frame, double currentTime);
    QImage applyTransform(const QImage& source, const ClipTransform& transform);
    bool maybeSaveModified(); // returns false if the user cancelled

    QDockWidget* m_mediaDock = nullptr;
    QDockWidget* m_propertiesDock = nullptr;
    QDockWidget* m_timelineDock = nullptr;

    MediaBrowser* m_mediaBrowser = nullptr;
    PreviewWidget* m_previewWidget = nullptr;
    PropertiesPanel* m_propertiesPanel = nullptr;
    TimelineWidget* m_timelineWidget = nullptr;
    PlaybackController* m_playbackController = nullptr;

    std::unique_ptr<OverlayRenderer> m_overlayRenderer;
    std::map<QString, std::unique_ptr<FitTrack>> m_fitTracks; // per-clip FIT data keyed by source path
    std::unique_ptr<FitTrack> m_previewFitTrack;
    std::unique_ptr<TimeSync> m_timeSync;
    std::unique_ptr<VideoPlaybackEngine> m_playbackEngine;
    double m_lastFramePts = 0.0;  // tracks actual video duration from decoded PTS
    QString m_currentClipPath;    // path of currently loaded clip in playback engine
    double m_currentClipTimeBase = 0.0; // timeline start time minus source start time
    bool m_playbackFromTimeline = false; // true if playback was initiated from timeline
    bool m_previewFitData = false; // true if previewing a standalone FIT file without video
    bool m_forceTimelineSeek = false; // flag to trigger seek when playing from timeline
    double m_lastSourceTime = -1.0; // previous tick's source time

    QSize m_canvasSize{1920, 1080}; // output canvas dimensions
    int m_selectedTrackIndex = -1;
    int m_selectedClipIndex = -1;
    
    // Project management
    std::unique_ptr<ProjectManager> m_projectManager;
    QString m_currentProjectPath;
    bool m_projectModified = false;
    
    void updateProjectSettings();
    ProjectSettings collectProjectSettings() const;
    void applyProjectSettings(const ProjectSettings& settings);
    void loadFitDataForClips();
};
