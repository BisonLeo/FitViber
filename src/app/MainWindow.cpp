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
#include "FitParser.h"
#include "TimeSync.h"
#include "VideoPlaybackEngine.h"
#include "OverlayPanelFactory.h"
#include "ProjectManager.h"

#include "MediaProbe.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QPainter>
#include <QDialog>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QtMath>
#include <QCloseEvent>
#include <QDir>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_overlayRenderer(std::make_unique<OverlayRenderer>())
    , m_previewFitTrack(std::make_unique<FitTrack>())
    , m_timeSync(std::make_unique<TimeSync>())
    , m_playbackEngine(std::make_unique<VideoPlaybackEngine>())
    , m_projectManager(std::make_unique<ProjectManager>())
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
    m_overlayRenderer->addPanel(OverlayPanelFactory::create(PanelType::MiniMap, this));

    // Set initial canvas size on preview
    m_previewWidget->setCanvasSize(m_canvasSize);

    // Connect project manager signals
    connect(m_projectManager.get(), &ProjectManager::autosaveTriggered,
            this, &MainWindow::onAutosaveTriggered);

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

    // Project operations
    auto* newProjectAction = fileMenu->addAction("&New Project");
    newProjectAction->setShortcut(QKeySequence::New);
    connect(newProjectAction, &QAction::triggered, this, &MainWindow::onNewProject);

    auto* openProjectAction = fileMenu->addAction("&Open Project...");
    openProjectAction->setShortcut(QKeySequence::Open);
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::onOpenProject);

    auto* saveProjectAction = fileMenu->addAction("&Save Project");
    saveProjectAction->setShortcut(QKeySequence::Save);
    connect(saveProjectAction, &QAction::triggered, this, &MainWindow::onSaveProject);

    auto* saveProjectAsAction = fileMenu->addAction("Save Project &As...");
    saveProjectAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveProjectAsAction, &QAction::triggered, this, &MainWindow::onSaveProjectAs);

    fileMenu->addSeparator();

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
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto* viewMenu = menuBar()->addMenu("&View");

    auto* canvasAction = viewMenu->addAction("Canvas &Settings...");
    connect(canvasAction, &QAction::triggered, this, &MainWindow::onCanvasSettings);

    viewMenu->addSeparator();

    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "About FitViber",
            QString("<h3>FitViber v%1</h3>"
                    "<p>GPS activity overlay video editor.</p>")
            .arg(AppConstants::AppVersion));
    });
}

