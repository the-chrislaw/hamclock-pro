#include "file_access.hpp"

#include <QtGlobal>

namespace hamclock::file_access {

// The sandbox sets this for every sandboxed process.
bool sandboxed() { return qEnvironmentVariableIsSet("APP_SANDBOX_CONTAINER_ID"); }

#ifndef Q_OS_MACOS
bool canChooseFiles() { return false; }
QString chooseFileToRead(const QString&) { return {}; }
QByteArray readBookmark(const QString&) { return {}; }
QString openReadBookmark(const QByteArray&) { return {}; }
#endif

} // namespace hamclock::file_access
