#pragma once
// Read access to files the user picks (the ADIF log) under the macOS App Sandbox.
// A sandboxed Mac App Store build may only read files chosen in the system open
// panel; a security-scoped bookmark keeps that access across launches. On other
// platforms (and unsandboxed builds) these fall back to plain paths.

#include <QByteArray>
#include <QString>

namespace hamclock::file_access {

// True when a native open panel is available (macOS).
bool canChooseFiles();
// True when running under the App Sandbox.
bool sandboxed();
// Shows the open panel; returns the chosen path, empty when cancelled or unsupported.
QString chooseFileToRead(const QString& title);
// Read-only security-scoped bookmark for a path the app can read now; empty when
// unsupported or not allowed.
QByteArray readBookmark(const QString& path);
// Resolves a bookmark and starts access for the rest of the run; returns the
// file's current path, empty when the bookmark no longer resolves.
QString openReadBookmark(const QByteArray& bookmark);

} // namespace hamclock::file_access