void MainWindow::setupDockWidgets() {
    m_mediaDock = new QDockWidget("Media Browser", this);
    m_mediaDock->setWidget(m_mediaBrowser);
    m_mediaDock->setObjectName("MediaBrowserDock");
    addDockWidget(Qt::LeftDockWidgetArea, m_mediaDock);

    m_propertiesDock = new QDockWidget("Properties", this);
    m_propertiesDock->setWidget(m_propertiesPanel);
    m_propertiesDock->setObjectName("PropertiesDock");
    m_propertiesPanel->setMinimumWidth(220);
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);

    m_timelineDock = new QDockWidget("Timeline", this);
    m_timelineDock->setWidget(m_timelineWidget);
    m_timelineDock->setObjectName("TimelineDock");
    addDockWidget(Qt::BottomDockWidgetArea, m_timelineDock);

    // Add toggle actions to View menu
    QMenu* viewMenu = nullptr;
    for (auto* action : menuBar()->actions()) {
        if (action->text() == "&View") {
            viewMenu = action->menu();
            break;
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
    connect(m_mediaBrowser, &MediaBrowser::mediaImported, this, [this]() {
        m_projectModified = true;
    });

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

    connect(m_timelineWidget, &TimelineWidget::clipAdded, this, [this](const QString& path, double offset, double dur) {
        m_projectModified = true;
        if (QFileInfo(path).suffix().toLower() == "fit") {
            FitParser parser;
            if (parser.parse(path)) {
                auto fitTrack = std::make_unique<FitTrack>();
                fitTrack->loadSession(parser.session());
                m_fitTracks[path] = std::move(fitTrack);

                // For TimeSync offset logic, set based on the first FIT clip added
                if (m_fitTracks.size() == 1) {
                    m_timeSync->setFitTimeOffset(parser.session().startTime - offset);
                }
            }
        }
    });

    connect(m_timelineWidget, &TimelineWidget::clipMoved, this, [this](int trackIndex, int clipIndex, double newOffset) {
        m_projectModified = true;
        auto* track = m_timelineWidget->model()->track(trackIndex);
        if (track && track->type() == TrackType::FitData) {
            const Clip& clip = track->clips()[clipIndex];
            if (clipIndex == 0) {
                m_timeSync->setFitTimeOffset(clip.absoluteStartTime - newOffset);
            }
        }

        // Update playback controller bounds after any clip move
        double dur = m_timelineWidget->model()->duration();
        double minT = m_timelineWidget->model()->minTime();
        if (dur > minT) {
            m_playbackController->setStartTime(minT);
            m_playbackController->setDuration(dur);
            m_previewWidget->setStartTime(minT);
            m_previewWidget->setDuration(dur);
        }

        if (m_playbackFromTimeline && m_playbackController->state() != PlaybackState::Playing) {
            onPlaybackTick(m_timelineWidget->model()->playheadPosition());
        }
    });

    // Clip selection → transform handles
    connect(m_timelineWidget, &TimelineWidget::clipSelectionChanged,
            this, &MainWindow::onClipSelectionChanged);

    // Preview transform changed → re-render + update properties labels
    connect(m_previewWidget, &PreviewWidget::transformChanged, this, [this]() {
        m_propertiesPanel->updateTransformLabels();
        if (m_playbackFromTimeline) {
            onPlaybackTick(m_timelineWidget->model()->playheadPosition());
        }
    });

    // Properties transform changed → re-render + update preview
    connect(m_propertiesPanel, &PropertiesPanel::transformChanged, this, [this]() {
        m_previewWidget->update();
        if (m_playbackFromTimeline) {
            onPlaybackTick(m_timelineWidget->model()->playheadPosition());
        }
    });
}

void MainWindow::onCanvasSettings() {
    QDialog dlg(this);
    dlg.setWindowTitle("Canvas Settings");

    auto* layout = new QFormLayout(&dlg);

    auto* widthSpin = new QSpinBox(&dlg);
    widthSpin->setRange(320, 7680);
    widthSpin->setValue(m_canvasSize.width());
    widthSpin->setSuffix(" px");
    layout->addRow("Width:", widthSpin);

    auto* heightSpin = new QSpinBox(&dlg);
    heightSpin->setRange(240, 4320);
    heightSpin->setValue(m_canvasSize.height());
    heightSpin->setSuffix(" px");
    layout->addRow("Height:", heightSpin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        m_canvasSize = QSize(widthSpin->value(), heightSpin->value());
        m_previewWidget->setCanvasSize(m_canvasSize);
        m_projectModified = true;

        // Re-render current frame if in timeline mode
        if (m_playbackFromTimeline) {
            onPlaybackTick(m_timelineWidget->model()->playheadPosition());
        }

        statusBar()->showMessage(QString("Canvas size set to %1x%2")
            .arg(m_canvasSize.width()).arg(m_canvasSize.height()));
    }
}

void MainWindow::onMediaSelected(const QString& path) {
    QFileInfo info(path);
    QString suffix = info.suffix().toLower();

    // Check if it's an image
    QStringList imageExts = {"jpg", "jpeg", "png", "bmp", "tiff", "tif"};
    if (imageExts.contains(suffix)) {
        // Image mode
        m_playbackFromTimeline = false;  // must be before stop() to prevent timeline playhead reset
        m_playbackController->stop();
        m_playbackEngine->close();
        m_previewFitData = false;
        if (m_previewFitTrack) m_previewFitTrack->clear();

        QImage img(path);
        if (!img.isNull()) {
            m_previewWidget->setComposited(false);
            m_previewWidget->setSourceSize(img.size());
            m_previewWidget->showImage(img);
            statusBar()->showMessage(QString("Showing image: %1").arg(info.fileName()));
        } else {
            statusBar()->showMessage(QString("Failed to load image: %1").arg(info.fileName()));
        }
        return;
    }

    // Video mode
    m_playbackFromTimeline = false;  // must be before stop() to prevent timeline playhead reset
    m_playbackController->stop();
    m_playbackEngine->close();
    m_previewFitData = false;
    if (m_previewFitTrack) m_previewFitTrack->clear();

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

    m_previewWidget->setComposited(false);
    m_previewWidget->setSourceSize(QSize(vi.width, vi.height));
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
        if (m_previewFitData) {
            QImage renderImage(m_canvasSize, QImage::Format_ARGB32);
            renderImage.fill(Qt::black);

            renderOverlay(renderImage, currentTime);
            m_previewWidget->setComposited(true);
            m_previewWidget->displayFrame(renderImage);

            m_lastFramePts = currentTime;
            m_previewWidget->setCurrentTime(currentTime);
            return;
        }

        if (!m_playbackEngine->isOpen()) return;

        TimedFrame frame = m_playbackEngine->nextFrame();
        if (!frame.image.isNull()) {
            // Sync controller to actual frame PTS to prevent timer drift
            m_playbackController->syncTime(frame.pts);

            QImage renderImage = frame.image.convertToFormat(QImage::Format_ARGB32);
            renderImage.detach();

            renderOverlay(renderImage, frame.pts);
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
    double minT = m_timelineWidget->model()->minTime();
    if (dur > minT) {
        m_playbackController->setStartTime(minT);
        m_playbackController->setDuration(dur);
        m_previewWidget->setStartTime(minT);
        m_previewWidget->setDuration(dur);
    }

    // Find clip at currentTime
    auto* model = m_timelineWidget->model();
    const Clip* currentVisualClip = nullptr;
    const Clip* currentFitClip = nullptr;

    for (int ti = 0; ti < model->trackCount(); ++ti) {
        Track* track = model->track(ti);
        if (!track) continue;
        for (const auto& clip : track->clips()) {
            if (currentTime >= clip.timelineOffset && currentTime < clip.timelineOffset + clip.duration()) {
                if (track->type() == TrackType::FitData && !currentFitClip) {
                    currentFitClip = &clip;
                } else if ((track->type() == TrackType::Video) && !currentVisualClip) {
                    currentVisualClip = &clip;
                }
            }
        }
    }

    if (!currentVisualClip) {
        // No visual clip: render black canvas, but still apply overlay
        if (m_playbackEngine->isOpen()) {
            m_playbackEngine->close();
        }
        m_currentClipPath.clear();
        m_lastSourceTime = -1.0;

        QImage blackFrame(m_canvasSize, QImage::Format_ARGB32);
        blackFrame.fill(Qt::black);

        renderOverlay(blackFrame, currentTime);

        m_previewWidget->setComposited(true);
        m_previewWidget->setSourceSize(QSize());
        m_previewWidget->showVideo();
        m_previewWidget->displayFrame(blackFrame);
        m_previewWidget->setCurrentTime(currentTime);
        return;
    }

    // Helper lambda: compose a source frame onto the canvas with the clip's transform
    auto composeFrame = [this](const QImage& source, const ClipTransform& transform) -> QImage {
        QImage canvas(m_canvasSize, QImage::Format_ARGB32);
        canvas.fill(Qt::black);
        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QPointF center(canvas.width() / 2.0, canvas.height() / 2.0);
        painter.translate(center + QPointF(transform.panX, transform.panY));
        painter.rotate(transform.rotation);
        double sx = transform.flipH ? -transform.scale : transform.scale;
        double sy = transform.flipV ? -transform.scale : transform.scale;
        painter.scale(sx, sy);
        // Draw source centered
        painter.drawImage(QPointF(-source.width() / 2.0, -source.height() / 2.0), source);
        painter.end();

        return canvas;
    };

    if (currentVisualClip->type == ClipType::Image) {
        if (m_currentClipPath != currentVisualClip->sourcePath) {
            if (m_playbackEngine->isOpen()) m_playbackEngine->close();
            m_currentClipPath = currentVisualClip->sourcePath;
        }
        QImage img(m_currentClipPath);
        if (!img.isNull()) {
            img = img.convertToFormat(QImage::Format_ARGB32);
            QImage composited = composeFrame(img, currentVisualClip->transform);
            renderOverlay(composited, currentTime);
            m_previewWidget->setComposited(true);
            m_previewWidget->setSourceSize(img.size());
            m_previewWidget->showVideo();
            m_previewWidget->displayFrame(composited);
        }
        m_previewWidget->setCurrentTime(currentTime);
        m_lastSourceTime = -1.0;
        return;
    }

    // Video clip
    m_previewWidget->showVideo();
    double sourceTime = currentTime - currentVisualClip->timelineOffset + currentVisualClip->sourceIn;

    bool justOpened = false;
    if (m_currentClipPath != currentVisualClip->sourcePath || !m_playbackEngine->isOpen()) {
        m_playbackEngine->close();
        if (m_playbackEngine->open(currentVisualClip->sourcePath)) {
            m_currentClipPath = currentVisualClip->sourcePath;
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
        if (std::abs(sourceTime - m_lastSourceTime) > 0.5) {
            continuous = false;
        }
    }

    bool shouldSeek = justOpened || m_forceTimelineSeek || !continuous ||
                      m_playbackController->state() != PlaybackState::Playing;

    QSize srcSize(m_playbackEngine->info().width, m_playbackEngine->info().height);

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
                QImage composited = composeFrame(renderImage, currentVisualClip->transform);
                renderOverlay(composited, currentTime);
                m_previewWidget->setComposited(true);
                m_previewWidget->setSourceSize(srcSize);
                m_previewWidget->displayFrame(composited);
                m_lastFramePts = frame.pts;
            }
            m_previewWidget->setCurrentTime(currentTime);
            m_lastSourceTime = sourceTime;
            return;
        }
    }

    TimedFrame frame = m_playbackEngine->nextFrame();
    if (!frame.image.isNull()) {
        // Use actual frame PTS to derive the correct timeline time,
        // preventing drift between the synthetic timer and real video time.
        double actualTimelineTime = frame.pts - currentVisualClip->sourceIn + currentVisualClip->timelineOffset;
        m_playbackController->syncTime(actualTimelineTime);
        m_timelineWidget->model()->setPlayheadPosition(actualTimelineTime);

        QImage renderImage = frame.image.convertToFormat(QImage::Format_ARGB32);
        renderImage.detach();
        QImage composited = composeFrame(renderImage, currentVisualClip->transform);
        renderOverlay(composited, actualTimelineTime);
        m_previewWidget->setComposited(true);
        m_previewWidget->setSourceSize(srcSize);
        m_previewWidget->displayFrame(composited);
        m_lastFramePts = frame.pts;
        m_previewWidget->setCurrentTime(actualTimelineTime);
        m_lastSourceTime = frame.pts;
        return;
    }

    m_previewWidget->setCurrentTime(currentTime);
    m_lastSourceTime = sourceTime;
}

