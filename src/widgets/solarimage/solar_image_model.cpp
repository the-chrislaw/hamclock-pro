#include "solar_image_model.hpp"
#include "../../net/polled_source.hpp"
#include <QTimer>

namespace hamclock {
namespace {
using namespace std::chrono_literals;
// SDO publishes a new "latest" image about every 15 minutes (AIA cadence is
// faster, but the web assets are refreshed on that schedule).
constexpr PollPolicy kPolicy{15min, 90min, 3h};

struct Channel { const char* cache; const char* url; const char* name; const char* caption; };
constexpr Channel kList[] = {
    {"sdo-aia-0193", "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_0193.jpg", "AIA 193 Å",
     "Corona and coronal holes, about 1.2 million K"},
    {"sdo-aia-0304", "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_0304.jpg", "AIA 304 Å",
     "Chromosphere and prominences, about 50,000 K"},
    {"sdo-hmi-ic", "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_HMIIC.jpg", "HMI intensitygram",
     "Visible light, sunspots"},
};
} // namespace

SolarImageModel::SolarImageModel(const WidgetContext& context, QObject* parent)
    : WidgetModel("solarimage", "Sun (SDO)", parent) {
    for (int i = 0; i < kChannels; ++i) {
        auto* source = new PolledSource(kList[i].cache, QUrl(kList[i].url), kPolicy, context.transport, this);
        sources_[static_cast<size_t>(i)] = source;
        connect(source, &PolledSource::updated, this, [this, i] { if (i == channel_) show(i); });
        connect(source, &PolledSource::failed, this, [this, i] { if (i == channel_) refreshState(); });
    }
    connect(this, &WidgetModel::ageChanged, this, &SolarImageModel::refreshState);
}

void SolarImageModel::start() {
    started_[static_cast<size_t>(channel_)] = true;
    sources_[static_cast<size_t>(channel_)]->start();
}

QString SolarImageModel::channelName() const { return kList[channel_].name; }
QString SolarImageModel::channelCaption() const { return kList[channel_].caption; }

bool SolarImageModel::valid_image(const QByteArray& body) {
    if (body.size() < 16) return false;
    const bool jpeg = body.startsWith("\xff\xd8\xff");
    const bool png = body.startsWith("\x89PNG");
    if (!jpeg && !png) return false;
    const QImage image = QImage::fromData(body);
    return !image.isNull() && image.width() >= 64 && image.height() >= 64;
}

void SolarImageModel::cycle() {
    channel_ = (channel_ + 1) % kChannels;
    emit channelChanged();
    if (!started_[static_cast<size_t>(channel_)]) {
        started_[static_cast<size_t>(channel_)] = true;
        sources_[static_cast<size_t>(channel_)]->start();
    }
    show(channel_); // whatever is cached for this channel, or empty until it arrives
}

void SolarImageModel::show(int channel) {
    const auto& body = sources_[static_cast<size_t>(channel)]->body();
    rejected_ = !body.isEmpty() && !valid_image(body);
    const QUrl url = !body.isEmpty() && !rejected_
        ? QUrl(QString("data:image/%1;base64,").arg(body.startsWith("\xff\xd8") ? "jpeg" : "png") +
               QString::fromLatin1(body.toBase64()))
        : QUrl();
    if (url != image_url_) { image_url_ = url; emit imageChanged(); }
    refreshState();
}

void SolarImageModel::refreshState() {
    const auto* source = sources_[static_cast<size_t>(channel_)];
    const QString label = QString("NASA SDO %1").arg(channelName());
    if (!hasImage()) {
        if (rejected_) setState("error", "NASA SDO returned something that is not an image");
        else if (!source->lastError().isEmpty()) setState("offline", "NASA SDO unreachable: " + source->lastError());
        else setState("loading", "Contacting NASA SDO…");
        return;
    }
    // Age of the download; the image itself is typically ~15 min older.
    if (source->isStale(now())) setState("stale", label + " (out of date)", source->fetchedAt());
    else setState("live", label + " · NASA/SDO science teams", source->fetchedAt());
}

void register_widget_solarimage(WidgetRegistry& registry, const WidgetContext& context) {
    auto* model = new SolarImageModel(context);
    registry.add({"solarimage", "Sun (SDO)", QUrl("qrc:/widgets/solarimage/SolarImageCard.qml"), "strip", 48}, model);
    QTimer::singleShot(0, model, &SolarImageModel::start);
}

} // namespace hamclock
