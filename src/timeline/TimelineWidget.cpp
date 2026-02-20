#include "TimelineWidget.h"
#include "TimelineModel.h"
#include "TimeUtil.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>

TimelineWidget::TimelineWidget(QWidget* parent)
    : QWidget(parent)
    , m_model(std::make_unique<TimelineModel>(this))
{
    setMinimumHeight(120);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

TimelineWidget::~TimelineWidget() = default;

void TimelineWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), QColor(35, 35, 38));

    QRect rulerRect(TrackHeaderWidth, 0, width() - TrackHeaderWidth, RulerHeight);
    QRect tracksRect(0, RulerHeight, width(), height() - RulerHeight);

    paintRuler(painter, rulerRect);
    paintTracks(painter, tracksRect);
    paintPlayhead(painter, QRect(TrackHeaderWidth, 0, width() - TrackHeaderWidth, height()));
}

void TimelineWidget::paintRuler(QPainter& painter, const QRect& rect) {
    painter.fillRect(rect, QColor(50, 50, 54));

    double duration = std::max(m_model->duration(), 60.0);
    double pixelsPerSecond = m_model->zoom();
    double offset = m_model->scrollOffset();

    // Determine tick interval based on zoom level
    double tickInterval = 1.0;
    if (pixelsPerSecond < 5) tickInterval = 30.0;
    else if (pixelsPerSecond < 10) tickInterval = 10.0;
    else if (pixelsPerSecond < 30) tickInterval = 5.0;

    painter.setPen(QColor(130, 130, 130));
    QFont rulerFont("Arial", 8);
    painter.setFont(rulerFont);

    double startTime = offset;
    double endTime = offset + (rect.width() / pixelsPerSecond);

    for (double t = std::floor(startTime / tickInterval) * tickInterval;
         t <= endTime; t += tickInterval) {
        if (t < 0) continue;
        int x = rect.left() + static_cast<int>((t - offset) * pixelsPerSecond);
        if (x < rect.left() || x > rect.right()) continue;

        painter.drawLine(x, rect.bottom() - 8, x, rect.bottom());
        painter.drawText(x + 3, rect.bottom() - 10, TimeUtil::secondsToMMSS(t));
    }

    // Bottom border
    painter.setPen(QColor(60, 60, 64));
    painter.drawLine(rect.bottomLeft(), rect.bottomRight());
}

void TimelineWidget::paintTracks(QPainter& painter, const QRect& rect) {
    int y = rect.top();

    for (int i = 0; i < m_model->trackCount(); ++i) {
        auto* track = m_model->track(i);
        if (!track) continue;

        QRect headerRect(0, y, TrackHeaderWidth, TrackHeight);
        QRect trackRect(TrackHeaderWidth, y, rect.width() - TrackHeaderWidth, TrackHeight);

        // Track header
        painter.fillRect(headerRect, QColor(45, 45, 48));
        painter.setPen(QColor(180, 180, 180));
        painter.drawText(headerRect.adjusted(6, 0, 0, 0), Qt::AlignVCenter, track->name());

        // Track background
        painter.fillRect(trackRect, QColor(40, 40, 43));

        // Draw clips
        double pps = m_model->zoom();
        double offset = m_model->scrollOffset();
        for (const auto& clip : track->clips()) {
            int cx = trackRect.left() + static_cast<int>((clip.timelineOffset - offset) * pps);
            int cw = static_cast<int>(clip.duration() * pps);
            QRect clipRect(cx, y + 2, cw, TrackHeight - 4);

            QColor clipColor = (track->type() == TrackType::Video) ? QColor(60, 100, 160) :
                               (track->type() == TrackType::Audio) ? QColor(60, 140, 80) :
                                                                      QColor(160, 100, 60);
            painter.fillRect(clipRect, clipColor);
            painter.setPen(clipColor.lighter(130));
            painter.drawRect(clipRect);
        }

        // Track separator
        painter.setPen(QColor(55, 55, 58));
        painter.drawLine(0, y + TrackHeight, rect.width(), y + TrackHeight);

        y += TrackHeight;
    }
}

void TimelineWidget::paintPlayhead(QPainter& painter, const QRect& rect) {
    int x = timeToX(m_model->playheadPosition());
    if (x < rect.left() || x > rect.right()) return;

    painter.setPen(QPen(QColor(220, 50, 50), 2));
    painter.drawLine(x, rect.top(), x, rect.bottom());

    // Playhead handle
    QPolygon triangle;
    triangle << QPoint(x - 6, rect.top()) << QPoint(x + 6, rect.top())
             << QPoint(x, rect.top() + 8);
    painter.setBrush(QColor(220, 50, 50));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(triangle);
}

void TimelineWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && event->position().x() > TrackHeaderWidth) {
        m_scrubbing = true;
        double time = xToTime(static_cast<int>(event->position().x()));
        m_model->setPlayheadPosition(time);
        emit playheadScrubbed(time);
        update();
    }
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_scrubbing) {
        double time = xToTime(static_cast<int>(event->position().x()));
        m_model->setPlayheadPosition(std::max(0.0, time));
        emit playheadScrubbed(time);
        update();
    }
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_scrubbing = false;
        emit seekRequested(m_model->playheadPosition());
    }
}

void TimelineWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double factor = event->angleDelta().y() > 0 ? 1.2 : 1.0 / 1.2;
        m_model->setZoom(m_model->zoom() * factor);
        update();
    } else {
        double delta = event->angleDelta().y() > 0 ? -2.0 : 2.0;
        m_model->setScrollOffset(m_model->scrollOffset() + delta / m_model->zoom());
        update();
    }
}

double TimelineWidget::xToTime(int x) const {
    return (x - TrackHeaderWidth) / m_model->zoom() + m_model->scrollOffset();
}

int TimelineWidget::timeToX(double time) const {
    return TrackHeaderWidth + static_cast<int>((time - m_model->scrollOffset()) * m_model->zoom());
}