void MainWindow::onTimelineSeek(double relativeSeconds) {
    onTimelineScrub(relativeSeconds);
}

void MainWindow::renderOverlay(QImage& frame, double currentTime) {
    FitRecord rec;
    const FitSession* sessionToRender = nullptr;
    FitSession dummySesh; // Used only if hasFitData is false

    bool hasFitData = false;

    if (m_playbackFromTimeline) {
        // Find if there is a FitData clip at this time
        const Clip* currentFitClip = nullptr;
        for (int ti = 0; ti < m_timelineWidget->model()->trackCount(); ++ti) {
            Track* track = m_timelineWidget->model()->track(ti);
            if (track && track->type() == TrackType::FitData) {
                for (const auto& clip : track->clips()) {
                    if (currentTime >= clip.timelineOffset && currentTime < clip.timelineOffset + clip.duration()) {
                        currentFitClip = &clip;
                        break;
                    }
                }
            }
            if (currentFitClip) break;
        }

        if (currentFitClip) {
            auto it = m_fitTracks.find(currentFitClip->sourcePath);
            if (it != m_fitTracks.end() && it->second && !it->second->isEmpty()) {
                double sourceTime = currentTime - currentFitClip->timelineOffset + currentFitClip->absoluteStartTime;
                rec = it->second->getRecordAtTime(sourceTime);
                sessionToRender = &it->second->session();
                hasFitData = true;
            }
        }
    } else {
        if (m_previewFitTrack && !m_previewFitTrack->isEmpty()) {
            double searchTime = m_previewFitData ? currentTime + m_previewFitTrack->startTime() : currentTime + m_timeSync->fitTimeOffset();
            rec = m_previewFitTrack->getRecordAtTime(searchTime);
            sessionToRender = &m_previewFitTrack->session();
            hasFitData = true;
        }
    }

    if (!hasFitData) {
        return;
    }

    m_overlayRenderer->render(frame, rec, *sessionToRender);
}

