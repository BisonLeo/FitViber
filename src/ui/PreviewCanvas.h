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

signals:
    void transformChanged();
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    enum class DragMode { None, Scale, Pan, Rotate };

    // Compute the display rect for the canvas fitted in the widget
    QRectF canvasDisplayRect() const;

    // Get transformed corner positions of the source frame in widget space
    QVector<QPointF> transformedCorners() const;

    // Hit-test mouse position
    DragMode hitTest(const QPointF& pos, int& cornerIndex) const;

    QImage m_frame;
    ClipTransform* m_transform = nullptr;
    bool m_handlesVisible = false;
    bool m_composited = false;   // true: frame is already composited, don't re-transform
    QSize m_canvasSize{1920, 1080};  // output canvas dimensions
    QSize m_sourceSize;              // original source media dimensions (for handles)

    DragMode m_dragMode = DragMode::None;
    QPointF m_dragStart;
    double m_dragStartScale = 1.0;
    double m_dragStartRotation = 0.0;
    double m_dragStartPanX = 0.0;
    double m_dragStartPanY = 0.0;
    int m_activeCorner = -1;

    static constexpr double HandleRadius = 8.0;
    static constexpr double RotateMargin = 30.0;
};
