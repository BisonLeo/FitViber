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
#include "VideoDecoder.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_overlayRenderer(std::make_unique<OverlayRenderer>())
    , m_fitTrack(std::make_unique<FitTrack>())
    , m_timeSync(std::make_unique<TimeSync>())
    , m_videoDecoder(std::make_unique<VideoDecoder>())
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
        if (!path.isEmpty()) onVideoFileOpened(path);
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

    // Add dock toggle actions to View menu after docks are created
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
    connect(m_mediaBrowser, &MediaBrowser::videoFileSelected,
            this, &MainWindow::onVideoFileOpened);
    connect(m_mediaBrowser, &MediaBrowser::fitFileSelected,
            this, &MainWindow::onFitFileOpened);
}

void MainWindow::onVideoFileOpened(const QString& path) {
    statusBar()->showMessage(QString("Opening video: %1").arg(path));
    // TODO: Phase 3 - open video with VideoDecoder
}

void MainWindow::onFitFileOpened(const QString& path) {
    statusBar()->showMessage(QString("Opening FIT file: %1").arg(path));
    // TODO: Phase 2 - parse FIT file
}

void MainWindow::onExportRequested() {
    // TODO: Phase 10 - export dialog
    QMessageBox::information(this, "Export", "Export functionality coming soon.");
}