void MainWindow::onTimelineScrub(double relativeSeconds) {
    if (!m_playbackFromTimeline) {
        m_playbackFromTimeline = true;
        m_playbackController->stop();
        if (m_playbackEngine->isOpen()) m_playbackEngine->close();
    }

    double dur = m_timelineWidget->model()->duration();
    double minT = m_timelineWidget->model()->minTime();
    if (dur > minT) {
        m_playbackController->setStartTime(minT);
        m_playbackController->setDuration(dur);
        m_previewWidget->setStartTime(minT);
        m_previewWidget->setDuration(dur);
    }

    m_playbackController->seek(relativeSeconds);
}

void MainWindow::onFitFileOpened(const QString& path) {
    statusBar()->showMessage(QString("Opening FIT file: %1").arg(path));

    FitParser parser;
    if (parser.parse(path)) {
        if (!m_previewFitTrack) {
            m_previewFitTrack = std::make_unique<FitTrack>();
        }
        m_previewFitTrack->loadSession(parser.session());
        statusBar()->showMessage(QString("Loaded FIT file: %1 (Records: %2)").arg(QFileInfo(path).fileName()).arg(m_previewFitTrack->records().size()));

        m_playbackFromTimeline = false;  // must be before stop() to prevent timeline playhead reset
        m_playbackController->stop();
        m_playbackEngine->close();

        m_previewFitData = true;
        m_currentClipPath = path;

        m_playbackController->setFps(30.0);
        m_playbackController->setDuration(m_previewFitTrack->duration());
        m_lastFramePts = 0.0;

        m_previewWidget->setComposited(true);
        m_previewWidget->setSourceSize(QSize());
        m_previewWidget->showVideo();
        m_previewWidget->setDuration(m_previewFitTrack->duration());
        // Force an update to show the first frame
        onPlaybackTick(0.0);
    } else {
        statusBar()->showMessage(QString("Failed to parse FIT file: %1").arg(parser.errorString()));
    }
}

