#include "MainWindow.h"
#include "AppConstants.h"
#include "MediaBrowser.h"
#include "PreviewWidget.h"
#include "PropertiesPanel.h"
#include "TimelineWidget.h"
#include "TimelineModel.h"
#include "Track.h"
#include "Clip.h"
#include "PlaybackController.h"
#include "OverlayRenderer.h"
#include "FitTrack.h"
#include "TimeSync.h"
#include "VideoPlaybackEngine.h"
#include "OverlayPanelFactory.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_overlayRenderer(std::make_unique<OverlayRenderer>())
    , m_fitTrack(std::make_unique<FitTrack>())
    , m_timeSync(std::make_unique<TimeSync>())
    , m_playbackEngine(std::make_unique<VideoPlaybackEngine>())
{
    setWindowTitle(QString("%1 v%2").arg(AppConstants::AppName, AppConstants::AppVersion));
    resize(AppConstants::DefaultWindowWidth, AppConstants::DefaultWindowHeight);

    setupUi();
    setupMenuBar();
    setupDockWidgets();
    connectSignals();

    // Default Overlay Panels
    m_overlayRenderer->addPanel(OverlayPanelFactory::create(PanelType::HeartRate, this));
    m_overlayRenderer->addPanel(OverlayPanelFactory::create(PanelType::Distance, this));
    m_overlayRenderer->addPanel(OverlayPanelFactory::create(PanelType::Elevation, this));
    m_overlayRenderer->addPanel(OverlayPanelFactory::create(PanelType::Inclination, this));
    m_overlayRenderer->addPanel(OverlayPanelFactory::create(PanelType::Speed, this));

    statusBar()->showMessage("Ready");
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    m_previewWidget = new PreviewWidget(this);
    setCentralWidget(m_previewWidget);

    m_mediaBrowser = new MediaBrowser(this);
    m_propertiesPanel = new PropertiesPanel(this);
    m_timelineWidget = new TimelineWidget(this);
    m_playbackController = new PlaybackController(this);
}

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("&File");

    auto* openVideoAction = fileMenu->addAction("Open &Video...");
    connect(openVideoAction, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Open Video File", {},
            "Video Files (*.mp4 *.avi *.mkv *.mov *.wmv);;All Files (*)");
        if (!path.isEmpty()) onMediaSelected(path);
    });

    auto* openFitAction = fileMenu->addAction("Open &FIT File...");
    connect(openFitAction, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Open FIT File", {},
            "FIT Files (*.fit);;All Files (*)");
        if (!path.isEmpty()) onFitFileOpened(path);
    });

    fileMenu->addSeparator();

    auto* exportAction = fileMenu->addAction("&Export...");
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportRequested);

    fileMenu->addSeparator();

    auto* exitAction = fileMenu->addAction("E&xit");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto* viewMenu = menuBar()->addMenu("&View");

    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About FitViber",
            QString("<h3>FitViber v%1</h3>"
                    "<p>GPS activity overlay video editor.</p>")
            .arg(AppConstants::AppVersion));
    });

    Q_UNUSED(viewMenu);
}

void MainWindow::setupDockWidgets() {
    m_mediaDock = new QDockWidget("Media Browser", this);
    m_mediaDock->setWidget(m_mediaBrowser);
    m_mediaDock->setObjectName("MediaBrowserDock");
    addDockWidget(Qt::LeftDockWidgetArea, m_mediaDock);

    m_propertiesDock = new QDockWidget("Properties", this);
    m_propertiesDock->setWidget(m_propertiesPanel);
    m_propertiesDock->setObjectName("PropertiesDock");
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);

    m_timelineDock = new QDockWidget("Timeline", this);
    m_timelineDock->setWidget(m_timelineWidget);
    m_timelineDock->setObjectName("TimelineDock");
    addDockWidget(Qt::BottomDockWidgetArea, m_timelineDock);

    // Add toggle actions to View menu
    auto* viewMenu = menuBar()->findChild<QMenu*>(QString(), Qt::FindDirectChildrenOnly);
    if (!viewMenu) {
        for (auto* action : menuBar()->actions()) {
            if (action->text() == "&View") {
                viewMenu = action->menu();
                break;
            }
        }
    }
    if (viewMenu) {
        viewMenu->addAction(m_mediaDock->toggleViewAction());
        viewMenu->addAction(m_propertiesDock->toggleViewAction());
        viewMenu->addAction(m_timelineDock->toggleViewAction());
    }
}

