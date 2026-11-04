// Pass reminder planning: lead time, past/imminent passes, stable ids.
#include "../src/platform/apple_passreminder.hpp"
#include <QCoreApplication>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using namespace hamclock;
    const QDateTime now(QDate(2026, 10, 24), QTime(12, 0), Qt::UTC);
    const QDateTime aos = now.addSecs(3600), los = aos.addSecs(600);

    auto p = planPassReminder("ISS", aos, los, 54.4, 10, now);
    check(p.valid && p.fire_utc == aos.addSecs(-600), "fires lead minutes before AOS");
    check(p.title == "ISS pass in 10 min" && p.body.contains("54°"), "text");
    check(p.id == planPassReminder("ISS", aos, los, 20, 5, now).id, "id stable across lead/elevation");
    check(p.id != planPassReminder("ISS", aos.addDays(1), los.addDays(1), 54, 10, now).id, "id differs per pass");

    check(!planPassReminder("ISS", aos, los, 54, 10, aos.addSecs(-300)).valid, "lead already elapsed");
    check(!planPassReminder("ISS", aos, los, 54, 10, los.addSecs(1)).valid, "pass over");
    check(!planPassReminder("", aos, los, 54, 10, now).valid, "no satellite");
    check(!planPassReminder("ISS", los, aos, 54, 10, now).valid, "LOS before AOS");
    check(planPassReminder("ISS", aos, los, 54, -5, now).fire_utc == aos, "negative lead clamps to 0");
    check(planPassReminder("ISS", aos.addDays(1), los.addDays(1), 54, 9999, now).fire_utc ==
              aos.addDays(1).addSecs(-7200), "lead clamps to 120 min");
    check(!nativeSchedulePassReminder(p), "no native scheduler off iOS");
    {
        QStringList scheduled, cancelled;
        PassReminderSync sync([&](const ReminderPlan& pl) { scheduled << pl.id; return true; },
                              [&](const QString& id) { cancelled << id; });
        auto row = [&](const char* n, bool star, const QDateTime& a) {
            return QVariant(QVariantMap{{"name", n}, {"starred", star}, {"aos", a.toSecsSinceEpoch()},
                                        {"los", a.addSecs(600).toSecsSinceEpoch()}, {"maxEl", 40}});
        };
        sync.sync({row("ISS", true, aos), row("AO-91", false, aos)}, now);
        check(scheduled.size() == 1 && scheduled[0].startsWith("pass.ISS."), "only starred rows scheduled");
        sync.sync({row("ISS", true, aos)}, now);
        check(scheduled.size() == 1, "unchanged rows are not rescheduled");
        sync.sync({row("ISS", false, aos)}, now);
        check(cancelled.size() == 1 && cancelled[0] == scheduled[0], "unstarred pass cancelled");
        sync.sync({row("ISS", true, aos)}, now);
        sync.clear();
        check(scheduled.size() == 2 && cancelled.size() == 2, "clear cancels everything");
    }
    return failures ? 1 : 0;
}
