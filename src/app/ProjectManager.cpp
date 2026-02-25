#include "ProjectManager.h"
#include "TimelineModel.h"
#include "Track.h"
#include "Clip.h"
#include "ClipTransform.h"
#include "OverlayRenderer.h"
#include "OverlayPanel.h"
#include "OverlayConfig.h"
#include "TimeSync.h"
#include "FitTrack.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QRegularExpression>
#include <memory>

ProjectManager::ProjectManager(QObject* parent)
    : QObject(parent)
    , m_autosaveTimer(new QTimer(this))
{
    connect(m_autosaveTimer, &QTimer::timeout, this, &ProjectManager::onAutosaveTimeout);
}

ProjectManager::~ProjectManager() = default;

bool ProjectManager::saveProject(const QString& filePath,
                                 const ProjectSettings& settings,
                                 TimelineModel* timelineModel,
                                 OverlayRenderer* overlayRenderer,
                                 const QStringList& importedMediaPaths)
{
    if (!timelineModel || !overlayRenderer) {
        m_error = "Invalid model or renderer";
        return false;
    }

    QFileInfo fileInfo(filePath);
    QString projectDir = fileInfo.absolutePath();

    QJsonObject root;
    root["version"] = 1;
    
    // Save the original project file path (for autosave recovery)
    // If this is an autosave, save the original path; otherwise save the current path
    QString originalPath = filePath.endsWith(".autosave") ? m_currentProjectPath : filePath;
    root["originalProjectPath"] = originalPath;
    
    root["settings"] = settingsToJson(settings);

    // Save imported media files
    QJsonArray mediaArray;
    for (const QString& path : importedMediaPaths) {
        QJsonObject mediaObj;
        QString relativePath = makeRelativePath(path, projectDir);
        mediaObj["path"] = formatPathWithAbsolute(relativePath, path);
        mediaArray.append(mediaObj);
    }
    root["importedMedia"] = mediaArray;

    // Save tracks and clips
    QJsonArray tracksArray;
    for (int i = 0; i < timelineModel->trackCount(); ++i) {
        Track* track = timelineModel->track(i);
        if (track) {
            tracksArray.append(trackToJson(track, projectDir));
        }
    }
    root["tracks"] = tracksArray;

    // Save overlay panels
    QJsonArray panelsArray = panelsToJson(overlayRenderer->panelsConfig());
    root["overlayPanels"] = panelsArray;

    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_error = QString("Cannot write to: %1").arg(filePath);
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    // Clear autosave files after successful save
    clearAutosaveFiles(filePath);

    m_currentProjectPath = filePath;
    return true;
}

