#include "../src/platform/credential_store.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <cstdio>

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); return 1; } } while (0)

using namespace hamclock;

static int exercise(CredentialStore& s) {
    CHECK(s.get("a").isEmpty());
    CHECK(s.set("a", "secret one"));
    CHECK(s.set("b", QString::fromUtf8("pässwörd\"{}")));
    CHECK(s.get("a") == "secret one");
    CHECK(s.get("b") == QString::fromUtf8("pässwörd\"{}"));
    CHECK(s.set("a", "changed"));
    CHECK(s.get("a") == "changed");
    CHECK(s.remove("a"));
    CHECK(s.get("a").isEmpty());
    CHECK(s.get("b") == QString::fromUtf8("pässwörd\"{}"));
    CHECK(s.remove("never-existed"));
    CHECK(s.remove("b"));
    return 0;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString path = dir.path() + "/sub/credentials.json";

    FileCredentialStore file(path);
    if (exercise(file)) return 1;

    // Owner-only permissions, and a second instance sees persisted data.
    CHECK(file.set("k", "v"));
#ifndef Q_OS_WIN
    const auto perms = QFileInfo(path).permissions();
    CHECK(perms & QFileDevice::ReadOwner);
    CHECK(perms & QFileDevice::WriteOwner);
    CHECK(!(perms & (QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ReadOther |
                     QFileDevice::WriteOther)));
#endif
    FileCredentialStore again(path);
    CHECK(again.get("k") == "v");

    // Corrupt file reads as empty and is recoverable.
    QFile bad(path);
    CHECK(bad.open(QIODevice::WriteOnly));
    bad.write("not json");
    bad.close();
    CHECK(again.get("k").isEmpty());
    CHECK(again.set("k", "v2"));
    CHECK(again.get("k") == "v2");

#ifdef Q_OS_DARWIN
    KeychainCredentialStore kc(QStringLiteral("org.hamclockpro.test"));
    if (!kc.available()) {
        std::printf("Keychain unavailable on this host; skipping Keychain checks\n");
    } else {
        if (exercise(kc)) return 1;
        std::printf("Keychain checks passed\n");
    }
#endif
    std::printf("credential store ok\n");
    return 0;
}
