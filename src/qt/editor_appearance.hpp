#pragma once
class QDialog;
class QString;
namespace hamclock {
// Apply bounded original styles for "system", "light" or "dark".
// Unknown modes throw std::invalid_argument before changing the dialog.
// System inherits QApplication's palette; explicit themes use literal colors
// in the stylesheet so native dark controls cannot override light surfaces.
void apply_editor_appearance(QDialog& dialog, const QString& mode);
}
