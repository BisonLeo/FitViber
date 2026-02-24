#include "MediaBrowser.h"
#include "VideoDecoder.h"
#include "FitParser.h"
#include "FitData.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QMimeData>
#include <QDrag>
#include <QUrl>
#include <cmath>
#include <algorithm>

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

void MediaBrowser::drawInfoText(QPainter& painter, const QRect& rect,
                                 const QString& text, Qt::Alignment align) {
    // Semi-transparent background behind text for readability
    QFont font("Arial", 7);
    painter.setFont(font);
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(rect, static_cast<int>(align) | Qt::TextWordWrap, text);
    textRect.adjust(-3, -1, 3, 1);

    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 160));
    painter.drawRect(textRect);
    painter.setPen(QColor(220, 220, 220));
    painter.setFont(font);
    painter.drawText(rect, static_cast<int>(align) | Qt::TextWordWrap, text);
    painter.restore();
}

QPixmap MediaBrowser::generateVideoThumbnail(const QString& path) {
    VideoDecoder decoder;
    QPixmap thumb;

    if (decoder.open(path)) {
        QImage frame = decoder.decodeNextFrame();
        VideoInfo vi = decoder.info();
        decoder.close();

        if (!frame.isNull()) {
            thumb = QPixmap::fromImage(frame).scaled(
                ThumbWidth, ThumbHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else {
            thumb = QPixmap(ThumbWidth, ThumbHeight);
            thumb.fill(QColor(40, 60, 80));
        }

        // Draw info overlay
        QPainter p(&thumb);
        QString sizeStr = QString("%1x%2").arg(vi.width).arg(vi.height);

        // Duration
        int totalSec = static_cast<int>(vi.duration);
        int h = totalSec / 3600;
        int m = (totalSec % 3600) / 60;
        int s = totalSec % 60;
        QString durStr;
        if (h > 0) durStr = QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        else durStr = QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));

        QString info = sizeStr + "  " + durStr;
        drawInfoText(p, thumb.rect().adjusted(2, 0, -2, -2),
                     info, Qt::AlignBottom | Qt::AlignRight);
        return thumb;
    }

    // Fallback
    thumb = QPixmap(ThumbWidth, ThumbHeight);
    thumb.fill(QColor(40, 60, 80));
    QPainter p(&thumb);
    p.setPen(QColor(100, 180, 255));
    p.drawText(thumb.rect(), Qt::AlignCenter, "Video");
    return thumb;
}

