#pragma once
// Short two-tone chime for the in-app pass banner on Linux (the Pi): a WAV
// written once to the cache directory and played with aplay or paplay, so no
// Qt Multimedia dependency. Does nothing on other platforms (Apple devices
// use the notification sound).
#include <QByteArray>

namespace hamclock {

// 16-bit mono PCM WAV of the chime (exposed for the regression test).
QByteArray alert_chime_wav();
void play_alert_chime();

} // namespace hamclock
