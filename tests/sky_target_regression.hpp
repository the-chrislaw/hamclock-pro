#pragma once
#include <QObject>
#include <QVariantMap>

class FakeSource : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap detail READ detail NOTIFY detailChanged)
public:
    QVariantMap detail() const { return d; }
    void set(const QVariantMap& m) { d = m; emit detailChanged(); }
    QVariantMap d;
signals:
    void detailChanged();
};
