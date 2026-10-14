#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "adif_parse.hpp"
#include <QFileSystemWatcher>
#include <QSet>
#include <QVariantList>

namespace hamclock {

class AppSettings;

// Station log read from a local ADIF file (path in Settings). The file is
// reloaded when it changes; nothing is ever written to it. Offline by design.
class AdifModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY logChanged)
    Q_PROPERTY(int count READ count NOTIFY logChanged)
    Q_PROPERTY(QVariantList recent READ recent NOTIFY logChanged)
public:
    explicit AdifModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();
    bool hasData() const { return loaded_; }
    int count() const { return int(qsos_.size()); }
    QVariantList recent() const;  // newest first: {call, band, mode, grid, age}
    // True when the callsign (portable suffixes ignored) is already in the log.
    Q_INVOKABLE bool worked(const QString& call) const;
    void load(const QString& path);

signals:
    void logChanged();

private:
    void reload();
    AppSettings* settings_;
    QFileSystemWatcher watcher_;
    QString path_;
    bool loaded_ = false;
    std::vector<adif::Qso> qsos_;  // newest first
    QSet<QString> calls_;
};

void register_widget_adif(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
