// Swipeable pane paging maths (src/ui/qml/pager.mjs, used by PaneSlot.qml):
// circular stepping, commit thresholds, flicks, rubber band, velocity and slide timing.
#include <QCoreApplication>
#include <QJSEngine>
#include <QJSValue>
#include <QJSValueList>
#include <QString>
#include <cmath>
#include <cstdio>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

namespace {

QJSValue pager;
QJSEngine* engine = nullptr;

QJSValue call(const char* name, const QJSValueList& args) {
    const QJSValue result = pager.property(name).call(args);
    if (result.isError()) std::fprintf(stderr, "%s threw: %s\n", name, qPrintable(result.toString()));
    return result;
}
double num(const char* name, const QJSValueList& args) { return call(name, args).toNumber(); }
double constant(const char* name) { return pager.property(name).toNumber(); }
bool near(double a, double b) { return std::fabs(a - b) < 1e-6; }

QJSValue list(std::initializer_list<const char*> ids) {
    QJSValue array = engine->newArray(static_cast<uint>(ids.size()));
    uint i = 0;
    for (const char* id : ids) array.setProperty(i++, QString::fromUtf8(id));
    return array;
}
QString neighbour(const QJSValue& ids, const char* current, int direction) {
    return call("neighbour", {ids, QString::fromUtf8(current), direction}).toString();
}

void checkNeighbour() {
    const QJSValue ids = list({"none", "bands", "dxcluster", "weather"});
    CHECK(neighbour(ids, "bands", 1) == "dxcluster");
    CHECK(neighbour(ids, "bands", -1) == "none");
    CHECK(neighbour(ids, "weather", 1) == "none");   // wraps through the empty page
    CHECK(neighbour(ids, "none", -1) == "weather");
    CHECK(neighbour(ids, "gone", 1) == "bands");     // unknown id counts as the first entry
    CHECK(neighbour(list({}), "bands", 1).isEmpty());
}

void checkRelease() {
    const double span = 400, flick = constant("FLICK_SPEED");
    const double commit = span * constant("COMMIT_FRACTION");
    // Slow drags: commit past a third of the width, otherwise spring back.
    CHECK(num("releaseStep", {-(commit + 1), 0, span, flick, true}) == 1);
    CHECK(num("releaseStep", {commit + 1, 0, span, flick, true}) == -1);
    CHECK(num("releaseStep", {-(commit - 1), 0, span, flick, true}) == 0);
    CHECK(num("releaseStep", {commit - 1, flick - 1, span, flick, true}) == 0);
    // A short, fast flick commits in its own direction.
    CHECK(num("releaseStep", {-20, -flick, span, flick, true}) == 1);
    CHECK(num("releaseStep", {20, flick * 3, span, flick, true}) == -1);
    // A flick back towards the current page cancels even a long drag.
    CHECK(num("releaseStep", {-(commit + 50), flick * 2, span, flick, true}) == 0);
    // Nothing to page to (one page) or no size yet: always spring back.
    CHECK(num("releaseStep", {-300, -flick * 4, span, flick, false}) == 0);
    CHECK(num("releaseStep", {-300, 0, 0, flick, true}) == 0);
}

void checkRubberBand() {
    const double span = 400;
    CHECK(num("rubberBand", {0, span}) == 0);
    const double small = num("rubberBand", {-40, span});
    CHECK(small < 0 && small > -40);                           // resists from the start
    CHECK(num("rubberBand", {40, span}) == -small);           // symmetric
    CHECK(num("rubberBand", {-4000, span}) > -span);          // never a whole page
    CHECK(num("rubberBand", {-800, span}) < num("rubberBand", {-400, span}));  // still follows
}

QJSValue sample(double t, double x) {
    QJSValue s = engine->newObject();
    s.setProperty("t", t);
    s.setProperty("x", x);
    return s;
}

void checkVelocity() {
    const double window = constant("VELOCITY_WINDOW_MS");
    QJSValue samples = engine->newArray(3);
    samples.setProperty(0, sample(1000, 0));
    samples.setProperty(1, sample(1040, -40));
    samples.setProperty(2, sample(1080, -80));
    CHECK(near(num("velocity", {samples, 1080, window}), -1000));      // 80 px in 80 ms
    // The finger stopped before lifting: the samples are stale, no flick.
    CHECK(num("velocity", {samples, 1080 + window + 100, window}) == 0);
    // A busy main thread delivered one move inside the window: measure from the one before.
    QJSValue sparse = engine->newArray(2);
    sparse.setProperty(0, sample(1000, 0));
    sparse.setProperty(1, sample(1000 + window + 50, -280));
    CHECK(near(num("velocity", {sparse, 1000 + window + 50, window}), -280 * 1000 / (window + 50)));
    // addSample returns a new, bounded array and leaves its input alone.
    QJSValue grown = samples;
    for (int i = 0; i < 10; ++i) grown = call("addSample", {grown, 2000 + i, -i});
    CHECK(grown.property("length").toInt() == constant("MAX_SAMPLES"));
    CHECK(samples.property("length").toInt() == 3);
    CHECK(grown.property(grown.property("length").toInt() - 1).property("x").toNumber() == -9);
}

void checkTiming() {
    const double span = 400;
    const double slideMs = constant("SLIDE_MS"), minMs = constant("MIN_SLIDE_MS"), maxMs = constant("MAX_SLIDE_MS");
    CHECK(num("slideDuration", {span, span, 0}) == slideMs);              // keys / rotation: a full page
    CHECK(num("slideDuration", {10, span, 0}) == minMs);                  // tiny spring back still eases
    const double flicked = num("slideDuration", {200, span, 4000});
    CHECK(flicked >= minMs && flicked < slideMs);                         // a fast flick lands sooner
    CHECK(num("slideDuration", {span, span, 50}) <= maxMs);
    CHECK(num("slideDuration", {0, span, 0}) == 0);
    CHECK(slideMs >= 250 && slideMs <= 300);
}

void checkDots() {
    CHECK(num("dotWeight", {3, 3}) == 1);
    CHECK(near(num("dotWeight", {4, 3.5}), 0.5));
    CHECK(near(num("dotWeight", {3, 3.5}), 0.5));
    CHECK(num("dotWeight", {5, 3}) == 0);
    CHECK(num("directionOf", {-1}) == 1 && num("directionOf", {1}) == -1 && num("directionOf", {0}) == 0);
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        std::fprintf(stderr, "usage: pager-regression <pager.mjs>\n");
        return 2;
    }
    QJSEngine js;
    engine = &js;
    pager = js.importModule(QString::fromLocal8Bit(argv[1]));
    if (pager.isError() || !pager.property("releaseStep").isCallable()) {
        std::fprintf(stderr, "could not load %s: %s\n", argv[1], qPrintable(pager.toString()));
        return 1;
    }
    checkNeighbour();
    checkRelease();
    checkRubberBand();
    checkVelocity();
    checkTiming();
    checkDots();
    if (failures) return 1;
    std::printf("ok\n");
    return 0;
}
