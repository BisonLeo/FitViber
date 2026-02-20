#pragma once

#include <QWidget>
#include <memory>

class TimelineModel;

class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget* parent = nullptr);
    ~TimelineWidget();

    TimelineModel* model() { return m_model.get(); }

signals:
    void playheadScrubbed(double seconds);
    void seekRequested(double seconds);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    QSize minimumSizeHint() const override { return QSize(200, 120); }

private:
    void paintRuler(QPainter& painter, const QRect& rect);
    void paintTracks(QPainter& painter, const QRect& rect);
    void paintPlayhead(QPainter& painter, const QRect& rect);

    double xToTime(int x) const;
    int timeToX(double time) const;

    std::unique_ptr<TimelineModel> m_model;
    bool m_scrubbing = false;

    static constexpr int RulerHeight = 28;
    static constexpr int TrackHeight = 40;
    static constexpr int TrackHeaderWidth = 80;
};