bool ProjectManager::loadProject(const QString& filePath,
                                 ProjectSettings& settings,
                                 TimelineModel* timelineModel,
                                 OverlayRenderer* overlayRenderer,
                                 TimeSync* timeSync,
                                 QStringList& importedMediaPaths,
                                 QString* originalProjectPath)
{
    if (!timelineModel || !overlayRenderer || !timeSync) {
        m_error = "Invalid model, renderer, or time sync";
        return false;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        m_error = QString("File not found: %1").arg(filePath);
        return false;
    }

    QString projectDir = fileInfo.absolutePath();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_error = QString("Cannot read: %1").arg(filePath);
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        m_error = "Invalid project file format";
        return false;
    }

    QJsonObject root = doc.object();

    // Check version
    int version = root["version"].toInt(0);
    if (version < 1) {
        m_error = "Unsupported project file version";
        return false;
    }

    // Load the original project path (for autosave recovery)
    if (originalProjectPath) {
        *originalProjectPath = root["originalProjectPath"].toString(filePath);
    }

    // Load settings
    settings = settingsFromJson(root["settings"].toObject());

    // Load imported media files
    importedMediaPaths.clear();
    QJsonArray mediaArray = root["importedMedia"].toArray();
    for (const auto& mediaVal : mediaArray) {
        QJsonObject mediaObj = mediaVal.toObject();
        QString savedPath = mediaObj["path"].toString();
        QString resolvedPath = resolvePath(savedPath, projectDir);
        if (!resolvedPath.isEmpty()) {
            importedMediaPaths.append(resolvedPath);
        }
    }

    // Clear existing tracks
    while (timelineModel->trackCount() > 0) {
        timelineModel->removeTrack(0);
    }

    // Load tracks
    QJsonArray tracksArray = root["tracks"].toArray();
    for (const auto& trackVal : tracksArray) {
        QJsonObject trackObj = trackVal.toObject();
        QString trackName = trackObj["name"].toString();
        QString trackTypeStr = trackObj["type"].toString();
        
        TrackType trackType = TrackType::Video;
        if (trackTypeStr == "audio") trackType = TrackType::Audio;
        else if (trackTypeStr == "fitdata") trackType = TrackType::FitData;

        Track* track = timelineModel->addTrack(trackType, trackName);
        if (track) {
            track->setMuted(trackObj["muted"].toBool(false));
            
            // Load clips
            QJsonArray clipsArray = trackObj["clips"].toArray();
            for (const auto& clipVal : clipsArray) {
                Clip clip = clipFromJson(clipVal.toObject(), projectDir);
                track->addClip(clip);
            }
        }
    }

    // Load overlay panels
    QJsonArray panelsArray = root["overlayPanels"].toArray();
    std::vector<PanelConfig> panels = panelsFromJson(panelsArray);
    overlayRenderer->setPanelsConfig(panels);

    // Set time sync offset
    timeSync->setFitTimeOffset(settings.fitTimeOffset);

    // Update timeline view settings
    timelineModel->setZoom(settings.timelineZoom);
    timelineModel->setScrollOffset(settings.timelineScrollOffset);
    timelineModel->setTimeOrigin(settings.timeOrigin);
    timelineModel->setPlayheadPosition(settings.playheadPosition);

    return true;
}

void ProjectManager::startAutosave(const QString& projectFilePath,
                                   const ProjectSettings& settings,
                                   TimelineModel* timelineModel,
                                   OverlayRenderer* overlayRenderer,
                                   const QStringList& importedMediaPaths)
{
    m_currentProjectPath = projectFilePath;
    m_currentSettings = settings;
    m_timelineModel = timelineModel;
    m_overlayRenderer = overlayRenderer;
    m_importedMediaPaths = importedMediaPaths;
    
    m_autosaveTimer->start(AutosaveIntervalMs);
}

void ProjectManager::stopAutosave()
{
    m_autosaveTimer->stop();
    m_timelineModel = nullptr;
    m_overlayRenderer = nullptr;
}

void ProjectManager::clearAutosaveFiles(const QString& projectFilePath)
{
    QString fileName = projectBaseName(projectFilePath);
    QString dir = QFileInfo(projectFilePath).absolutePath();
    QString pattern = fileName + "*.fvProj.autosave";

    QDir projectDir(dir);
    QStringList filters;
    filters << pattern;
    QStringList files = projectDir.entryList(filters, static_cast<QDir::Filters>(QDir::Files));

    for (const QString& file : files) {
        QFile::remove(projectDir.absoluteFilePath(file));
    }
}

bool ProjectManager::hasAutosaveFiles(const QString& projectFilePath) const
{
    return !getLatestAutosaveFile(projectFilePath).isEmpty();
}

QString ProjectManager::getLatestAutosaveFile(const QString& projectFilePath) const
{
    QString fileName = projectBaseName(projectFilePath);
    QString dir = QFileInfo(projectFilePath).absolutePath();
    QString pattern = fileName + "*.fvProj.autosave";

    QDir projectDir(dir);
    QStringList filters;
    filters << pattern;
    QStringList files = projectDir.entryList(filters, QDir::Files, QDir::Time);

    if (files.isEmpty()) {
        return QString();
    }

    return projectDir.absoluteFilePath(files.first());
}

void ProjectManager::onAutosaveTimeout()
{
    if (m_currentProjectPath.isEmpty() || !m_timelineModel || !m_overlayRenderer) {
        return;
    }

    // Get the base name - strip any existing timestamps or autosave extensions
    QString fileName = projectBaseName(m_currentProjectPath);

    QString dir = QFileInfo(m_currentProjectPath).absolutePath();
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString autosavePath = dir + "/" + fileName + "_" + timestamp + ".fvProj.autosave";

    if (saveProject(autosavePath, m_currentSettings, m_timelineModel, m_overlayRenderer, m_importedMediaPaths)) {
        emit autosaveTriggered(autosavePath);
        
        // Clean up old autosave files
        QDir projectDir(dir);
        QStringList filters;
        filters << fileName + "*.fvProj.autosave";
        QStringList files = projectDir.entryList(filters, QDir::Files, QDir::Time);
        
        for (int i = MaxAutosaveFiles; i < files.size(); ++i) {
            QFile::remove(projectDir.absoluteFilePath(files[i]));
        }
    }
}

