// The source cache follows XDG_CACHE_HOME on every platform. Qt ignores that
// variable on macOS, so the app's offscreen smoke tests read the developer's real
// ~/Library/Caches/hamclock-pro and pass or fail depending on what it holds.
#include "../src/net/polled_source.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const char* label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("hamclock-pro");
    QTemporaryDir root;
    check(root.isValid(), "temporary dir");
    qputenv("XDG_CACHE_HOME", root.path().toUtf8());
    const hamclock::Transport none = [](const QUrl&, hamclock::FetchDone) {};
    hamclock::PolledSource source("cache-dir-test", QUrl("https://x.test/a"), {}, none);
    const QString want = QDir(root.path()).filePath("hamclock-pro/sources/cache-dir-test.cache");
    check(source.cachePath() == want, "cache follows XDG_CACHE_HOME");
    if (source.cachePath() != want) std::cerr << "  got " << source.cachePath().toStdString() << '\n';

    qunsetenv("XDG_CACHE_HOME");
    check(!source.cachePath().startsWith(root.path()), "platform cache dir without XDG_CACHE_HOME");

    std::cout << (failures ? "FAILED\n" : "cache-dir-regression: ok\n");
    return failures ? 1 : 0;
}