void MainWindow::onClipSelectionChanged(int trackIndex, int clipIndex) {
    if (trackIndex < 0 || clipIndex < 0) {
        m_previewWidget->setClipTransform(nullptr);
        m_previewWidget->setHandlesVisible(false);
        m_propertiesPanel->setClipTransform(nullptr);
        m_propertiesPanel->clearClipInfo();
        statusBar()->clearMessage();
        return;
    }

    auto* track = m_timelineWidget->model()->track(trackIndex);
    if (!track || clipIndex >= track->clipCount()) {
        m_previewWidget->setClipTransform(nullptr);
        m_previewWidget->setHandlesVisible(false);
        m_propertiesPanel->setClipTransform(nullptr);
        m_propertiesPanel->clearClipInfo();
        statusBar()->clearMessage();
        return;
    }

    Clip& clip = track->clips()[clipIndex];
    if (clip.type == ClipType::Video || clip.type == ClipType::Image) {
        m_previewWidget->setClipTransform(&clip.transform);
        m_previewWidget->setHandlesVisible(true);
        m_propertiesPanel->setClipTransform(&clip.transform);
    } else {
        m_previewWidget->setClipTransform(nullptr);
        m_previewWidget->setHandlesVisible(false);
        m_propertiesPanel->setClipTransform(nullptr);
    }

    // Populate clip info
    ClipInfo info;
    info.path = clip.sourcePath;

    if (clip.type == ClipType::Video || clip.type == ClipType::Image) {
        info.type = (clip.type == ClipType::Video) ? "Video" : "Image";
        MediaProbe probe;
        if (probe.probe(clip.sourcePath)) {
            const auto& mi = probe.info();
            info.width = mi.videoWidth;
            info.height = mi.videoHeight;
            info.fps = mi.videoFps;
            info.totalSeconds = mi.duration;
            info.codec = mi.videoCodec;
            if (mi.videoFps > 0 && mi.duration > 0)
                info.totalFrames = static_cast<int>(mi.videoFps * mi.duration);
        }
        statusBar()->showMessage(QString("%1: %2x%3, %4 fps, %5s")
            .arg(clip.displayName).arg(info.width).arg(info.height)
            .arg(info.fps, 0, 'f', 1).arg(info.totalSeconds, 0, 'f', 1));
    } else if (clip.type == ClipType::FitData) {
        info.type = "FIT Data";
        auto it = m_fitTracks.find(clip.sourcePath);
        if (it != m_fitTracks.end() && it->second) {
            const auto& session = it->second->session();
            info.totalRecords = static_cast<int>(session.records.size());
            info.totalSeconds = session.totalElapsedTime;
            info.totalDistance = session.totalDistance;
            if (!session.records.empty()) {
                QDateTime dt = QDateTime::fromSecsSinceEpoch(
                    static_cast<qint64>(session.records.front().timestamp),
                    QTimeZone(8 * 3600));
                info.firstTimestamp = dt.toString("yyyy-MM-dd HH:mm:ss");
            }
        }
        double distKm = info.totalDistance / 1000.0;
        statusBar()->showMessage(QString("%1: %2 records, %3 km, %4s")
            .arg(clip.displayName).arg(info.totalRecords)
            .arg(distKm, 0, 'f', 1).arg(info.totalSeconds, 0, 'f', 0));
    }

    m_propertiesPanel->setClipInfo(info);
}