QJsonObject ProjectManager::settingsToJson(const ProjectSettings& settings) const
{
    QJsonObject obj;
    obj["canvasWidth"] = settings.canvasSize.width();
    obj["canvasHeight"] = settings.canvasSize.height();
    obj["timelineZoom"] = settings.timelineZoom;
    obj["timelineScrollOffset"] = settings.timelineScrollOffset;
    obj["timeOrigin"] = settings.timeOrigin;
    obj["playheadPosition"] = settings.playheadPosition;
    obj["fitTimeOffset"] = settings.fitTimeOffset;
    return obj;
}

ProjectSettings ProjectManager::settingsFromJson(const QJsonObject& obj) const
{
    ProjectSettings settings;
    settings.canvasSize = QSize(
        obj["canvasWidth"].toInt(1920),
        obj["canvasHeight"].toInt(1080)
    );
    settings.timelineZoom = obj["timelineZoom"].toDouble(1.0);
    settings.timelineScrollOffset = obj["timelineScrollOffset"].toDouble(0.0);
    settings.timeOrigin = obj["timeOrigin"].toDouble(0.0);
    settings.playheadPosition = obj["playheadPosition"].toDouble(0.0);
    settings.fitTimeOffset = obj["fitTimeOffset"].toDouble(0.0);
    return settings;
}

QJsonObject ProjectManager::trackToJson(const Track* track, const QString& projectDir) const
{
    QJsonObject obj;
    
    QString typeStr = "video";
    if (track->type() == TrackType::Audio) typeStr = "audio";
    else if (track->type() == TrackType::FitData) typeStr = "fitdata";
    
    obj["type"] = typeStr;
    obj["name"] = track->name();
    obj["muted"] = track->isMuted();

    QJsonArray clipsArray;
    for (int i = 0; i < track->clipCount(); ++i) {
        clipsArray.append(clipToJson(track->clip(i), projectDir));
    }
    obj["clips"] = clipsArray;

    return obj;
}

QJsonObject ProjectManager::clipToJson(const Clip& clip, const QString& projectDir) const
{
    QJsonObject obj;
    
    // Format path with relative and absolute
    QString relativePath = makeRelativePath(clip.sourcePath, projectDir);
    obj["sourcePath"] = formatPathWithAbsolute(relativePath, clip.sourcePath);
    
    obj["displayName"] = clip.displayName;
    
    QString typeStr = "video";
    if (clip.type == ClipType::Audio) typeStr = "audio";
    else if (clip.type == ClipType::Image) typeStr = "image";
    else if (clip.type == ClipType::FitData) typeStr = "fitdata";
    obj["type"] = typeStr;
    
    obj["sourceIn"] = clip.sourceIn;
    obj["sourceOut"] = clip.sourceOut;
    obj["timelineOffset"] = clip.timelineOffset;
    obj["absoluteStartTime"] = clip.absoluteStartTime;
    obj["locked"] = clip.locked;
    
    // Clip transform
    QJsonObject transformObj;
    transformObj["scale"] = clip.transform.scale;
    transformObj["rotation"] = clip.transform.rotation;
    transformObj["panX"] = clip.transform.panX;
    transformObj["panY"] = clip.transform.panY;
    transformObj["flipH"] = clip.transform.flipH;
    transformObj["flipV"] = clip.transform.flipV;
    obj["transform"] = transformObj;

    return obj;
}

