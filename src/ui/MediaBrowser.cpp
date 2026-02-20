#include "MediaBrowser.h"
#include "VideoDecoder.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMouseEvent>
#include <QPainter>
#include <QMimeData>
#include <QDrag>
#include <QUrl>

void MediaBrowserListWidget::startDrag(Qt::DropActions /*supportedActions*/) {
    QListWidgetItem* item = currentItem();
    if (!item) return;

    QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty()) return;

    auto* mimeData = new QMimeData();
    mimeData->setUrls({QUrl::fromLocalFile(path)});

    auto* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(item->icon().pixmap(64, 48));
    drag->exec(Qt::CopyAction);
}

MediaBrowser::MediaBrowser(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    m_importButton = new QPushButton("Import Media...", this);
    layout->addWidget(m_importButton);

    m_listWidget = new MediaBrowserListWidget(this);
    m_listWidget->setViewMode(QListWidget::IconMode);
    m_listWidget->setIconSize(QSize(ThumbWidth, ThumbHeight));
    m_listWidget->setGridSize(QSize(ThumbWidth + 20, ThumbHeight + 40));
    m_listWidget->setResizeMode(QListWidget::Adjust);
    m_listWidget->setWrapping(true);
    m_listWidget->setSpacing(8);
    m_listWidget->setMovement(QListWidget::Static);
    m_listWidget->setDragEnabled(true);
    m_listWidget->setDragDropMode(QAbstractItemView::DragOnly);
    m_listWidget->viewport()->setMouseTracking(true);
    m_listWidget->viewport()->installEventFilter(this);
    layout->addWidget(m_listWidget);

    connect(m_importButton, &QPushButton::clicked, this, &MediaBrowser::onImportClicked);
    connect(m_listWidget, &QListWidget::itemClicked, this, &MediaBrowser::onItemClicked);
}

MediaBrowser::~MediaBrowser() = default;

MediaType MediaBrowser::classifyFile(const QString& suffix) const {
    QString s = suffix.toLower();
    if (s == "fit") return MediaType::FIT;
    if (s == "jpg" || s == "jpeg" || s == "png" || s == "bmp" || s == "tiff" || s == "tif")
        return MediaType::Image;
    return MediaType::Video;
}

