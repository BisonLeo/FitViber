#include "OverlayRenderer.h"
#include <QPainter>

OverlayRenderer::OverlayRenderer(QObject* parent) : QObject(parent) {}
OverlayRenderer::~OverlayRenderer() = default;

void OverlayRenderer::render(QImage& frame, const FitRecord& record, const FitSession& session) {
    QPainter painter(&frame);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    for (auto& panel : m_panels) {
        if (!panel->config().visible) continue;

        QRect rect = panel->resolveRect(frame.width(), frame.height());
        panel->paint(painter, rect, record, session);
    }

    painter.end();
}

void OverlayRenderer::addPanel(std::unique_ptr<OverlayPanel> panel) {
    m_panels.push_back(std::move(panel));
}

void OverlayRenderer::removePanel(int index) {
    if (index >= 0 && index < static_cast<int>(m_panels.size())) {
        m_panels.erase(m_panels.begin() + index);
    }
}

void OverlayRenderer::clearPanels() {
    m_panels.clear();
}

OverlayPanel* OverlayRenderer::panel(int index) {
    if (index >= 0 && index < static_cast<int>(m_panels.size())) {
        return m_panels[index].get();
    }
    return nullptr;
}
