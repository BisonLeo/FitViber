#include "TimelineWidget.h"
#include "TimelineModel.h"
#include "Track.h"
#include "TimeUtil.h"
#include "MediaProbe.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMenu>
#include <QFileInfo>
#include <QUrl>
#include <algorithm>

TimelineWidget::TimelineWidget(QWidget* parent)
    : QWidget(parent)
    , m_model(std::make_unique<TimelineModel>(this))
{
    setMinimumHeight(120);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);

    connect(m_model.get(), &TimelineModel::playheadChanged, this, [this](double) {
        update();
    });
}

TimelineWidget::~TimelineWidget() = default;

// --- Snapping ---

double TimelineWidget::snapToClipEdges(int trackIndex, double proposedTime,
                                        double clipDuration, int excludeClipIndex) const {
    Track* track = m_model->track(trackIndex);
    if (!track) return proposedTime;

    double snapThreshold = SnapThresholdPx / m_model->zoom(); // convert px to seconds
    double bestTime = proposedTime;
    double bestDist = snapThreshold + 1.0; // start with no snap

    const auto& clips = track->clips();
    for (int i = 0; i < static_cast<int>(clips.size()); ++i) {
        if (i == excludeClipIndex) continue;
        const Clip& other = clips[i];
        double otherStart = other.timelineOffset;
        double otherEnd = otherStart + other.duration();

        // Snap proposed start to other's end
        double dist = std::abs(proposedTime - otherEnd);
        if (dist < bestDist) {
            bestDist = dist;
            bestTime = otherEnd;
        }
        // Snap proposed start to other's start
        dist = std::abs(proposedTime - otherStart);
        if (dist < bestDist) {
            bestDist = dist;
            bestTime = otherStart;
        }
        // Snap proposed end to other's start
        dist = std::abs((proposedTime + clipDuration) - otherStart);
        if (dist < bestDist) {
            bestDist = dist;
            bestTime = otherStart - clipDuration;
        }
        // Snap proposed end to other's end
        dist = std::abs((proposedTime + clipDuration) - otherEnd);
        if (dist < bestDist) {
            bestDist = dist;
            bestTime = otherEnd - clipDuration;
        }
    }

    if (bestDist <= snapThreshold) {
        return bestTime;
    }
    return proposedTime;
}

// --- Overlap resolution ---

double TimelineWidget::resolveOverlap(int trackIndex, double proposedTime,
                                       double clipDuration, int excludeClipIndex) const {
    Track* track = m_model->track(trackIndex);
    if (!track) return proposedTime;

    double newStart = proposedTime;
    double newEnd = newStart + clipDuration;

    const auto& clips = track->clips();
    // Iterate until no overlap found (simple iterative push-forward)
    bool changed = true;
    int iterations = 0;
    while (changed && iterations < 100) {
        changed = false;
        for (int i = 0; i < static_cast<int>(clips.size()); ++i) {
            if (i == excludeClipIndex) continue;
            const Clip& other = clips[i];
            double oStart = other.timelineOffset;
            double oEnd = oStart + other.duration();

            // Check if [newStart, newEnd) overlaps [oStart, oEnd)
            if (newStart < oEnd && newEnd > oStart) {
                // Push to end of the overlapping clip
                newStart = oEnd;
                newEnd = newStart + clipDuration;
                changed = true;
            }
        }
        ++iterations;
    }

    return newStart;
}

// --- Hit-test ---

QPair<int,int> TimelineWidget::clipAtPosition(const QPoint& pos) const {
    int y = pos.y() - RulerHeight;
    int x = pos.x();
    if (y < 0 || x < TrackHeaderWidth) return {-1, -1};

    int trackIndex = y / TrackHeight;
    if (trackIndex >= m_model->trackCount()) return {-1, -1};

    Track* track = m_model->track(trackIndex);
    if (!track) return {-1, -1};

    double pps = m_model->zoom();
    double scrollOff = m_model->scrollOffset();

    const auto& clips = track->clips();
    for (int i = 0; i < static_cast<int>(clips.size()); ++i) {
        const Clip& clip = clips[i];
        int cx = TrackHeaderWidth + static_cast<int>((clip.timelineOffset - scrollOff) * pps);
        int cw = std::max(static_cast<int>(clip.duration() * pps), 4);
        if (x >= cx && x <= cx + cw) {
            return {trackIndex, i};
        }
    }
    return {-1, -1};
}

