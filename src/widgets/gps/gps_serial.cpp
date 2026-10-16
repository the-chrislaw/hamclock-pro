#include "gps_serial.hpp"
#include <QRegularExpression>

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
#define HAMCLOCK_HAVE_TERMIOS 1
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace hamclock::gps {

SerialConfig parse_serial_config(const QString& device, const QString& baud, const SerialConfig& fallback) {
    SerialConfig out = fallback;
    static const QRegularExpression ok(R"(^/dev/(tty|serial/)[A-Za-z0-9_./-]{1,64}$)");
    const QString d = device.trimmed();
    if (ok.match(d).hasMatch() && !d.contains("..")) out.device = d;
    bool good = false;
    const int b = baud.trimmed().toInt(&good);
    if (good && (b == 4800 || b == 9600 || b == 19200 || b == 38400 || b == 57600 || b == 115200)) out.baud = b;
    return out;
}

#ifdef HAMCLOCK_HAVE_TERMIOS
namespace {
speed_t speed_for(int baud) {
    switch (baud) {
    case 4800: return B4800;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    default: return B9600;
    }
}
}

bool SerialPort::open(const QString& device, int baud) {
    close();
    const int fd = ::open(device.toLocal8Bit().constData(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return false;
    termios tio{};
    if (tcgetattr(fd, &tio) != 0) { ::close(fd); return false; }
    cfmakeraw(&tio);
    tio.c_cflag |= CLOCAL | CREAD;
    tio.c_cflag &= ~(CSTOPB | CRTSCTS);
    cfsetispeed(&tio, speed_for(baud));
    cfsetospeed(&tio, speed_for(baud));
    if (tcsetattr(fd, TCSANOW, &tio) != 0) { ::close(fd); return false; }
    fd_ = fd;
    notifier_ = new QSocketNotifier(fd_, QSocketNotifier::Read, this);
    connect(notifier_, &QSocketNotifier::activated, this, &SerialPort::onReadable);
    return true;
}

void SerialPort::close() {
    delete notifier_;
    notifier_ = nullptr;
    if (fd_ >= 0) ::close(fd_);
    fd_ = -1;
}

void SerialPort::onReadable() {
    char buf[512];
    const ssize_t n = ::read(fd_, buf, sizeof buf);
    if (n > 0) { emit data(QByteArray(buf, int(n))); return; }
    if (n < 0 && (errno == EAGAIN || errno == EINTR)) return;
    close();
    emit closed();
}
#else
bool SerialPort::open(const QString&, int) { return false; }
void SerialPort::close() {}
void SerialPort::onReadable() {}
#endif

} // namespace hamclock::gps