QImage MainWindow::applyTransform(const QImage& source, const ClipTransform& transform) {
    if (transform.isIdentity()) return source;

    QImage canvas(m_canvasSize, QImage::Format_ARGB32);
    canvas.fill(Qt::black);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPointF center(canvas.width() / 2.0, canvas.height() / 2.0);
    painter.translate(center + QPointF(transform.panX, transform.panY));
    painter.rotate(transform.rotation);
    double sx = transform.flipH ? -transform.scale : transform.scale;
    double sy = transform.flipV ? -transform.scale : transform.scale;
    painter.scale(sx, sy);
    painter.drawImage(QPointF(-source.width() / 2.0, -source.height() / 2.0), source);
    painter.end();

    return canvas;
}

void MainWindow::onExportRequested() {
    // TODO: Phase 10 - export dialog
    QMessageBox::information(this, "Export", "Export functionality coming soon.");
}

// ============================================================================
// Project Management
// ============================================================================

bool MainWindow::maybeSaveModified()
{
    if (!m_projectModified) return true;

    auto reply = QMessageBox::question(this, "Unsaved Changes",
        "Current project has unsaved changes. Save before proceeding?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (reply == QMessageBox::Save) {
        onSaveProject();
        return !m_projectModified; // false if save failed/cancelled
    }
    return reply != QMessageBox::Cancel;
}

