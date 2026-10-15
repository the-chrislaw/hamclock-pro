// BandActivityModel mirrors a source model's activity and state, offline.
#include "../band_activity_model.hpp"
#include <QCoreApplication>
#include <QDateTime>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}

class FakeCluster : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList activity MEMBER activity_ NOTIFY countChanged)
    Q_PROPERTY(QString state MEMBER state_ NOTIFY stateChanged)
    Q_PROPERTY(QDateTime updatedAt MEMBER updated_ NOTIFY stateChanged)
public:
    QVariantList activity_;
    QString state_ = "unconfigured";
    QDateTime updated_;
signals:
    void countChanged();
    void stateChanged();
};

QVariantMap row(const char* band, int total, int na, int eu) {
    return {{"band", band}, {"total", total}, {"NA", na}, {"SA", 0}, {"EU", eu}, {"AF", 0}, {"AS", 0}, {"OC", 0}};
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    BandActivityModel model{WidgetContext{}};
    check(model.state() == "unconfigured", "no source: unconfigured, never live");
    model.attach(nullptr);
    check(model.state() == "unconfigured" && model.rows().isEmpty(), "null source stays unconfigured");

    FakeCluster src;
    model.attach(&src);
    check(model.state() == "unconfigured", "unconfigured cluster mirrors");
    src.state_ = "live";
    src.updated_ = QDateTime::currentDateTimeUtc();
    emit src.stateChanged();
    check(model.state() == "loading", "live link without spots is not live data");
    src.activity_ = {row("40m", 2, 1, 1), row("20m", 7, 5, 2)};
    emit src.countChanged();
    check(model.state() == "live" && model.rows().size() == 2, "live with spots");
    check(model.maxCell() == 5, "largest cell");
    check(model.busiestText() == "20m · 7 spots", "busiest band");
    check(model.continents().size() == 6, "six continents");
    src.state_ = "stale";
    emit src.stateChanged();
    check(model.state() == "stale", "stale mirrors");
    src.state_ = "offline";
    emit src.stateChanged();
    check(model.state() == "offline", "offline is never live");
    return failures ? 1 : 0;
}

#include "band_activity_model_regression.moc"
