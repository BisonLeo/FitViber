#include "ImageUtil.h"
#include <QPainter>

namespace ImageUtil {

QImage createPlaceholder(int width, int height) {
    QImage img(width, height, QImage::Format_RGB32);
    img.fill(QColor(30, 30, 30));

    QPainter p(&img);
    p.setPen(QColor(100, 100, 100));
    p.setFont(QFont("Arial", 14));
    p.drawText(img.rect(), Qt::AlignCenter, "No Video");
    p.end();

    return img;
}

} // namespace ImageUtil
