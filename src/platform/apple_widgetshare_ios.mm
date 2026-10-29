#include "apple_widgetshare.hpp"

#include <QDateTime>
#import <Foundation/Foundation.h>

namespace hamclock {

// NSUserDefaults of the App Group: readable by the widget extension while the
// device is locked (CarPlay, StandBy), unlike class A/B protected files. Without
// the App Group entitlement (unsigned Simulator builds) the suite is app-local,
// so the widgets simply show their "set your station" state.
bool nativeShareForWidgets(const WidgetShare& share) {
    NSUserDefaults* defaults = [[NSUserDefaults alloc] initWithSuiteName:@(kWidgetAppGroup)];
    if (!defaults) return false;
    const QVariantMap fields = widgetShareFields(share, QDateTime::currentSecsSinceEpoch());
    for (auto it = fields.cbegin(); it != fields.cend(); ++it) {
        NSString* key = it.key().toNSString();
        const QVariant& v = it.value();
        switch (v.typeId()) {
        case QMetaType::Bool: [defaults setBool:v.toBool() forKey:key]; break;
        case QMetaType::Int:
        case QMetaType::LongLong: [defaults setObject:@(v.toLongLong()) forKey:key]; break;
        case QMetaType::Double: [defaults setDouble:v.toDouble() forKey:key]; break;
        case QMetaType::QStringList: {
            NSMutableArray* list = [NSMutableArray array];
            for (const QString& s : v.toStringList()) [list addObject:s.toNSString()];
            [defaults setObject:list forKey:key];
            break;
        }
        default: [defaults setObject:v.toString().toNSString() forKey:key]; break;
        }
    }
    return true;
}

} // namespace hamclock
