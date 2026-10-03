#pragma once
#include <string>
class QDialog;
namespace hamclock {
// Original approved warm/slate dashboard theme. System selects from current
// application palette brightness and follows live application palette changes.
void apply_dashboard_appearance(QDialog& dialog,const std::string& mode);
}