void MainWindow::onNewProject()
{
    if (!maybeSaveModified()) return;

    // Clear timeline
    auto* model = m_timelineWidget->model();
    while (model->trackCount() > 0) {
        model->removeTrack(0);
    }

    // Clear media browser
    m_mediaBrowser->clearMedia();

    // Clear FIT tracks
    m_fitTracks.clear();
    m_previewFitTrack->clear();

    // Reset settings
    m_canvasSize = QSize(1920, 1080);
    m_previewWidget->setCanvasSize(m_canvasSize);
    model->setZoom(1.0);
    model->setScrollOffset(0.0);
    model->setTimeOrigin(0.0);
    model->setPlayheadPosition(0.0);
    m_timeSync->setFitTimeOffset(0.0);

    // Clear overlay panels and re-add defaults
    m_overlayRenderer->clearPanels();
    m_overlayRenderer->addPanel(OverlayPanelFactory::create(PanelType::HeartRate, this));
    m_overlayRenderer->addPanel(OverlayPanelFactory::create(PanelType::Distance, this));
    m_overlayRenderer->addPanel(OverlayPanelFactory::create(PanelType::Elevation, this));
    m_overlayRenderer->addPanel(OverlayPanelFactory::create(PanelType::Inclination, this));
    m_overlayRenderer->addPanel(OverlayPanelFactory::create(PanelType::Speed, this));
    m_overlayRenderer->addPanel(OverlayPanelFactory::create(PanelType::MiniMap, this));

    // Stop playback
    m_playbackController->stop();
    m_playbackEngine->close();
    m_playbackFromTimeline = false;
    m_previewFitData = false;

    // Reset project state
    m_currentProjectPath.clear();
    m_projectModified = false;
    setWindowTitle(QString("%1 v%2 - [Untitled]").arg(AppConstants::AppName, AppConstants::AppVersion));
    
    m_projectManager->stopAutosave();
    
    statusBar()->showMessage("New project created");
}

void MainWindow::onOpenProject()
{
    if (!maybeSaveModified()) return;

    QString filePath = QFileDialog::getOpenFileName(this, "Open Project", {},
        "FitViber Projects (*.fvProj);;All Files (*)");
    
    if (filePath.isEmpty()) {
        return;
    }

    // Check for autosave files
    if (m_projectManager->hasAutosaveFiles(filePath)) {
        QString autosavePath = m_projectManager->getLatestAutosaveFile(filePath);
        auto reply = QMessageBox::question(this, "Autosave Found",
            QString("An autosave file was found for this project:\n%1\n\nWould you like to restore from the autosave file?").arg(autosavePath),
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            filePath = autosavePath;
        }
    }

    ProjectSettings settings;
    QStringList importedMedia;
    QString originalPath;
    if (m_projectManager->loadProject(filePath, settings, m_timelineWidget->model(), 
                                      m_overlayRenderer.get(), m_timeSync.get(), importedMedia, &originalPath)) {
        
        // Apply loaded settings
        applyProjectSettings(settings);
        
        // Restore imported media files to browser
        m_mediaBrowser->clearMedia();
        for (const QString& mediaPath : importedMedia) {
            if (QFile::exists(mediaPath)) {
                m_mediaBrowser->addMediaFile(mediaPath);
            }
        }
        
        // Load FIT data for all FitData clips
        loadFitDataForClips();
        
        // Update UI - use original path from project file (works for both regular and autosave)
        m_currentProjectPath = originalPath;
        QFileInfo info(m_currentProjectPath);
        
        if (!filePath.endsWith(".autosave")) {
            setWindowTitle(QString("%1 v%2 - %3").arg(AppConstants::AppName, AppConstants::AppVersion, info.fileName()));
            m_projectModified = false;
        } else {
            setWindowTitle(QString("%1 v%2 - %3 [Recovered]").arg(AppConstants::AppName, AppConstants::AppVersion, info.fileName()));
            m_projectModified = true;
        }
        
        // Start autosave
        updateProjectSettings();
        m_projectManager->startAutosave(m_currentProjectPath, collectProjectSettings(),
                                        m_timelineWidget->model(), m_overlayRenderer.get(),
                                        m_mediaBrowser->getImportedMediaPaths());
        
        statusBar()->showMessage(QString("Project loaded: %1").arg(filePath));
    } else {
        QMessageBox::critical(this, "Error", 
            QString("Failed to load project:\n%1").arg(m_projectManager->errorString()));
    }
}

