#include "OverlayPanel.h"

OverlayPanel::OverlayPanel(PanelType type, QObject* parent)
    : QObject(parent)
{
    m_config.type = type;
    // Note: label is set by each subclass constructor after calling base
}

OverlayPanel::~OverlayPanel() = default;

QRect OverlayPanel::resolveRect(int videoWidth, int videoHeight) const {
    int px = static_cast<int>(m_config.x * videoWidth);
    int py = static_cast<int>(m_config.y * videoHeight);
    int pw = static_cast<int>(m_config.width * videoWidth);
    int ph = static_cast<int>(m_config.height * videoHeight);
    return QRect(px, py, pw, ph);
}

void OverlayPanel::paintBackground(QPainter& painter, const QRect& rect) {
    painter.setOpacity(m_config.opacity);
    painter.fillRect(rect, m_config.bgColor);
    painter.setOpacity(1.0);
}

void OverlayPanel::paintLabel(QPainter& painter, const QRect& rect, const QString& label) {
    QFont labelFont = m_config.font;
    labelFont.setPointSize(std::max(8, m_config.font.pointSize() - 4));
    painter.setFont(labelFont);
    painter.setPen(QColor(180, 180, 180));

    QRect labelRect = rect.adjusted(6, 4, -6, -rect.height() / 2);
    painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, label);
}

void OverlayPanel::paintValue(QPainter& painter, const QRect& rect,
                               const QString& value, const QString& unit) {
    painter.setFont(m_config.font);
    painter.setPen(m_config.textColor);

    QRect valueRect = rect.adjusted(6, rect.height() / 3, -6, -4);
    QString text = unit.isEmpty() ? value : QString("%1 %2").arg(value, unit);
    painter.drawText(valueRect, Qt::AlignLeft | Qt::AlignVCenter, text);
}
