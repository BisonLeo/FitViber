#pragma once

#include <QWidget>
#include <QSet>
#include <QPair>
#include <memory>

class TimelineModel;

class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget* parent = nullptr);
    ~TimelineWidget();

    TimelineModel* model() { return m_model.get(); }

    void addClipFromFile(const QString& path);
    void deleteSelectedClips();

signals:
    void playheadScrubbed(double seconds);
    void seekRequested(double seconds);
    void clipAdded(const QString& path, double timelineOffset, double duration);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    QSize minimumSizeHint() const override { return QSize(200, 120); }

private:
    void paintRuler(QPainter& painter, const QRect& rect);
    void paintTracks(QPainter& painter, const QRect& rect);
    void paintPlayhead(QPainter& painter, const QRect& rect);

    double xToTime(int x) const;
    int timeToX(double time) const;

    // Hit-test: returns (trackIndex, clipIndex) or (-1,-1) if no clip at pos
    QPair<int,int> clipAtPosition(const QPoint& pos) const;

    // Snap proposed time to nearby clip edges on the same track
    double snapToClipEdges(int trackIndex, double proposedTime,
                           double clipDuration, int excludeClipIndex = -1) const;

    // Resolve overlap: push proposedTime forward if it would overlap existing clips
    double resolveOverlap(int trackIndex, double proposedTime,
                          double clipDuration, int excludeClipIndex = -1) const;

    std::unique_ptr<TimelineModel> m_model;

    // Playhead scrub state
    bool m_scrubbing = false;

    // Clip selection: set of (trackIndex, clipIndex) pairs
    QSet<QPair<int,int>> m_selectedClips;

    // Clip drag-move state
    bool m_draggingClip = false;
    int m_dragTrack = -1;
    int m_dragClip = -1;
    double m_dragClickOffset = 0.0;  // time offset from clip start to mouse click

    static constexpr int RulerHeight = 28;
    static constexpr int TrackHeight = 40;
    static constexpr int TrackHeaderWidth = 80;
    static constexpr double SnapThresholdPx = 10.0;
};
