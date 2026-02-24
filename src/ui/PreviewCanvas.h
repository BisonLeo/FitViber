#pragma once

#include <QWidget>
#include <QImage>
#include <QPointF>
#include <QSize>

struct ClipTransform;

class PreviewCanvas : public QWidget {
    Q_OBJECT
public:
    explicit PreviewCanvas(QWidget* parent = nullptr);

    void setFrame(const QImage& frame);
    void setTransform(ClipTransform* transform);
    void setHandlesVisible(bool visible);
    void setCanvasSize(QSize canvasSize);
    void setSourceSize(QSize sourceSize);
    void setComposited(bool composited);
    void resetView();

signals:
    void transformChanged();
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    enum class DragMode { None, Scale, Pan, Rotate, ViewPan };

    // Compute the display rect for the canvas (with view zoom + pan applied)
    QRectF canvasDisplayRect() const;

    // Get transformed corner positions of the source frame in widget space
    QVector<QPointF> transformedCorners() const;

    // Hit-test mouse position
    DragMode hitTest(const QPointF& pos, int& cornerIndex) const;

    QImage m_frame;
    ClipTransform* m_transform = nullptr;
    bool m_handlesVisible = false;
    bool m_composited = false;
    QSize m_canvasSize{1920, 1080};
    QSize m_sourceSize;

    // Clip transform drag state
    DragMode m_dragMode = DragMode::None;
    QPointF m_dragStart;
    double m_dragStartScale = 1.0;
    double m_dragStartRotation = 0.0;
    double m_dragStartPanX = 0.0;
    double m_dragStartPanY = 0.0;
    int m_activeCorner = -1;

    // Viewport navigation (view zoom + pan)
    double m_viewZoom = 1.0;
    QPointF m_viewOffset{0.0, 0.0};  // pixel offset in widget space
    QPointF m_viewPanStart;          // mouse pos at start of view pan
    QPointF m_viewOffsetStart;       // view offset at start of view pan

    static constexpr double HandleRadius = 8.0;
    static constexpr double RotateMargin = 30.0;
};
