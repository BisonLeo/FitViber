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

double OverlayPanel::drawSvgText(QPainter& painter, QPointF baselinePos, const QString& text, 
                               double fontSize, QColor color, Qt::Alignment align, bool isBold) {
    QFont font("Segoe UI", qMax(1, qRound(fontSize)));
    if (isBold || fontSize > 30) font.setWeight(QFont::Bold);
    else if (fontSize > 20) font.setWeight(QFont::DemiBold);
    else font.setWeight(QFont::Normal);
    
    painter.setFont(font);
    QFontMetrics fm(font);
    double advance = fm.horizontalAdvance(text);
    
    double x = baselinePos.x();
    double y = baselinePos.y();
    
    if (align & Qt::AlignRight) {
        x -= advance;
    } else if (align & Qt::AlignHCenter) {
        x -= advance / 2.0;
    }
    
    // Draw drop shadow
    painter.setPen(QColor(0, 0, 0, 150));
    painter.drawText(QPointF(x + 2, y + 2), text);
    
    // Draw main text
    painter.setPen(color);
    painter.drawText(QPointF(x, y), text);

    return advance;
}

void OverlayPanel::drawSvgArc(QPainter& painter, QPointF center, double radius,
                              double startAngleDeg, double spanAngleDeg, double strokeWidth, 
                              QColor color, QLinearGradient* gradient) {
    QRectF arcRect(center.x() - radius, center.y() - radius, radius * 2, radius * 2);
    
    QPen pen;
    if (gradient) {
        pen.setBrush(*gradient);
    } else {
        pen.setColor(color);
    }
    pen.setWidthF(strokeWidth);
    pen.setCapStyle(Qt::RoundCap);
    
    painter.setPen(pen);
    
    // Qt drawArc takes angles in 1/16th of a degree
    // Note: Qt's Y-axis grows downwards, but its angles go counter-clockwise (mathematical).
    // SVG angles are usually clockwise because Y grows downwards. We pass raw Qt degrees.
    painter.drawArc(arcRect, qRound(startAngleDeg * 16), qRound(spanAngleDeg * 16));
}
