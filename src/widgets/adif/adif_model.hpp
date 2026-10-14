#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "adif_parse.hpp"
#include <QFileSystemWatcher>
#include <QSet>
#include <QVariantList>
#include <functional>

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
    // NADXCC / NABand: true when no QSO in the log is with the call's DXCC entity (any band),
    // or none with that entity on that band. Unknown entities and an unloaded log report false
    // (never claim "needed" without data).
    Q_INVOKABLE bool entityNeeded(const QString& call) const;
    Q_INVOKABLE bool bandSlotNeeded(const QString& call, const QString& band) const;
    // Call -> DXCC primary prefix; default is the bundled table (tests inject one).
    void setEntityResolver(std::function<QString(const QString&)> resolver);
    void load(const QString& path);
    void setMapVisible(bool visible);  // per-source map on/off (default on)
    bool mapVisible() const { return map_visible_; }
    ~AdifModel() override;

signals:
    void logChanged();

private:
    void reload();
    void publishOverlay();
    bool map_visible_ = true;
    AppSettings* settings_;
    QFileSystemWatcher watcher_;
    QString path_;
    bool loaded_ = false;
    std::vector<adif::Qso> qsos_;  // newest first
    QSet<QString> calls_;
    QSet<QString> entities_, slots_;  // primary prefix; "prefix|band"
    std::function<QString(const QString&)> entity_of_;
    QString entityFor(const QString& call) const;
};

void register_widget_adif(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