// --- Add clip from file ---

void TimelineWidget::addClipFromFile(const QString& path) {
    QFileInfo fi(path);
    QString suffix = fi.suffix().toLower();

    // Classify media type
    QStringList imageExts = {"jpg", "jpeg", "png", "bmp", "tiff", "tif"};
    bool isImage = imageExts.contains(suffix);

    // Get absolute timestamp
    double absTimestamp = TimeUtil::extractMediaTimestamp(path);

    // Get duration
    double duration = 5.0; // default for images
    if (!isImage) {
        MediaProbe probe;
        if (probe.probe(path)) {
            duration = probe.info().duration;
        }
    }

    // Find or create appropriate track
    QString trackName = isImage ? "Images" : "Video";
    Track* targetTrack = nullptr;
    int targetTrackIndex = -1;
    for (int i = 0; i < m_model->trackCount(); ++i) {
        Track* t = m_model->track(i);
        if (t->type() == TrackType::Video) {
            targetTrack = t;
            targetTrackIndex = i;
            break;
        }
    }
    if (!targetTrack) {
        targetTrack = m_model->addTrack(TrackType::Video, trackName);
        targetTrackIndex = m_model->trackCount() - 1;
    }

    double relativeOffset = 0.0;

    if (targetTrack->clipCount() == 0) {
        // First clip on this track: use its absolute timestamp as the time origin
        if (m_model->timeOrigin() == 0.0) {
            m_model->setTimeOrigin(absTimestamp);
        }
        relativeOffset = m_model->absoluteToRelative(absTimestamp);
    } else {
        // Subsequent clips: place at the end of the last clip (append, no overlap)
        double trackEnd = 0.0;
        for (const auto& c : targetTrack->clips()) {
            double cEnd = c.timelineOffset + c.duration();
            if (cEnd > trackEnd) trackEnd = cEnd;
        }
        relativeOffset = trackEnd;
    }

    // Ensure no overlap with existing clips
    relativeOffset = resolveOverlap(targetTrackIndex, relativeOffset, duration, -1);

    // Create clip
    Clip clip;
    clip.sourcePath = path;
    clip.displayName = fi.fileName();
    clip.type = isImage ? ClipType::Image : ClipType::Video;
    clip.sourceIn = 0.0;
    clip.sourceOut = duration;
    clip.timelineOffset = relativeOffset;
    clip.absoluteStartTime = absTimestamp;

    targetTrack->addClip(clip);
    emit clipAdded(path, relativeOffset, duration);
    update();
}

// --- Delete selected clips ---

void TimelineWidget::deleteSelectedClips() {
    if (m_selectedClips.isEmpty()) return;

    // Sort by track and clip index descending so removing doesn't shift indices
    QList<QPair<int,int>> sorted = m_selectedClips.values();
    std::sort(sorted.begin(), sorted.end(), [](const QPair<int,int>& a, const QPair<int,int>& b) {
        if (a.first != b.first) return a.first > b.first;
        return a.second > b.second;
    });

    for (const auto& sel : sorted) {
        Track* track = m_model->track(sel.first);
        if (track && sel.second < track->clipCount()) {
            track->removeClip(sel.second);
        }
    }

    m_selectedClips.clear();
    update();
}

