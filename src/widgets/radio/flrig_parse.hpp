#pragma once
#include <QByteArray>
#include <QString>
#include <optional>

namespace hamclock::radio {

// flrig XML-RPC (http://127.0.0.1:12345/RPC2). Only the read-only methods
// rig.get_vfo and rig.get_mode are ever requested.
QByteArray flrig_request(const char* method);
// First <string>/<i4>/<int>/<double> scalar of a methodResponse; nullopt for a
// <fault>, malformed XML or an empty value.
std::optional<QString> flrig_scalar(const QByteArray& body);
std::optional<double> flrig_frequency_hz(const QByteArray& body);
std::optional<QString> flrig_mode(const QByteArray& body);

} // namespace hamclock::radio