void MainWindow::connectSignals() {
    // MediaBrowser signals
    connect(m_mediaBrowser, &MediaBrowser::mediaSelected,
            this, &MainWindow::onMediaSelected);
    connect(m_mediaBrowser, &MediaBrowser::fitFileSelected,
            this, &MainWindow::onFitFileOpened);

    // Playback tick → pull next frame from decode queue
    connect(m_playbackController, &PlaybackController::tick,
            this, &MainWindow::onPlaybackTick);

    // Playback state → preview button appearance
    connect(m_playbackController, &PlaybackController::stateChanged,
            this, [this](PlaybackState state) {
        m_previewWidget->setPlayingState(state == PlaybackState::Playing);
    });

    // Preview play/pause button and video area click
    connect(m_previewWidget, &PreviewWidget::playPauseClicked,
            m_playbackController, &PlaybackController::togglePlayPause);
    connect(m_previewWidget, &PreviewWidget::videoAreaClicked,
            m_playbackController, &PlaybackController::togglePlayPause);

    // Step buttons — these need explicit seek since they jump to specific frames
    connect(m_previewWidget, &PreviewWidget::stepForward,
            m_playbackController, &PlaybackController::stepForward);
    connect(m_previewWidget, &PreviewWidget::stepBackward,
            m_playbackController, &PlaybackController::stepBackward);

    // Seek from preview slider → goes through PlaybackController
    connect(m_previewWidget, &PreviewWidget::seekRequested,
            m_playbackController, &PlaybackController::seek);

    // Any seek/step performed by PlaybackController → sync the decode engine
    connect(m_playbackController, &PlaybackController::seekPerformed, this, [this](double seconds) {
        if (!m_playbackFromTimeline) {
            if (m_playbackEngine->isOpen()) m_playbackEngine->seek(seconds);
        } else {
            m_forceTimelineSeek = true;
        }
    });

    // Timeline seek/scrub → find clip and display frame
    connect(m_timelineWidget, &TimelineWidget::seekRequested,
            this, &MainWindow::onTimelineSeek);
    connect(m_timelineWidget, &TimelineWidget::playheadScrubbed,
            this, &MainWindow::onTimelineScrub);
}

void MainWindow::onMediaSelected(const QString& path) {
    QFileInfo info(path);
    QString suffix = info.suffix().toLower();

    // Check if it's an image
    QStringList imageExts = {"jpg", "jpeg", "png", "bmp", "tiff", "tif"};
    if (imageExts.contains(suffix)) {
        // Image mode
        m_playbackController->stop();
        m_playbackEngine->close();

        QImage img(path);
        if (!img.isNull()) {
            m_previewWidget->showImage(img);
            statusBar()->showMessage(QString("Showing image: %1").arg(info.fileName()));
        } else {
            statusBar()->showMessage(QString("Failed to load image: %1").arg(info.fileName()));
        }
        return;
    }

    // Video mode
    m_playbackController->stop();
    m_playbackEngine->close();

    m_playbackFromTimeline = false;
    m_currentClipPath = path;

    if (!m_playbackEngine->open(path)) {
        statusBar()->showMessage(QString("Failed to open video: %1").arg(info.fileName()));
        return;
    }

    const auto& vi = m_playbackEngine->info();
    m_playbackController->setFps(vi.fps > 0 ? vi.fps : 30.0);
    // Set a large duration on the controller — actual EOF is driven by the decode engine
    m_playbackController->setDuration(vi.duration > 0 ? vi.duration * 10.0 : 3600.0);
    m_lastFramePts = 0.0;

    m_previewWidget->showVideo();

    // Show first frame — pull from decode queue (give decode thread a moment)
    QThread::msleep(50);
    TimedFrame firstFrame = m_playbackEngine->nextFrame();
    if (!firstFrame.image.isNull()) {
        m_previewWidget->displayFrame(firstFrame.image);
        m_lastFramePts = firstFrame.pts;
    }
    // Duration will be corrected once we see the last frame; for now use metadata
    m_previewWidget->setDuration(vi.duration);
    m_previewWidget->setCurrentTime(0.0);

    statusBar()->showMessage(QString("Loaded video: %1 (%2x%3 @ %4fps, %5s)")
        .arg(info.fileName())
        .arg(vi.width).arg(vi.height)
        .arg(vi.fps, 0, 'f', 1)
        .arg(vi.duration, 0, 'f', 1));
}

