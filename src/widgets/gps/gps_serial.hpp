#pragma once
#include <QByteArray>
#include <QObject>
#include <QSocketNotifier>
#include <QString>

namespace hamclock::gps {

struct SerialConfig {
    QString device;   // empty: serial GPS not configured
    int baud = 9600;
};

// Serial GPS from options.ini ([gps] device / baud). The device must be a plain
// /dev/tty* or /dev/serial/* path with no ".." and only letters, digits and "_.-/";
// the baud rate one of the usual NMEA rates. Invalid values fall back field by field.
SerialConfig parse_serial_config(const QString& device, const QString& baud, const SerialConfig& fallback);

// Read-only serial line (8N1, no flow control, raw). Never writes to the device.
// Where termios is unavailable (iPad) open() fails and the GPS card stays unconfigured.
class SerialPort : public QObject {
    Q_OBJECT
public:
    explicit SerialPort(QObject* parent = nullptr) : QObject(parent) {}
    ~SerialPort() override { close(); }
    bool open(const QString& device, int baud);
    void close();
    bool is_open() const { return fd_ >= 0; }
signals:
    void data(const QByteArray& chunk);
    void closed();   // device vanished or read failed
private:
    void onReadable();
    int fd_ = -1;
    QSocketNotifier* notifier_ = nullptr;
};

} // namespace hamclock::gps
