#include "ui/alarms.hpp"

#include "ui/alarms_model.hpp"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <cstdio>

using namespace hamclock;

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (0)

static QDateTime utc(int d, int h, int m, int s = 0) { return QDateTime(QDate(2026, 10, d), QTime(h, m, s), Qt::UTC); }

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    Alarm a;
    a.minute_of_day = 7 * 60 + 30;
    CHECK(alarm_time_text(a.minute_of_day) == "07:30 UTC");
    CHECK(alarm_time_text(-1) == "23:59 UTC");
    CHECK(alarm_next(a, utc(15, 6, 0)) == utc(15, 7, 30));
    CHECK(alarm_next(a, utc(15, 7, 30)) == utc(16, 7, 30));  // strictly after now
    CHECK(alarm_next(a, utc(15, 23, 59)) == utc(16, 7, 30));

    CHECK(!alarm_due(a, utc(15, 7, 29, 58), utc(15, 7, 29, 59)));
    CHECK(alarm_due(a, utc(15, 7, 29, 59), utc(15, 7, 30, 0)));   // inclusive end
    CHECK(!alarm_due(a, utc(15, 7, 30, 0), utc(15, 7, 30, 1)));   // exclusive start: no double ring
    CHECK(alarm_due(a, utc(15, 7, 29, 0), utc(15, 7, 31, 0)));    // a late tick still rings
    CHECK(!alarm_due(a, utc(15, 7, 0), utc(15, 9, 0)));           // 2 h jump is not elapsed time
    CHECK(!alarm_due(a, utc(15, 7, 30, 1), utc(15, 7, 29, 0)));   // clock set backwards
    CHECK(!alarm_due(a, QDateTime(), utc(15, 7, 30)));

    Alarm over;  // crosses midnight
    over.minute_of_day = 0;
    CHECK(alarm_due(over, utc(15, 23, 59, 59), utc(16, 0, 0, 1)));

    a.enabled = false;
    CHECK(!alarm_next(a, utc(15, 6, 0)).isValid());
    CHECK(!alarm_due(a, utc(15, 7, 29, 59), utc(15, 7, 30, 0)));
    CHECK(alarm_in_text(a, utc(15, 6, 0)) == "off");
    a.enabled = true;
    CHECK(alarm_in_text(a, utc(15, 6, 0)) == "in 1h 30m");
    CHECK(alarm_in_text(a, utc(15, 7, 10)) == "in 20m");
    CHECK(alarm_in_text(a, utc(15, 7, 29, 30)) == "in 1m");
    CHECK(alarm_in_text(a, utc(15, 7, 30, 0)) == "in 24h 0m");

    // Persistence text round trip and rejection of bad entries.
    QVector<Alarm> list;
    list.push_back(a);
    Alarm once;
    once.minute_of_day = 23 * 60 + 5;
    once.daily = false;
    once.enabled = false;
    list.push_back(once);
    const QStringList text = alarms_serialize(list);
    CHECK(text == QStringList({"0730,d,1", "2305,o,0"}));
    const QVector<Alarm> back = alarms_parse(text);
    CHECK(back.size() == 2 && back[1].minute_of_day == 23 * 60 + 5 && !back[1].daily && !back[1].enabled);
    CHECK(alarms_parse({"2460,d,1", "0730,x,1", "0730,d,2", "junk", "", "07:30,d,1", "0730,d,1"}).size() == 1);
    QStringList many;
    for (int i = 0; i < 12; ++i) many << "0100,d,1";
    CHECK(alarms_parse(many).size() == 8);

    // The model saves on every change and a new model reads it back.
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString path = dir.path() + "/options.ini";
    {
        AlarmsModel m(path);
        CHECK(m.add(6, 15, true));
        CHECK(m.add(18, 0, false));
        m.setEnabled(0, false);
        CHECK(m.add(24, 0, true) == false);
    }
    {
        AlarmsModel m(path);
        CHECK(m.items().size() == 2);
        CHECK(m.items()[0].contains("06:15 UTC") && m.items()[0].endsWith("off"));
        CHECK(m.items()[1].contains("18:00 UTC") && m.items()[1].contains("once"));
        m.remove(0);
    }
    {
        AlarmsModel m(path);
        CHECK(m.items().size() == 1 && m.items()[0].contains("18:00 UTC"));
    }
    {
        AlarmsModel m;  // no path: session only, nothing read or written
        CHECK(m.items().isEmpty());
        CHECK(m.add(1, 2, true));
    }

    return failures ? 1 : 0;
}