QPixmap MediaBrowser::generateVideoThumbnail(const QString& path) {
    VideoDecoder decoder;
    if (decoder.open(path)) {
        QImage frame = decoder.decodeNextFrame();
        decoder.close();
        if (!frame.isNull()) {
            return QPixmap::fromImage(frame).scaled(
                ThumbWidth, ThumbHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
    }
    // Fallback: colored placeholder
    QPixmap pm(ThumbWidth, ThumbHeight);
    pm.fill(QColor(40, 60, 80));
    QPainter p(&pm);
    p.setPen(QColor(100, 180, 255));
    p.drawText(pm.rect(), Qt::AlignCenter, "Video");
    return pm;
}

QPixmap MediaBrowser::generateImageThumbnail(const QString& path) {
    QImage img(path);
    if (!img.isNull()) {
        return QPixmap::fromImage(img).scaled(
            ThumbWidth, ThumbHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    QPixmap pm(ThumbWidth, ThumbHeight);
    pm.fill(QColor(40, 80, 60));
    QPainter p(&pm);
    p.setPen(QColor(100, 255, 180));
    p.drawText(pm.rect(), Qt::AlignCenter, "Image");
    return pm;
}

QPixmap MediaBrowser::generateFitThumbnail(const QString& /*path*/) {
    QPixmap pm(ThumbWidth, ThumbHeight);
    pm.fill(QColor(60, 50, 30));
    QPainter p(&pm);
    p.setPen(QColor(255, 180, 60));
    QFont f = p.font();
    f.setPointSize(14);
    f.setBold(true);
    p.setFont(f);
    p.drawText(pm.rect(), Qt::AlignCenter, "FIT");
    return pm;
}

void MediaBrowser::onImportClicked() {
    QStringList paths = QFileDialog::getOpenFileNames(this, "Import Media", {},
        "All Supported (*.mp4 *.avi *.mkv *.mov *.wmv *.fit *.jpg *.jpeg *.png *.bmp *.tiff *.tif);;"
        "Video Files (*.mp4 *.avi *.mkv *.mov *.wmv);;"
        "Image Files (*.jpg *.jpeg *.png *.bmp *.tiff *.tif);;"
        "FIT Files (*.fit);;"
        "All Files (*)");

    for (const auto& path : paths) {
        if (m_mediaPaths.contains(path)) continue;
        m_mediaPaths.append(path);

        QFileInfo info(path);
        MediaType type = classifyFile(info.suffix());

        QPixmap thumb;
        switch (type) {
            case MediaType::Video: thumb = generateVideoThumbnail(path); break;
            case MediaType::Image: thumb = generateImageThumbnail(path); break;
            case MediaType::FIT:   thumb = generateFitThumbnail(path); break;
        }

        auto* item = new QListWidgetItem(QIcon(thumb), info.fileName());
        item->setData(UserRolePath, path);
        item->setData(UserRoleType, static_cast<int>(type));
        item->setToolTip(path);
        item->setSizeHint(QSize(ThumbWidth + 20, ThumbHeight + 40));

        // Store video duration for hover scrub
        if (type == MediaType::Video) {
            VideoDecoder decoder;
            if (decoder.open(path)) {
                item->setData(UserRoleDuration, decoder.info().duration);
                decoder.close();
            }
        }

        m_listWidget->addItem(item);
    }
}

void MediaBrowser::onItemClicked(QListWidgetItem* item) {
    QString path = item->data(UserRolePath).toString();
    auto type = static_cast<MediaType>(item->data(UserRoleType).toInt());

    if (type == MediaType::FIT) {
        emit fitFileSelected(path);
    } else {
        emit mediaSelected(path);
    }
}

bool MediaBrowser::eventFilter(QObject* obj, QEvent* event) {
    if (obj != m_listWidget->viewport()) return QWidget::eventFilter(obj, event);

    if (event->type() == QEvent::MouseMove) {
        auto* me = static_cast<QMouseEvent*>(event);
        QPoint viewportPos = me->pos();
        QListWidgetItem* item = m_listWidget->itemAt(viewportPos);

        // If we moved off the previous hovered item, restore its icon
        if (m_hoveredItem && m_hoveredItem != item) {
            m_hoveredItem->setIcon(m_hoveredOriginalIcon);
            m_hoveredItem = nullptr;
            if (m_scrubDecoder) {
                m_scrubDecoder->close();
                m_scrubDecoder.reset();
            }
        }

        if (!item) return false;

        auto type = static_cast<MediaType>(item->data(UserRoleType).toInt());
        if (type != MediaType::Video) return false;

        // Starting hover on a new video item
        if (item != m_hoveredItem) {
            m_hoveredItem = item;
            m_hoveredOriginalIcon = item->icon();
            m_scrubDecoder = std::make_unique<VideoDecoder>();
            QString path = item->data(UserRolePath).toString();
            if (!m_scrubDecoder->open(path)) {
                m_scrubDecoder.reset();
                m_hoveredItem = nullptr;
                return false;
            }
        }

        if (!m_scrubDecoder || !m_scrubDecoder->isOpen()) return false;

        // Calculate scrub position from mouse X relative to item rect
        // visualItemRect returns viewport coordinates for visible items
        QRect itemRect = m_listWidget->visualItemRect(item);
        double fraction = qBound(0.0,
            static_cast<double>(viewportPos.x() - itemRect.left()) / itemRect.width(), 1.0);
        double duration = item->data(UserRoleDuration).toDouble();
        double seekTime = fraction * duration;

        if (m_scrubDecoder->seek(seekTime)) {
            QImage frame = m_scrubDecoder->decodeNextFrame();
            if (!frame.isNull()) {
                QPixmap thumb = QPixmap::fromImage(frame).scaled(
                    ThumbWidth, ThumbHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                item->setIcon(QIcon(thumb));
            }
        }
        return false;
    }

    if (event->type() == QEvent::Leave) {
        if (m_hoveredItem) {
            m_hoveredItem->setIcon(m_hoveredOriginalIcon);
            m_hoveredItem = nullptr;
            if (m_scrubDecoder) {
                m_scrubDecoder->close();
                m_scrubDecoder.reset();
            }
        }
        return false;
    }

    return QWidget::eventFilter(obj, event);
}
