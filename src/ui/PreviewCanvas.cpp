#include "PreviewCanvas.h"
#include "ClipTransform.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>

PreviewCanvas::PreviewCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(320, 240);
    setMouseTracking(true);
}

void PreviewCanvas::setFrame(const QImage& frame) {
    m_frame = frame;
    update();
}

void PreviewCanvas::setTransform(ClipTransform* transform) {
    m_transform = transform;
    update();
}

void PreviewCanvas::setHandlesVisible(bool visible) {
    m_handlesVisible = visible;
    update();
}

void PreviewCanvas::setCanvasSize(QSize canvasSize) {
    m_canvasSize = canvasSize;
    update();
}

void PreviewCanvas::setSourceSize(QSize sourceSize) {
    m_sourceSize = sourceSize;
    update();
}

void PreviewCanvas::setComposited(bool composited) {
    m_composited = composited;
    update();
}

void PreviewCanvas::resetView() {
    m_viewZoom = 1.0;
    m_viewOffset = QPointF(0.0, 0.0);
    update();
}

QRectF PreviewCanvas::canvasDisplayRect() const {
    if (!m_canvasSize.isValid() || m_canvasSize.isEmpty()) return QRectF();

    QSizeF canvasSize(m_canvasSize);
    QSizeF widgetSize = size();

    // Base scale: fit canvas aspect ratio into widget
    double baseScale = qMin(widgetSize.width() / canvasSize.width(),
                            widgetSize.height() / canvasSize.height());
    double effectiveScale = baseScale * m_viewZoom;

    double w = canvasSize.width() * effectiveScale;
    double h = canvasSize.height() * effectiveScale;
    double x = (widgetSize.width() - w) / 2.0 + m_viewOffset.x();
    double y = (widgetSize.height() - h) / 2.0 + m_viewOffset.y();

    return QRectF(x, y, w, h);
}

QVector<QPointF> PreviewCanvas::transformedCorners() const {
    QRectF cr = canvasDisplayRect();
    if (cr.isNull() || !m_sourceSize.isValid() || m_sourceSize.isEmpty()) return {};

    QPointF center = cr.center();
    double displayScale = cr.width() / m_canvasSize.width();

    double s = m_transform ? m_transform->scale : 1.0;
    double rot = m_transform ? m_transform->rotation : 0.0;
    double px = m_transform ? m_transform->panX * displayScale : 0.0;
    double py = m_transform ? m_transform->panY * displayScale : 0.0;
    bool fh = m_transform ? m_transform->flipH : false;
    bool fv = m_transform ? m_transform->flipV : false;

    // Half-size of the source frame in display coordinates
    double hw = m_sourceSize.width() * displayScale / 2.0 * s;
    double hh = m_sourceSize.height() * displayScale / 2.0 * s;

    // Corners relative to center (before rotation)
    QPointF corners[4] = {
        {-hw, -hh}, {hw, -hh}, {hw, hh}, {-hw, hh}
    };

    if (fh) for (auto& c : corners) c.setX(-c.x());
    if (fv) for (auto& c : corners) c.setY(-c.y());

    double rad = qDegreesToRadians(rot);
    double cosR = qCos(rad);
    double sinR = qSin(rad);

    QVector<QPointF> result(4);
    for (int i = 0; i < 4; ++i) {
        double rx = corners[i].x() * cosR - corners[i].y() * sinR;
        double ry = corners[i].x() * sinR + corners[i].y() * cosR;
        result[i] = center + QPointF(px + rx, py + ry);
    }
    return result;
}

PreviewCanvas::DragMode PreviewCanvas::hitTest(const QPointF& pos, int& cornerIndex) const {
    cornerIndex = -1;
    if (!m_transform || !m_handlesVisible) return DragMode::None;

    auto corners = transformedCorners();
    if (corners.isEmpty()) return DragMode::None;

    // Check corner handles first (for scale)
    for (int i = 0; i < 4; ++i) {
        double dist = QLineF(pos, corners[i]).length();
        if (dist < HandleRadius) {
            cornerIndex = i;
            return DragMode::Scale;
        }
    }

    // Check if inside the polygon (for pan)
    QPolygonF poly;
    for (const auto& c : corners) poly << c;
    if (poly.containsPoint(pos, Qt::OddEvenFill)) {
        return DragMode::Pan;
    }

    // Check near corners but outside (for rotate)
    for (int i = 0; i < 4; ++i) {
        double dist = QLineF(pos, corners[i]).length();
        if (dist < HandleRadius + RotateMargin) {
            cornerIndex = i;
            return DragMode::Rotate;
        }
    }

    return DragMode::None;
}

