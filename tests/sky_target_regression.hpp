#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class FakeSource : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList skyTrack READ skyTrack NOTIFY changed)
    Q_PROPERTY(QVariantMap detail READ detail NOTIFY detailChanged)
public:
    QVariantMap detail() const { return d; }
    void set(const QVariantMap& m) { d = m; emit detailChanged(); }
    QVariantList skyTrack() const { return t; }
    void setTrack(const QVariantList& l) { t = l; emit changed(); }
    QVariantMap d;
    QVariantList t;
signals:
    void detailChanged();
    void changed();
};