Clip ProjectManager::clipFromJson(const QJsonObject& obj, const QString& projectDir) const
{
    Clip clip;
    
    QString savedPath = obj["sourcePath"].toString();
    clip.sourcePath = resolvePath(savedPath, projectDir);
    clip.displayName = obj["displayName"].toString();
    
    QString typeStr = obj["type"].toString();
    if (typeStr == "audio") clip.type = ClipType::Audio;
    else if (typeStr == "image") clip.type = ClipType::Image;
    else if (typeStr == "fitdata") clip.type = ClipType::FitData;
    else clip.type = ClipType::Video;
    
    clip.sourceIn = obj["sourceIn"].toDouble(0.0);
    clip.sourceOut = obj["sourceOut"].toDouble(0.0);
    clip.timelineOffset = obj["timelineOffset"].toDouble(0.0);
    clip.absoluteStartTime = obj["absoluteStartTime"].toDouble(0.0);
    clip.locked = obj["locked"].toBool(false);
    
    // Clip transform
    QJsonObject transformObj = obj["transform"].toObject();
    clip.transform.scale = transformObj["scale"].toDouble(1.0);
    clip.transform.rotation = transformObj["rotation"].toDouble(0.0);
    clip.transform.panX = transformObj["panX"].toDouble(0.0);
    clip.transform.panY = transformObj["panY"].toDouble(0.0);
    clip.transform.flipH = transformObj["flipH"].toBool(false);
    clip.transform.flipV = transformObj["flipV"].toBool(false);

    return clip;
}

QJsonArray ProjectManager::panelsToJson(const std::vector<PanelConfig>& panels) const
{
    QJsonArray arr;
    for (const auto& panel : panels) {
        arr.append(OverlayConfig::panelToJson(panel));
    }
    return arr;
}

std::vector<PanelConfig> ProjectManager::panelsFromJson(const QJsonArray& arr) const
{
    std::vector<PanelConfig> panels;
    for (const auto& val : arr) {
        panels.push_back(OverlayConfig::panelFromJson(val.toObject()));
    }
    return panels;
}

QString ProjectManager::makeRelativePath(const QString& absolutePath, const QString& projectDir) const
{
    if (absolutePath.isEmpty()) {
        return QString();
    }
    
    QDir projDir(projectDir);
    QString relativePath = projDir.relativeFilePath(absolutePath);
    
    // If the relative path starts with "..", it's outside the project directory
    // In that case, we still return it but it will be resolved to absolute on load
    return relativePath;
}

QString ProjectManager::resolvePath(const QString& savedPath, const QString& projectDir) const
{
    if (savedPath.isEmpty()) {
        return QString();
    }

    // Check if the path is in the format: "relative/path (C:/absolute/path)"
    QRegularExpression regex(R"(^(.+)\s+\((.+)\)$)");
    QRegularExpressionMatch match = regex.match(savedPath);
    
    QString relativePath;
    QString absolutePath;
    
    if (match.hasMatch()) {
        relativePath = match.captured(1).trimmed();
        absolutePath = match.captured(2).trimmed();
    } else {
        // Old format or simple path
        relativePath = savedPath;
    }

    // First try the relative path
    QDir projDir(projectDir);
    QString resolvedRelative = projDir.absoluteFilePath(relativePath);
    
    if (QFile::exists(resolvedRelative)) {
        return resolvedRelative;
    }

    // Try the absolute path if available
    if (!absolutePath.isEmpty() && QFile::exists(absolutePath)) {
        return absolutePath;
    }

    // Return the resolved relative path even if it doesn't exist
    // (file might be moved, let the UI handle this)
    return resolvedRelative;
}

QString ProjectManager::formatPathWithAbsolute(const QString& relativePath, const QString& absolutePath) const
{
    if (absolutePath.isEmpty()) {
        return relativePath;
    }
    return QString("%1 (%2)").arg(relativePath, absolutePath);
}

QString ProjectManager::projectBaseName(const QString& path) const
{
    QString fileName = QFileInfo(path).fileName();

    // Remove .fvProj.autosave or .fvProj extension
    if (fileName.endsWith(".fvProj.autosave", Qt::CaseInsensitive)) {
        fileName = fileName.left(fileName.length() - 16);
    } else if (fileName.endsWith(".fvProj", Qt::CaseInsensitive)) {
        fileName = fileName.left(fileName.length() - 7);
    }

    // Remove timestamp pattern if present (e.g. _20260225_103803)
    QRegularExpression timestampRegex("_\\d{8}_\\d{6}$");
    fileName = fileName.remove(timestampRegex);

    return fileName;
}
