#pragma once

#include <QObject>
#include <QString>
#include <QSize>
#include <QTimer>
#include <memory>
#include <vector>

class TimelineModel;
class Track;
class OverlayRenderer;
class TimeSync;
struct Clip;
struct PanelConfig;

struct ProjectSettings {
    QSize canvasSize{1920, 1080};
    double timelineZoom = 1.0;
    double timelineScrollOffset = 0.0;
    double timeOrigin = 0.0;
    double playheadPosition = 0.0;
    double fitTimeOffset = 0.0;
};

class ProjectManager : public QObject {
    Q_OBJECT
public:
    explicit ProjectManager(QObject* parent = nullptr);
    ~ProjectManager();

    // Project file operations
    bool saveProject(const QString& filePath,
                     const ProjectSettings& settings,
                     TimelineModel* timelineModel,
                     OverlayRenderer* overlayRenderer,
                     const QStringList& importedMediaPaths);
    
    bool loadProject(const QString& filePath,
                     ProjectSettings& settings,
                     TimelineModel* timelineModel,
                     OverlayRenderer* overlayRenderer,
                     TimeSync* timeSync,
                     QStringList& importedMediaPaths,
                     QString* originalProjectPath = nullptr);

    // Autosave functionality
    void startAutosave(const QString& projectFilePath,
                       const ProjectSettings& settings,
                       TimelineModel* timelineModel,
                       OverlayRenderer* overlayRenderer,
                       const QStringList& importedMediaPaths);
    void stopAutosave();
    void clearAutosaveFiles(const QString& projectFilePath);
    bool hasAutosaveFiles(const QString& projectFilePath) const;
    QString getLatestAutosaveFile(const QString& projectFilePath) const;

    QString errorString() const { return m_error; }

signals:
    void autosaveTriggered(const QString& autosavePath);

private slots:
    void onAutosaveTimeout();

private:
    // JSON serialization helpers
    QJsonObject settingsToJson(const ProjectSettings& settings) const;
    ProjectSettings settingsFromJson(const QJsonObject& obj) const;
    
    QJsonObject trackToJson(const Track* track, const QString& projectDir) const;
    QJsonObject clipToJson(const Clip& clip, const QString& projectDir) const;
    Clip clipFromJson(const QJsonObject& obj, const QString& projectDir) const;
    
    QJsonArray panelsToJson(const std::vector<PanelConfig>& panels) const;
    std::vector<PanelConfig> panelsFromJson(const QJsonArray& arr) const;

    // Path utilities
    QString makeRelativePath(const QString& absolutePath, const QString& projectDir) const;
    QString resolvePath(const QString& savedPath, const QString& projectDir) const;

    // Format: "relative/path/to/file.ext (C:/absolute/path/to/file.ext)"
    QString formatPathWithAbsolute(const QString& relativePath, const QString& absolutePath) const;

    // Extract base name from project/autosave path, stripping extensions and timestamps
    QString projectBaseName(const QString& path) const;

    QTimer* m_autosaveTimer = nullptr;
    QString m_currentProjectPath;      // The actual project file path (not autosave)
    ProjectSettings m_currentSettings;
    TimelineModel* m_timelineModel = nullptr;
    OverlayRenderer* m_overlayRenderer = nullptr;
    QStringList m_importedMediaPaths;
    
    QString m_error;
    static constexpr int AutosaveIntervalMs = 15000; // 15 seconds
    static constexpr int MaxAutosaveFiles = 5;
};
