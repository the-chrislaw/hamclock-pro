#include "sat_alert_chime.hpp"
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QtGlobal>
#include <cmath>
#include <cstdint>
#if QT_CONFIG(process)
#include <QProcess>
#endif

namespace hamclock {

namespace {
constexpr int kSampleRate = 22050;
constexpr double kToneSeconds = 0.16;
constexpr double kAmplitude = 0.35 * 32767;
constexpr double kPi = 3.14159265358979323846;
constexpr double kTonesHz[] = {880.0, 1318.5}; // A5 then E6

void put_u32(QByteArray& b, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) b.append(static_cast<char>((v >> (8 * i)) & 0xff));
}
void put_u16(QByteArray& b, std::uint16_t v) {
    b.append(static_cast<char>(v & 0xff));
    b.append(static_cast<char>((v >> 8) & 0xff));
}
} // namespace

QByteArray alert_chime_wav() {
    QByteArray pcm;
    const int per_tone = static_cast<int>(kSampleRate * kToneSeconds);
    for (double hz : kTonesHz) {
        for (int i = 0; i < per_tone; ++i) {
            const double t = static_cast<double>(i) / kSampleRate;
            const double envelope = std::sin(kPi * i / per_tone); // no clicks at either end
            put_u16(pcm, static_cast<std::uint16_t>(static_cast<std::int16_t>(
                             std::lround(kAmplitude * envelope * std::sin(2 * kPi * hz * t)))));
        }
    }
    QByteArray wav("RIFF");
    put_u32(wav, static_cast<std::uint32_t>(36 + pcm.size()));
    wav.append("WAVEfmt ");
    put_u32(wav, 16);
    put_u16(wav, 1); // PCM
    put_u16(wav, 1); // mono
    put_u32(wav, kSampleRate);
    put_u32(wav, kSampleRate * 2);
    put_u16(wav, 2);
    put_u16(wav, 16);
    wav.append("data");
    put_u32(wav, static_cast<std::uint32_t>(pcm.size()));
    return wav + pcm;
}

void play_alert_chime() {
#if defined(Q_OS_LINUX) && QT_CONFIG(process)
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (dir.isEmpty() || !QDir().mkpath(dir)) return;
    const QString path = QDir(dir).filePath("pass-alert.wav");
    if (!QFile::exists(path)) {
        // Written beside and renamed into place, so a full disk never leaves a half file behind.
        QFile file(path + ".tmp");
        const QByteArray wav = alert_chime_wav();
        const bool written = file.open(QIODevice::WriteOnly) && file.write(wav) == wav.size();
        file.close();
        if (!written || !file.rename(path)) {
            qWarning("Pass alert chime: cannot write %s", qPrintable(path));
            file.remove();
            return;
        }
    }
    // aplay ships with Raspberry Pi OS; paplay covers PulseAudio/PipeWire desktops.
    for (const char* player : {"aplay", "paplay"}) {
        const QString exe = QStandardPaths::findExecutable(QString::fromLatin1(player));
        if (exe.isEmpty()) continue;
        const QStringList args = QString::fromLatin1(player) == "aplay" ? QStringList{"-q", path} : QStringList{path};
        if (QProcess::startDetached(exe, args)) return;
    }
#endif
}

} // namespace hamclock