void PreviewCanvas::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Gray background for widget area
    painter.fillRect(rect(), QColor(0x3a, 0x3a, 0x3a));

    QRectF cr = canvasDisplayRect();
    if (cr.isNull()) return;

    // Black canvas rectangle representing the output viewport
    painter.fillRect(cr, Qt::black);

    if (m_frame.isNull()) return;

    if (m_composited) {
        // Frame is already composited onto canvas — just draw it fitted in the canvas rect
        painter.drawImage(cr, m_frame);
    } else {
        // Raw source frame — apply transform visually within the canvas rect
        QPointF center = cr.center();
        double displayScale = cr.width() / m_canvasSize.width();

        if (m_transform && !m_transform->isIdentity()) {
            double s = m_transform->scale;
            double px = m_transform->panX * displayScale;
            double py = m_transform->panY * displayScale;

            painter.save();
            painter.setClipRect(cr);
            painter.translate(center + QPointF(px, py));
            painter.rotate(m_transform->rotation);
            double sx = m_transform->flipH ? -s : s;
            double sy = m_transform->flipV ? -s : s;
            painter.scale(sx, sy);

            // Draw source frame centered at origin, scaled to display
            double fw = m_frame.width() * displayScale;
            double fh = m_frame.height() * displayScale;
            painter.drawImage(QRectF(-fw / 2.0, -fh / 2.0, fw, fh), m_frame);
            painter.restore();
        } else {
            // Identity transform — draw source centered in canvas, scaled to fit
            QPointF center2 = cr.center();
            double displayScaleW = cr.width() / m_frame.width();
            double displayScaleH = cr.height() / m_frame.height();
            double fitScale = qMin(displayScaleW, displayScaleH);
            double fw = m_frame.width() * fitScale;
            double fh = m_frame.height() * fitScale;
            QRectF frameRect(center2.x() - fw / 2.0, center2.y() - fh / 2.0, fw, fh);
            painter.drawImage(frameRect, m_frame);
        }
    }

    // Draw handles (not clipped to canvas — always visible even when source extends beyond)
    if (m_handlesVisible && m_transform) {
        auto corners = transformedCorners();
        if (corners.size() == 4) {
            // Outline
            painter.setPen(QPen(Qt::white, 1.5, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            QPolygonF poly;
            for (const auto& c : corners) poly << c;
            poly << corners[0]; // close
            painter.drawPolyline(poly);

            // Corner knobs
            painter.setPen(QPen(Qt::white, 1.5));
            painter.setBrush(QColor(255, 255, 255, 200));
            for (const auto& c : corners) {
                painter.drawEllipse(c, HandleRadius, HandleRadius);
            }
        }
    }
}

void PreviewCanvas::mousePressEvent(QMouseEvent* event) {
    // Middle button → viewport pan
    if (event->button() == Qt::MiddleButton) {
        m_dragMode = DragMode::ViewPan;
        m_viewPanStart = event->position();
        m_viewOffsetStart = m_viewOffset;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    QPointF pos = event->position();
    int corner = -1;
    DragMode mode = hitTest(pos, corner);

    if (mode == DragMode::None) {
        emit clicked();
        return;
    }

    m_dragMode = mode;
    m_dragStart = pos;
    m_activeCorner = corner;

    if (m_transform) {
        m_dragStartScale = m_transform->scale;
        m_dragStartRotation = m_transform->rotation;
        m_dragStartPanX = m_transform->panX;
        m_dragStartPanY = m_transform->panY;
    }
}

void PreviewCanvas::mouseMoveEvent(QMouseEvent* event) {
    // Viewport pan with middle button
    if (m_dragMode == DragMode::ViewPan) {
        QPointF delta = event->position() - m_viewPanStart;
        m_viewOffset = m_viewOffsetStart + delta;
        update();
        return;
    }

    if (m_dragMode == DragMode::None) {
        // Update cursor based on hit-test
        int corner = -1;
        DragMode mode = hitTest(event->position(), corner);
        switch (mode) {
            case DragMode::Scale:  setCursor(Qt::SizeFDiagCursor); break;
            case DragMode::Pan:    setCursor(Qt::SizeAllCursor); break;
            case DragMode::Rotate: setCursor(Qt::CrossCursor); break;
            default:               setCursor(Qt::ArrowCursor); break;
        }
        return;
    }

    if (!m_transform) return;

    QPointF pos = event->position();
    QRectF cr = canvasDisplayRect();
    if (cr.isNull()) return;

    QPointF center = cr.center();
    double displayScale = cr.width() / m_canvasSize.width();

    switch (m_dragMode) {
        case DragMode::Scale: {
            double startDist = QLineF(center, m_dragStart).length();
            double curDist = QLineF(center, pos).length();
            if (startDist > 1.0) {
                m_transform->scale = m_dragStartScale * (curDist / startDist);
                m_transform->scale = qBound(0.1, m_transform->scale, 10.0);
            }
            break;
        }
        case DragMode::Pan: {
            QPointF delta = pos - m_dragStart;
            if (displayScale > 0.0) {
                m_transform->panX = m_dragStartPanX + delta.x() / displayScale;
                m_transform->panY = m_dragStartPanY + delta.y() / displayScale;
            }
            break;
        }
        case DragMode::Rotate: {
            double startAngle = qRadiansToDegrees(qAtan2(m_dragStart.y() - center.y(),
                                                          m_dragStart.x() - center.x()));
            double curAngle = qRadiansToDegrees(qAtan2(pos.y() - center.y(),
                                                        pos.x() - center.x()));
            m_transform->rotation = m_dragStartRotation + (curAngle - startAngle);
            break;
        }
        default: break;
    }

    update();
    emit transformChanged();
}

void PreviewCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton && m_dragMode == DragMode::ViewPan) {
        m_dragMode = DragMode::None;
        setCursor(Qt::ArrowCursor);
        return;
    }
    if (event->button() == Qt::LeftButton) {
        m_dragMode = DragMode::None;
        m_activeCorner = -1;
    }
}