// --- Paint ---

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

    double pixelsPerSecond = m_model->zoom();
    double offset = m_model->scrollOffset();
    bool hasOrigin = m_model->timeOrigin() > 0;

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

        if (hasOrigin) {
            double absTime = m_model->relativeToAbsolute(t);
            painter.drawText(x + 3, rect.bottom() - 10,
                             TimeUtil::unixToLocalTimeStr(absTime));
        } else {
            painter.drawText(x + 3, rect.bottom() - 10, TimeUtil::secondsToMMSS(t));
        }
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
        double scrollOff = m_model->scrollOffset();
        const auto& clips = track->clips();
        for (int ci = 0; ci < static_cast<int>(clips.size()); ++ci) {
            const Clip& clip = clips[ci];
            int cx = trackRect.left() + static_cast<int>((clip.timelineOffset - scrollOff) * pps);
            int cw = std::max(static_cast<int>(clip.duration() * pps), 4);
            QRect clipRect(cx, y + 2, cw, TrackHeight - 4);

            // Clip color
            QColor clipColor;
            if (clip.type == ClipType::Image) {
                clipColor = QColor(100, 140, 80);
            } else if (track->type() == TrackType::Video) {
                clipColor = QColor(60, 100, 160);
            } else if (track->type() == TrackType::Audio) {
                clipColor = QColor(60, 140, 80);
            } else {
                clipColor = QColor(160, 100, 60);
            }

            // Dim locked clips slightly
            if (clip.locked) {
                clipColor = clipColor.darker(120);
            }

            painter.fillRect(clipRect, clipColor);

            // Selection highlight or normal border
            bool isSelected = m_selectedClips.contains({i, ci});
            if (isSelected) {
                painter.setPen(QPen(QColor(255, 200, 50), 2));
            } else {
                painter.setPen(clipColor.lighter(130));
            }
            painter.drawRect(clipRect);

            // Draw clip name
            if (cw > 40) {
                painter.setPen(QColor(220, 220, 220));
                QFont clipFont("Arial", 7);
                painter.setFont(clipFont);
                int textRightMargin = clip.locked ? 16 : 2;
                painter.drawText(clipRect.adjusted(4, 0, -textRightMargin, 0),
                                 Qt::AlignVCenter | Qt::TextSingleLine,
                                 clip.displayName);
            }

            // Lock indicator
            if (clip.locked) {
                painter.setPen(QColor(200, 200, 200));
                QFont lockFont("Arial", 7, QFont::Bold);
                painter.setFont(lockFont);
                QRect lockRect(clipRect.right() - 14, clipRect.top() + 1, 13, 13);
                painter.drawText(lockRect, Qt::AlignCenter, "L");
            }
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

// --- Mouse events ---

void TimelineWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;

    int mx = static_cast<int>(event->position().x());
    int my = static_cast<int>(event->position().y());

    if (mx <= TrackHeaderWidth) return;

    // Ruler area → playhead scrub
    if (my < RulerHeight) {
        m_scrubbing = true;
        double time = xToTime(mx);
        m_model->setPlayheadPosition(std::max(0.0, time));
        emit playheadScrubbed(time);
        update();
        return;
    }

    // Track area → clip selection / drag
    auto hit = clipAtPosition(QPoint(mx, my));

    if (hit.first >= 0) {
        // Clicked on a clip
        bool ctrlHeld = event->modifiers() & Qt::ControlModifier;

        if (ctrlHeld) {
            // Toggle selection
            if (m_selectedClips.contains(hit)) {
                m_selectedClips.remove(hit);
            } else {
                m_selectedClips.insert(hit);
            }
        } else {
            if (!m_selectedClips.contains(hit)) {
                m_selectedClips.clear();
                m_selectedClips.insert(hit);
            }
        }

        // Begin drag if clip is not locked
        Track* track = m_model->track(hit.first);
        if (track && hit.second < track->clipCount()) {
            const Clip& clip = track->clip(hit.second);
            if (!clip.locked) {
                m_draggingClip = true;
                m_dragTrack = hit.first;
                m_dragClip = hit.second;
                double clickTime = xToTime(mx);
                m_dragClickOffset = clickTime - clip.timelineOffset;
            }
        }
    } else {
        // Clicked empty area → deselect all
        m_selectedClips.clear();
    }

    update();
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* event) {
    int mx = static_cast<int>(event->position().x());

    if (m_scrubbing) {
        double time = xToTime(mx);
        m_model->setPlayheadPosition(std::max(0.0, time));
        emit playheadScrubbed(time);
        update();
        return;
    }

    if (m_draggingClip) {
        Track* track = m_model->track(m_dragTrack);
        if (!track || m_dragClip >= track->clipCount()) {
            m_draggingClip = false;
            return;
        }

        Clip& clip = track->clips()[m_dragClip];
        double newTime = xToTime(mx) - m_dragClickOffset;
        newTime = std::max(0.0, newTime);

        // Apply snapping, then prevent overlap
        newTime = snapToClipEdges(m_dragTrack, newTime, clip.duration(), m_dragClip);
        newTime = resolveOverlap(m_dragTrack, newTime, clip.duration(), m_dragClip);

        clip.timelineOffset = newTime;
        update();
    }
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_scrubbing) {
            m_scrubbing = false;
            emit seekRequested(m_model->playheadPosition());
        }
        m_draggingClip = false;
    }
}

