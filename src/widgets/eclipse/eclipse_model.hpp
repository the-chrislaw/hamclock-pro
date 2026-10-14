#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "../../domain/astro_eclipse.hpp"
#include <QString>
#include <QTimer>

namespace hamclock {

class AppSettings;

// Next solar eclipse anywhere on Earth, offline from the system clock.
struct EclipseSummary {
    QString kind, when, countdown, detail, visibility; // empty = not available
};
// With a station, `visibility` says whether the eclipse can be seen there.
EclipseSummary summarize_eclipse(UtcSeconds now);
EclipseSummary summarize_eclipse(UtcSeconds now, double latitude_deg, double longitude_deg);

class EclipseModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(QString kind READ kind NOTIFY changed)
    Q_PROPERTY(QString when READ when NOTIFY changed)
    Q_PROPERTY(QString countdown READ countdown NOTIFY changed)
    Q_PROPERTY(QString detail READ detail NOTIFY changed)
    Q_PROPERTY(QString visibility READ visibility NOTIFY changed)
public:
    explicit EclipseModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();
    void recompute();
    QString kind() const { return s_.kind; }
    QString when() const { return s_.when; }
    QString countdown() const { return s_.countdown; }
    QString detail() const { return s_.detail; }
    QString visibility() const { return s_.visibility; }
signals:
    void changed();
private:
    QTimer timer_;
    AppSettings* settings_ = nullptr;
    EclipseSummary s_;
};

void register_widget_eclipse(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
