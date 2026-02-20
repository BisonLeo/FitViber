#include "MediaBrowser.h"
#include <QFileDialog>
#include <QFileInfo>

MediaBrowser::MediaBrowser(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    m_importButton = new QPushButton("Import Media...", this);
    layout->addWidget(m_importButton);

    m_listWidget = new QListWidget(this);
    m_listWidget->setDragEnabled(true);
    layout->addWidget(m_listWidget);

    connect(m_importButton, &QPushButton::clicked, this, &MediaBrowser::onImportClicked);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &MediaBrowser::onItemDoubleClicked);
}

MediaBrowser::~MediaBrowser() = default;

void MediaBrowser::onImportClicked() {
    QStringList paths = QFileDialog::getOpenFileNames(this, "Import Media", {},
        "All Supported (*.mp4 *.avi *.mkv *.mov *.wmv *.fit);;"
        "Video Files (*.mp4 *.avi *.mkv *.mov *.wmv);;"
        "FIT Files (*.fit);;"
        "All Files (*)");

    for (const auto& path : paths) {
        if (m_mediaPaths.contains(path)) continue;
        m_mediaPaths.append(path);

        QFileInfo info(path);
        auto* item = new QListWidgetItem(info.fileName());
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);

        if (info.suffix().toLower() == "fit") {
            item->setForeground(QColor(255, 180, 60));
        } else {
            item->setForeground(QColor(100, 180, 255));
        }

        m_listWidget->addItem(item);
    }
}

void MediaBrowser::onItemDoubleClicked(QListWidgetItem* item) {
    QString path = item->data(Qt::UserRole).toString();
    QFileInfo info(path);

    if (info.suffix().toLower() == "fit") {
        emit fitFileSelected(path);
    } else {
        emit videoFileSelected(path);
    }
}
