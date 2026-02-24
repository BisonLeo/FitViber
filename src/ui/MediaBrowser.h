#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStringList>
#include <memory>

class VideoDecoder;

enum class MediaType {
    Video,
    Image,
    FIT
};

class MediaBrowserListWidget : public QListWidget {
    Q_OBJECT
public:
    using QListWidget::QListWidget;
protected:
    void startDrag(Qt::DropActions supportedActions) override;
};

class MediaBrowser : public QWidget {
    Q_OBJECT
public:
    explicit MediaBrowser(QWidget* parent = nullptr);
    ~MediaBrowser();

signals:
    void mediaSelected(const QString& path);
    void fitFileSelected(const QString& path);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onImportClicked();
    void onItemClicked(QListWidgetItem* item);

private:
    static constexpr int ThumbWidth = 160;
    static constexpr int ThumbHeight = 120;
    static constexpr int UserRolePath = Qt::UserRole;
    static constexpr int UserRoleType = Qt::UserRole + 1;
    static constexpr int UserRoleDuration = Qt::UserRole + 2;

    QPixmap generateVideoThumbnail(const QString& path);
    QPixmap generateImageThumbnail(const QString& path);
    QPixmap generateFitThumbnail(const QString& path);
    MediaType classifyFile(const QString& suffix) const;

    void drawInfoText(QPainter& painter, const QRect& rect,
                      const QString& text, Qt::Alignment align);

    MediaBrowserListWidget* m_listWidget;
    QPushButton* m_importButton;
    QStringList m_mediaPaths;

    // Hover scrub state
    std::unique_ptr<VideoDecoder> m_scrubDecoder;
    QListWidgetItem* m_hoveredItem = nullptr;
    QIcon m_hoveredOriginalIcon;
};
