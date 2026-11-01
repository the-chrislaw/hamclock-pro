#include "pane_fill.hpp"

#include <QColor>
#include <QQuickItem>
#include <QVariant>
#include <algorithm>
#include <cstdlib>

namespace hamclock {
namespace {

constexpr int kColorTolerance = 6;  // anti-aliasing and dithering noise, per channel

bool near_color(QRgb a, QRgb b) {
    return std::abs(qRed(a) - qRed(b)) <= kColorTolerance && std::abs(qGreen(a) - qGreen(b)) <= kColorTolerance &&
           std::abs(qBlue(a) - qBlue(b)) <= kColorTolerance;
}

void collect_slots(QQuickItem* item, std::vector<QQuickItem*>& out) {
    if (!item || !item->isVisible()) return;
    if (item->objectName() == "paneSlot" && item->width() > 0 && item->height() > 0) out.push_back(item);
    for (auto* child : item->childItems()) collect_slots(child, out);
}

QQuickItem* find_body(QQuickItem* item) {
    if (!item || !item->isVisible()) return nullptr;
    if (item->objectName() == "cardBody") return item;
    for (auto* child : item->childItems())
        if (auto* body = find_body(child)) return body;
    return nullptr;
}

QString widget_state(QQuickItem* card) {
    const auto* widget = card->property("widget").value<QObject*>();
    return widget ? widget->property("state").toString() : QString("none");
}

} // namespace

double blank_band(const QImage& image, const QRect& rect, QRgb background) {
    const QRect area = rect.intersected(image.rect());
    if (area.height() <= 0 || area.width() <= 0) return 0.0;
    int longest = 0, run = 0;
    for (int y = area.top(); y <= area.bottom(); ++y) {
        bool blank = true;
        for (int x = area.left(); x <= area.right() && blank; ++x) blank = near_color(image.pixel(x, y), background);
        run = blank ? run + 1 : 0;
        longest = std::max(longest, run);
    }
    return static_cast<double>(longest) / area.height();
}

std::vector<double> pane_layout_state(QQuickItem* root) {
    std::vector<QQuickItem*> found;
    collect_slots(root, found);
    std::vector<double> state;
    for (auto* slot : found) {
        auto* body = find_body(slot);
        auto* card = body && body->parentItem() ? body->parentItem()->parentItem() : nullptr;
        if (!card) continue;
        state.push_back(card->property("fit").toDouble());
        state.push_back(card->property("lift").toDouble());
    }
    return state;
}

std::vector<PaneFill> measure_pane_fill(QQuickItem* root, const QImage& image) {
    std::vector<QQuickItem*> found;
    collect_slots(root, found);
    const double ratio = root && root->width() > 0 ? image.width() / root->width() : 1.0;
    std::vector<PaneFill> result;
    for (auto* slot : found) {
        auto* body = find_body(slot);
        auto* card = body && body->parentItem() ? body->parentItem()->parentItem() : nullptr;
        if (!card) continue;
        PaneFill fill;
        fill.slot = slot->property("index").toInt() + 1;
        fill.title = card->property("title").toString();
        fill.state = widget_state(card);
        fill.paneRect = slot->mapRectToScene(slot->boundingRect());
        const QRectF scene = body->mapRectToScene(body->boundingRect());
        const QRect pixels(qRound(scene.x() * ratio), qRound(scene.y() * ratio), qRound(scene.width() * ratio),
                           qRound(scene.height() * ratio));
        fill.blankBand = blank_band(image, pixels, card->property("color").value<QColor>().rgb());
        result.push_back(fill);
    }
    std::sort(result.begin(), result.end(), [](const PaneFill& a, const PaneFill& b) { return a.slot < b.slot; });
    return result;
}

} // namespace hamclock