void MainWindow::onSaveProject()
{
    if (m_currentProjectPath.isEmpty()) {
        onSaveProjectAs();
        return;
    }

    updateProjectSettings();
    if (m_projectManager->saveProject(m_currentProjectPath, collectProjectSettings(),
                                      m_timelineWidget->model(), m_overlayRenderer.get(),
                                      m_mediaBrowser->getImportedMediaPaths())) {
        m_projectModified = false;
        QFileInfo info(m_currentProjectPath);
        setWindowTitle(QString("%1 v%2 - %3").arg(AppConstants::AppName, AppConstants::AppVersion, info.fileName()));
        statusBar()->showMessage(QString("Project saved: %1").arg(m_currentProjectPath));
        
        // Restart autosave with current settings
        m_projectManager->startAutosave(m_currentProjectPath, collectProjectSettings(),
                                        m_timelineWidget->model(), m_overlayRenderer.get(),
                                        m_mediaBrowser->getImportedMediaPaths());
    } else {
        QMessageBox::critical(this, "Error",
            QString("Failed to save project:\n%1").arg(m_projectManager->errorString()));
    }
}

void MainWindow::onSaveProjectAs()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Project As", {},
        "FitViber Projects (*.fvProj);;All Files (*)");
    
    if (filePath.isEmpty()) {
        return;
    }

    // Ensure proper extension
    if (!filePath.endsWith(".fvProj", Qt::CaseInsensitive)) {
        filePath += ".fvProj";
    }

    m_currentProjectPath = filePath;
    onSaveProject();
}

void MainWindow::onAutosaveTriggered(const QString& autosavePath)
{
    statusBar()->showMessage(QString("Autosaved: %1").arg(QFileInfo(autosavePath).fileName()), 3000);
}

ProjectSettings MainWindow::collectProjectSettings() const
{
    ProjectSettings settings;
    settings.canvasSize = m_canvasSize;
    settings.timelineZoom = m_timelineWidget->model()->zoom();
    settings.timelineScrollOffset = m_timelineWidget->model()->scrollOffset();
    settings.timeOrigin = m_timelineWidget->model()->timeOrigin();
    settings.playheadPosition = m_timelineWidget->model()->playheadPosition();
    settings.fitTimeOffset = m_timeSync->fitTimeOffset();
    return settings;
}

void MainWindow::applyProjectSettings(const ProjectSettings& settings)
{
    m_canvasSize = settings.canvasSize;
    m_previewWidget->setCanvasSize(m_canvasSize);
    m_timelineWidget->model()->setZoom(settings.timelineZoom);
    m_timelineWidget->model()->setScrollOffset(settings.timelineScrollOffset);
    m_timelineWidget->model()->setTimeOrigin(settings.timeOrigin);
    m_timelineWidget->model()->setPlayheadPosition(settings.playheadPosition);
    m_timeSync->setFitTimeOffset(settings.fitTimeOffset);
}

void MainWindow::updateProjectSettings()
{
    // Update the settings used for autosave
    if (!m_currentProjectPath.isEmpty()) {
        m_projectManager->stopAutosave();
    }
}

void MainWindow::loadFitDataForClips()
{
    m_fitTracks.clear();
    
    auto* model = m_timelineWidget->model();
    for (int ti = 0; ti < model->trackCount(); ++ti) {
        Track* track = model->track(ti);
        if (!track || track->type() != TrackType::FitData) continue;
        
        for (int ci = 0; ci < track->clipCount(); ++ci) {
            const Clip& clip = track->clip(ci);
            if (clip.type == ClipType::FitData) {
                FitParser parser;
                if (parser.parse(clip.sourcePath)) {
                    auto fitTrack = std::make_unique<FitTrack>();
                    fitTrack->loadSession(parser.session());
                    m_fitTracks[clip.sourcePath] = std::move(fitTrack);
                }
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (!maybeSaveModified()) {
        event->ignore();
        return;
    }

    m_projectManager->stopAutosave();
    event->accept();
}