// --- Key events ---

void TimelineWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        deleteSelectedClips();
    } else {
        QWidget::keyPressEvent(event);
    }
}

// --- Context menu ---

void TimelineWidget::contextMenuEvent(QContextMenuEvent* event) {
    auto hit = clipAtPosition(event->pos());
    if (hit.first < 0) return;

    // Ensure the right-clicked clip is selected
    if (!m_selectedClips.contains(hit)) {
        m_selectedClips.clear();
        m_selectedClips.insert(hit);
        update();
    }

    Track* track = m_model->track(hit.first);
    if (!track || hit.second >= track->clipCount()) return;

    QMenu menu(this);

    // Delete action
    QAction* deleteAction = menu.addAction("Delete");
    connect(deleteAction, &QAction::triggered, this, &TimelineWidget::deleteSelectedClips);

    menu.addSeparator();

    // Lock/Unlock toggle
    bool allLocked = true;
    for (const auto& sel : m_selectedClips) {
        Track* t = m_model->track(sel.first);
        if (t && sel.second < t->clipCount()) {
            if (!t->clip(sel.second).locked) {
                allLocked = false;
                break;
            }
        }
    }

    QAction* lockAction = menu.addAction(allLocked ? "Unlock" : "Lock");
    lockAction->setCheckable(true);
    lockAction->setChecked(allLocked);
    connect(lockAction, &QAction::triggered, this, [this, allLocked]() {
        bool newState = !allLocked;
        for (const auto& sel : m_selectedClips) {
            Track* t = m_model->track(sel.first);
            if (t && sel.second < t->clipCount()) {
                t->clips()[sel.second].locked = newState;
            }
        }
        update();
    });

    menu.exec(event->globalPos());
}

// --- Wheel ---

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

// --- Drag and drop ---

void TimelineWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void TimelineWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void TimelineWidget::dropEvent(QDropEvent* event) {
    const QMimeData* mime = event->mimeData();
    if (!mime->hasUrls()) return;

    for (const QUrl& url : mime->urls()) {
        if (!url.isLocalFile()) continue;
        QString path = url.toLocalFile();

        // Skip FIT files
        QFileInfo fi(path);
        if (fi.suffix().toLower() == "fit") continue;

        addClipFromFile(path);
    }

    event->acceptProposedAction();
}

// --- Coordinate conversion ---

double TimelineWidget::xToTime(int x) const {
    return (x - TrackHeaderWidth) / m_model->zoom() + m_model->scrollOffset();
}

int TimelineWidget::timeToX(double time) const {
    return TrackHeaderWidth + static_cast<int>((time - m_model->scrollOffset()) * m_model->zoom());
}
