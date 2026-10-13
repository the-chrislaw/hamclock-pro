#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include <QImage>
#include <QStringList>
#include <QUrl>
#include <array>
#include <vector>

namespace hamclock {

class PolledSource;

// Latest NASA SDO Sun images (AIA 193/304, HMI intensitygram). Only the shown
// channel is polled; tapping cycles to the next. Bodies that do not decode as
// an image are rejected rather than shown.
class SolarImageModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasImage READ hasImage NOTIFY imageChanged)
    Q_PROPERTY(QUrl imageUrl READ imageUrl NOTIFY imageChanged)
    Q_PROPERTY(QString channelName READ channelName NOTIFY channelChanged)
    Q_PROPERTY(QString channelCaption READ channelCaption NOTIFY channelChanged)
    Q_PROPERTY(int channelCount READ channelCount CONSTANT)
public:
    explicit SolarImageModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasImage() const { return !image_url_.isEmpty(); }
    QUrl imageUrl() const { return image_url_; }
    QString channelName() const;
    QString channelCaption() const;
    int channelCount() const { return kChannels; }
    int channel() const { return channel_; }
    Q_INVOKABLE void cycle();
    PolledSource& source(int channel) { return *sources_[static_cast<size_t>(channel)]; }

    // True when the bytes are a JPEG/PNG that decodes to a non-empty image.
    static bool valid_image(const QByteArray& body);

signals:
    void imageChanged();
    void channelChanged();

private:
    static constexpr int kChannels = 3;
    void show(int channel);
    void refreshState();

    std::array<PolledSource*, kChannels> sources_{};
    std::array<bool, kChannels> started_{};
    int channel_ = 0;
    QUrl image_url_;
    bool rejected_ = false;
};

void register_widget_solarimage(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
