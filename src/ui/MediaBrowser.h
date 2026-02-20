#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStringList>

class MediaBrowser : public QWidget {
    Q_OBJECT
public:
    explicit MediaBrowser(QWidget* parent = nullptr);
    ~MediaBrowser();

signals:
    void videoFileSelected(const QString& path);
    void fitFileSelected(const QString& path);

private slots:
    void onImportClicked();
    void onItemDoubleClicked(QListWidgetItem* item);

private:
    QListWidget* m_listWidget;
    QPushButton* m_importButton;
    QStringList m_mediaPaths;
};
