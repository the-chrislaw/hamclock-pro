#pragma once
// Pane fill check for --screenshot runs (adaptive panes, owner Oct 29: "no wasted
// space"). Measures how much of a dashboard pane's card body is left blank.
#include <QImage>
#include <QRectF>
#include <QString>
#include <vector>

class QQuickItem;

namespace hamclock {

struct PaneFill {
    int slot = 0;             // 1-based pane number
    QString title;            // card title
    QString state;            // widget state ("live", "loading", ...)
    QRectF paneRect;          // pane slot in window coordinates
    double blankBand = 0.0;   // tallest run of blank rows in the card body, as a fraction of its height
};

// Largest blank fraction a pane with data may leave in its card body.
constexpr double kMaxBlankBand = 0.34;

// Fill of every visible pane slot under root; image is the window grab.
std::vector<PaneFill> measure_pane_fill(QQuickItem* root, const QImage& image);

// Every visible pane card's adaptive scale (Card.fit) and lift, in tree order: equal readings
// a moment apart mean the panes have settled (screenshots wait for that).
std::vector<double> pane_layout_state(QQuickItem* root);

// Tallest run of rows in rect whose pixels all match background (within a small tolerance), as a fraction of rect height.
double blank_band(const QImage& image, const QRect& rect, QRgb background);

} // namespace hamclock