void PreviewCanvas::wheelEvent(QWheelEvent* event) {
    if (!(event->modifiers() & Qt::ControlModifier)) {
        event->ignore();
        return;
    }

    // Ctrl+scroll → viewport zoom anchored at cursor
    QPointF mousePos = event->position();
    double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    double newZoom = qBound(0.1, m_viewZoom * factor, 20.0);

    // Compute the canvas-space point under the cursor before zoom
    // canvasDisplayRect uses m_viewZoom and m_viewOffset
    QRectF crBefore = canvasDisplayRect();
    if (crBefore.isNull()) {
        event->accept();
        return;
    }

    // Relative position within the canvas display rect (0..1)
    double relX = (mousePos.x() - crBefore.x()) / crBefore.width();
    double relY = (mousePos.y() - crBefore.y()) / crBefore.height();

    // Apply new zoom
    double oldZoom = m_viewZoom;
    m_viewZoom = newZoom;

    // Compute what the new canvas rect would be with current offset
    QSizeF canvasSize(m_canvasSize);
    QSizeF widgetSize = size();
    double baseScale = qMin(widgetSize.width() / canvasSize.width(),
                            widgetSize.height() / canvasSize.height());

    double newW = canvasSize.width() * baseScale * m_viewZoom;
    double newH = canvasSize.height() * baseScale * m_viewZoom;

    // The new top-left should be such that mousePos maps to the same relative point
    double newX = mousePos.x() - relX * newW;
    double newY = mousePos.y() - relY * newH;

    // The centered position (with zero offset) would be:
    double centeredX = (widgetSize.width() - newW) / 2.0;
    double centeredY = (widgetSize.height() - newH) / 2.0;

    // Offset is the difference
    m_viewOffset.setX(newX - centeredX);
    m_viewOffset.setY(newY - centeredY);

    update();
    event->accept();
}

void PreviewCanvas::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        resetView();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}
