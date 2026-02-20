#include "MainWindow.h"
#include "AppConstants.h"
#include "MediaBrowser.h"
#include "PreviewWidget.h"
#include "PropertiesPanel.h"
#include "TimelineWidget.h"
#include "PlaybackController.h"
#include "OverlayRenderer.h"
#include "FitTrack.h"
#include "TimeSync.h"
#include "VideoPlaybackEngine.h"

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
        if (m_playbackEngine->isOpen()) {
            m_playbackEngine->seek(seconds);
        }
    });
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

void MainWindow::onPlaybackTick(double /*currentTime*/) {
    if (!m_playbackEngine->isOpen()) return;

    // Pull pre-decoded frame from queue (no seeking — sequential decode)
    TimedFrame frame = m_playbackEngine->nextFrame();
    if (!frame.image.isNull()) {
        m_previewWidget->displayFrame(frame.image);
        m_lastFramePts = frame.pts;
        m_previewWidget->setCurrentTime(frame.pts);
    } else if (m_playbackEngine->isFinished()) {
        // All frames consumed and decoder hit EOF — stop playback.
        // Use the last frame PTS as the true video duration.
        m_playbackController->pause();
        m_previewWidget->setDuration(m_lastFramePts);
        m_previewWidget->setCurrentTime(m_lastFramePts);
    }
    // When no frame is ready but not EOF, keep the last displayed time.
}

void MainWindow::onFitFileOpened(const QString& path) {
    statusBar()->showMessage(QString("Opening FIT file: %1").arg(path));
    // TODO: Phase 2 - parse FIT file
}

void MainWindow::onExportRequested() {
    // TODO: Phase 10 - export dialog
    QMessageBox::information(this, "Export", "Export functionality coming soon.");
}