QPixmap MediaBrowser::generateImageThumbnail(const QString& path) {
    QImage img(path);
    QPixmap thumb;

    if (!img.isNull()) {
        thumb = QPixmap::fromImage(img).scaled(
            ThumbWidth, ThumbHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QPainter p(&thumb);
        // Format string
        QString fmtStr;
        switch (img.format()) {
            case QImage::Format_RGB32:       fmtStr = "RGB32"; break;
            case QImage::Format_ARGB32:      fmtStr = "ARGB32"; break;
            case QImage::Format_RGB888:      fmtStr = "RGB24"; break;
            case QImage::Format_Grayscale8:  fmtStr = "Gray8"; break;
            case QImage::Format_Grayscale16: fmtStr = "Gray16"; break;
            case QImage::Format_RGBA8888:    fmtStr = "RGBA"; break;
            default: fmtStr = QString("fmt%1").arg(static_cast<int>(img.format())); break;
        }

        QString info = QString("%1x%2 %3").arg(img.width()).arg(img.height()).arg(fmtStr);
        drawInfoText(p, thumb.rect().adjusted(2, 0, -2, -2),
                     info, Qt::AlignBottom | Qt::AlignRight);
        return thumb;
    }

    thumb = QPixmap(ThumbWidth, ThumbHeight);
    thumb.fill(QColor(40, 80, 60));
    QPainter p(&thumb);
    p.setPen(QColor(100, 255, 180));
    p.drawText(thumb.rect(), Qt::AlignCenter, "Image");
    return thumb;
}

QPixmap MediaBrowser::generateFitThumbnail(const QString& path) {
    QPixmap pm(ThumbWidth, ThumbHeight);
    pm.fill(QColor(30, 30, 32));

    FitParser parser;
    if (!parser.parse(path)) {
        QPainter p(&pm);
        p.setPen(QColor(255, 180, 60));
        QFont f = p.font();
        f.setPointSize(14);
        f.setBold(true);
        p.setFont(f);
        p.drawText(pm.rect(), Qt::AlignCenter, "FIT");
        return pm;
    }

    const FitSession& session = parser.session();
    const auto& records = session.records;
    if (records.empty()) {
        QPainter p(&pm);
        p.setPen(QColor(255, 180, 60));
        p.drawText(pm.rect(), Qt::AlignCenter, "FIT (empty)");
        return pm;
    }

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    // Layout: top 2/3 = minimap, bottom 1/3 = elevation profile
    int mapHeight = ThumbHeight * 2 / 3;   // 80px
    int elevHeight = ThumbHeight - mapHeight; // 40px
    QRect mapRect(0, 0, ThumbWidth, mapHeight);
    QRect elevRect(0, mapHeight, ThumbWidth, elevHeight);

    // --- Minimap (top 2/3) ---
    p.fillRect(mapRect, QColor(25, 35, 25));

    // Collect GPS points
    std::vector<std::pair<double, double>> gpsPoints;
    for (const auto& r : records) {
        if (r.hasGps && r.latitude != 0.0 && r.longitude != 0.0) {
            gpsPoints.emplace_back(r.latitude, r.longitude);
        }
    }

    if (gpsPoints.size() >= 2) {
        double minLat = gpsPoints[0].first, maxLat = minLat;
        double minLon = gpsPoints[0].second, maxLon = minLon;
        for (const auto& pt : gpsPoints) {
            if (pt.first < minLat) minLat = pt.first;
            if (pt.first > maxLat) maxLat = pt.first;
            if (pt.second < minLon) minLon = pt.second;
            if (pt.second > maxLon) maxLon = pt.second;
        }

        double latRange = maxLat - minLat;
        double lonRange = maxLon - minLon;
        if (latRange < 1e-6) latRange = 1e-6;
        if (lonRange < 1e-6) lonRange = 1e-6;

        // Fit within mapRect with padding
        int pad = 4;
        int drawW = mapRect.width() - pad * 2;
        int drawH = mapRect.height() - pad * 2;

        double scaleX = drawW / lonRange;
        double scaleY = drawH / latRange;
        double scale = std::min(scaleX, scaleY);

        double cxOff = mapRect.left() + pad + (drawW - lonRange * scale) / 2.0;
        double cyOff = mapRect.top() + pad + (drawH - latRange * scale) / 2.0;

        QPainterPath path;
        for (size_t i = 0; i < gpsPoints.size(); ++i) {
            double x = cxOff + (gpsPoints[i].second - minLon) * scale;
            double y = cyOff + (maxLat - gpsPoints[i].first) * scale; // flip Y
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }

        p.setPen(QPen(QColor(80, 200, 120), 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        // Start dot (green) and end dot (red)
        double sx = cxOff + (gpsPoints.front().second - minLon) * scale;
        double sy = cyOff + (maxLat - gpsPoints.front().first) * scale;
        double ex = cxOff + (gpsPoints.back().second - minLon) * scale;
        double ey = cyOff + (maxLat - gpsPoints.back().first) * scale;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(50, 220, 80));
        p.drawEllipse(QPointF(sx, sy), 3, 3);
        p.setBrush(QColor(220, 60, 50));
        p.drawEllipse(QPointF(ex, ey), 3, 3);
    } else {
        p.setPen(QColor(100, 100, 100));
        QFont f("Arial", 7);
        p.setFont(f);
        p.drawText(mapRect, Qt::AlignCenter, "No GPS");
    }

    // --- Info text overlay on minimap ---
    {
        // Total distance
        double totalDistKm = session.totalDistance / 1000.0;
        QString distStr;
        if (totalDistKm >= 1.0)
            distStr = QString("%1 km").arg(totalDistKm, 0, 'f', 1);
        else
            distStr = QString("%1 m").arg(static_cast<int>(session.totalDistance));

        // Duration in hh:mm:ss
        int totalSec = static_cast<int>(session.totalElapsedTime);
        int h = totalSec / 3600;
        int m = (totalSec % 3600) / 60;
        int s = totalSec % 60;
        QString durStr;
        if (h > 0) durStr = QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        else durStr = QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));

        QString info = distStr + "  " + durStr;
        drawInfoText(p, mapRect.adjusted(2, 0, -2, -2),
                     info, Qt::AlignBottom | Qt::AlignRight);
    }

    // Separator line
    p.setPen(QColor(60, 60, 60));
    p.drawLine(0, mapHeight, ThumbWidth, mapHeight);

    // --- Elevation profile (bottom 1/3) ---
    p.fillRect(elevRect, QColor(30, 25, 20));

    std::vector<float> elevations;
    for (const auto& r : records) {
        if (r.altitude != 0.0f || elevations.size() > 0) {
            elevations.push_back(r.altitude);
        }
    }
    // If no altitude data at all, fill from all records
    if (elevations.empty()) {
        for (const auto& r : records) {
            elevations.push_back(r.altitude);
        }
    }

    if (elevations.size() >= 2) {
        float minElev = *std::min_element(elevations.begin(), elevations.end());
        float maxElev = *std::max_element(elevations.begin(), elevations.end());
        float elevRange = maxElev - minElev;
        if (elevRange < 1.0f) elevRange = 1.0f;

        int pad = 2;
        int drawW = elevRect.width() - pad * 2;
        int drawH = elevRect.height() - pad * 2;

        // Build filled polygon for elevation
        QPainterPath elevPath;
        int n = static_cast<int>(elevations.size());

        // Downsample if too many points
        int step = std::max(1, n / drawW);

        double baseY = elevRect.bottom() - pad;
        bool first = true;
        double lastX = 0;
        for (int i = 0; i < n; i += step) {
            double x = elevRect.left() + pad + (static_cast<double>(i) / (n - 1)) * drawW;
            double y = baseY - ((elevations[i] - minElev) / elevRange) * drawH;
            if (first) {
                elevPath.moveTo(x, baseY);
                elevPath.lineTo(x, y);
                first = false;
            } else {
                elevPath.lineTo(x, y);
            }
            lastX = x;
        }
        elevPath.lineTo(lastX, baseY);
        elevPath.closeSubpath();

        // Gradient fill
        QLinearGradient grad(0, elevRect.top(), 0, elevRect.bottom());
        grad.setColorAt(0.0, QColor(180, 120, 60, 180));
        grad.setColorAt(1.0, QColor(80, 50, 20, 100));
        p.setPen(Qt::NoPen);
        p.setBrush(grad);
        p.drawPath(elevPath);

        // Outline
        p.setPen(QPen(QColor(200, 140, 70), 1.0));
        p.setBrush(Qt::NoBrush);

        QPainterPath outlinePath;
        first = true;
        for (int i = 0; i < n; i += step) {
            double x = elevRect.left() + pad + (static_cast<double>(i) / (n - 1)) * drawW;
            double y = baseY - ((elevations[i] - minElev) / elevRange) * drawH;
            if (first) { outlinePath.moveTo(x, y); first = false; }
            else outlinePath.lineTo(x, y);
        }
        p.drawPath(outlinePath);
    } else {
        p.setPen(QColor(100, 100, 100));
        QFont f("Arial", 7);
        p.setFont(f);
        p.drawText(elevRect, Qt::AlignCenter, "No elevation");
    }

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
