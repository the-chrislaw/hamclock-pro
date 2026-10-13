#pragma once
#include "../widget_model.hpp"
#include "../widget_registry.hpp"
#include "storms_parse.hpp"
#include <QVariantList>

namespace hamclock {

// Active tropical cyclones from the National Hurricane Center. An empty list
// from a successful fetch is shown as "No active storms", never as missing data.
class StormsModel : public WidgetModel {
    Q_OBJECT
    Q_PROPERTY(bool hasData READ hasData NOTIFY stormsChanged)
    Q_PROPERTY(int count READ count NOTIFY stormsChanged)
    Q_PROPERTY(QString headline READ headline NOTIFY stormsChanged)
    Q_PROPERTY(QString headlineIntensity READ headlineIntensity NOTIFY stormsChanged)
    Q_PROPERTY(QString headlineDetail READ headlineDetail NOTIFY stormsChanged)
    Q_PROPERTY(int headlineCategory READ headlineCategory NOTIFY stormsChanged)
    Q_PROPERTY(QVariantList others READ others NOTIFY stormsChanged)
public:
    explicit StormsModel(const WidgetContext& context, QObject* parent = nullptr);
    void start();

    bool hasData() const { return feed_.ok; }
    int count() const { return static_cast<int>(feed_.storms.size()); }
    QString headline() const;        // storm name: "Simon"
    QString headlineIntensity() const;  // "Hurricane cat 1 · 80 kt · 977 mb"
    QString headlineDetail() const;  // "moving W 2 kt · 20.2N 107.5W · issued 13 21:00Z"
    int headlineCategory() const;    // Saffir-Simpson 0-5
    QVariantList others() const;     // remaining storms: {text, detail}

    PolledSource& source() { return *source_; }

signals:
    void stormsChanged();

private:
    void refreshState();

    PolledSource* source_;
    storms::Feed feed_;
};

void register_widget_storms(WidgetRegistry& registry, const WidgetContext& context);

} // namespace hamclock