void MainWindow::onPlaybackTick(double currentTime) {
    if (!m_playbackFromTimeline) {
        if (!m_playbackEngine->isOpen()) return;

        TimedFrame frame = m_playbackEngine->nextFrame();
        if (!frame.image.isNull()) {
            QImage renderImage = frame.image.convertToFormat(QImage::Format_ARGB32);
            renderImage.detach();

            FitRecord rec;
            FitSession sesh;
            if (m_fitTrack && !m_fitTrack->isEmpty()) {
                rec = m_fitTrack->getRecordAtTime(frame.pts + m_timeSync->fitTimeOffset());
                sesh = m_fitTrack->session();
            } else {
                double loopTime = std::fmod(frame.pts, 10.0); // 10 second loop
                double progress = loopTime / 10.0;
                rec.speed = (progress * 200.0f) / 3.6f;
                rec.distance = (progress * 100.0f) * 1000.0f;
                rec.heartRate = progress * 200.0f;
                rec.hasHeartRate = true;
                rec.altitude = -100.0f + progress * 10100.0f;
                rec.grade = -100.0f + progress * 200.0f;
                sesh.records.push_back(rec);
            }

            m_overlayRenderer->render(renderImage, rec, sesh);
            m_previewWidget->displayFrame(renderImage);
            
            m_lastFramePts = frame.pts;
            m_previewWidget->setCurrentTime(frame.pts);
        } else if (m_playbackEngine->isFinished()) {
            m_playbackController->pause();
            m_previewWidget->setDuration(m_lastFramePts);
            m_previewWidget->setCurrentTime(m_lastFramePts);
        }
        return;
    }

    // --- TIMELINE PREVIEW MODE ---
    m_timelineWidget->model()->setPlayheadPosition(currentTime);
    
    double dur = m_timelineWidget->model()->duration();
    if (dur > 0) {
        m_playbackController->setDuration(dur);
        m_previewWidget->setDuration(dur);
    }

    // Find clip at currentTime
    auto* model = m_timelineWidget->model();
    const Clip* currentClip = nullptr;
    for (int ti = 0; ti < model->trackCount(); ++ti) {
        Track* track = model->track(ti);
        if (!track) continue;
        for (const auto& clip : track->clips()) {
            if (currentTime >= clip.timelineOffset && currentTime < clip.timelineOffset + clip.duration()) {
                currentClip = &clip;
                break;
            }
        }
        if (currentClip) break;
    }

    if (!currentClip) {
        // No clip: black screen
        if (m_playbackEngine->isOpen()) {
            m_playbackEngine->close();
        }
        m_currentClipPath.clear();
        m_lastSourceTime = -1.0;
        
        QSize s = m_previewWidget->size();
        if (!s.isValid() || s.width() < 1) s = QSize(1280, 720);
        QImage blackFrame(s, QImage::Format_RGB32);
        blackFrame.fill(Qt::black);
        m_previewWidget->showVideo();
        m_previewWidget->displayFrame(blackFrame);
        m_previewWidget->setCurrentTime(currentTime);
        return;
    }

    if (currentClip->type == ClipType::Image) {
        if (m_currentClipPath != currentClip->sourcePath) {
            if (m_playbackEngine->isOpen()) m_playbackEngine->close();
            m_currentClipPath = currentClip->sourcePath;
            QImage img(m_currentClipPath);
            if (!img.isNull()) {
                m_previewWidget->showImage(img);
            }
        }
        m_previewWidget->setCurrentTime(currentTime);
        m_lastSourceTime = -1.0;
        return;
    }

    // Video clip
    m_previewWidget->showVideo();
    double sourceTime = currentTime - currentClip->timelineOffset + currentClip->sourceIn;
    
    bool justOpened = false;
    if (m_currentClipPath != currentClip->sourcePath || !m_playbackEngine->isOpen()) {
        m_playbackEngine->close();
        if (m_playbackEngine->open(currentClip->sourcePath)) {
            m_currentClipPath = currentClip->sourcePath;
            justOpened = true;
            double fps = m_playbackEngine->info().fps;
            if (fps > 0) m_playbackController->setFps(fps);
        } else {
            m_currentClipPath.clear();
            return;
        }
    }

    // Seek if necessary (scrubbed, opened, paused, or discontinuous time)
    bool continuous = true;
    if (m_lastSourceTime >= 0.0) {
        // If sourceTime jumped by a noticeable amount instead of advancing sequentially
        if (std::abs(sourceTime - m_lastSourceTime) > 0.5) {
            continuous = false;
        }
    }

    bool shouldSeek = justOpened || m_forceTimelineSeek || !continuous || 
                      m_playbackController->state() != PlaybackState::Playing;

    if (shouldSeek) {
        m_playbackEngine->seek(sourceTime);
        m_forceTimelineSeek = false;
        
        // When not playing, block briefly to grab a frame so the UI updates during scrub
        if (m_playbackController->state() != PlaybackState::Playing) {
            TimedFrame frame;
            for (int i = 0; i < 50; ++i) { // Wait up to 500ms
                QThread::msleep(10);
                frame = m_playbackEngine->nextFrame();
                if (!frame.image.isNull()) break;
            }
            if (!frame.image.isNull()) {
                QImage renderImage = frame.image.convertToFormat(QImage::Format_ARGB32);
                renderImage.detach();
                
                FitRecord rec;
                FitSession sesh;
                if (m_fitTrack && !m_fitTrack->isEmpty()) {
                    double absTime = m_timelineWidget->model()->relativeToAbsolute(currentTime);
                    rec = m_fitTrack->getRecordAtTime(absTime + m_timeSync->fitTimeOffset());
                    sesh = m_fitTrack->session();
                } else {
                    double loopTime = std::fmod(frame.pts, 10.0);
                    double progress = loopTime / 10.0;
                    rec.speed = (progress * 200.0f) / 3.6f;
                    rec.distance = (progress * 100.0f) * 1000.0f;
                    rec.heartRate = progress * 200.0f;
                    rec.hasHeartRate = true;
                    rec.altitude = -100.0f + progress * 10100.0f;
                    rec.grade = -100.0f + progress * 200.0f;
                    sesh.records.push_back(rec);
                }

                m_overlayRenderer->render(renderImage, rec, sesh);
                m_previewWidget->displayFrame(renderImage);
                m_lastFramePts = frame.pts;
            }
            m_previewWidget->setCurrentTime(currentTime);
            m_lastSourceTime = sourceTime;
            return;
        }
    }

    TimedFrame frame = m_playbackEngine->nextFrame();
    if (!frame.image.isNull()) {
        QImage renderImage = frame.image.convertToFormat(QImage::Format_ARGB32);
        renderImage.detach();
        
        FitRecord rec;
        FitSession sesh;
        if (m_fitTrack && !m_fitTrack->isEmpty()) {
            double absTime = m_timelineWidget->model()->relativeToAbsolute(currentTime);
            rec = m_fitTrack->getRecordAtTime(absTime + m_timeSync->fitTimeOffset());
            sesh = m_fitTrack->session();
        } else {
            double loopTime = std::fmod(frame.pts, 10.0);
            double progress = loopTime / 10.0;
            rec.speed = (progress * 200.0f) / 3.6f;
            rec.distance = (progress * 100.0f) * 1000.0f;
            rec.heartRate = progress * 200.0f;
            rec.hasHeartRate = true;
            rec.altitude = -100.0f + progress * 10100.0f;
            rec.grade = -100.0f + progress * 200.0f;
            sesh.records.push_back(rec);
        }

        m_overlayRenderer->render(renderImage, rec, sesh);
        m_previewWidget->displayFrame(renderImage);
        
        m_lastFramePts = frame.pts;
    }
    
    m_previewWidget->setCurrentTime(currentTime);
    m_lastSourceTime = sourceTime;
}

void MainWindow::onTimelineSeek(double relativeSeconds) {
    onTimelineScrub(relativeSeconds);
}

void MainWindow::onTimelineScrub(double relativeSeconds) {
    if (!m_playbackFromTimeline) {
        m_playbackFromTimeline = true;
        m_playbackController->stop();
        if (m_playbackEngine->isOpen()) m_playbackEngine->close();
    }
    
    double dur = m_timelineWidget->model()->duration();
    if (dur > 0) {
        m_playbackController->setDuration(dur);
        m_previewWidget->setDuration(dur);
    }
    
    m_playbackController->seek(relativeSeconds);
}

void MainWindow::onFitFileOpened(const QString& path) {
    statusBar()->showMessage(QString("Opening FIT file: %1").arg(path));
    // TODO: Phase 2 - parse FIT file
}

void MainWindow::onExportRequested() {
    // TODO: Phase 10 - export dialog
    QMessageBox::information(this, "Export", "Export functionality coming soon.");
}
