#include "editor_appearance.hpp"
#include <QApplication>
#include <QColor>
#include <QDialog>
#include <QEvent>
#include <QObject>
#include <QPalette>
#include <QPointer>
#include <QString>
#include <stdexcept>
namespace hamclock {
namespace {
void paint_appearance(QDialog& dialog, const QString& mode) {
    const bool system = mode == "system", dark = mode == "dark";
    QPalette palette = system ? QApplication::palette() : QPalette();
    if (!system) {
        palette.setColor(QPalette::Window,QColor(dark ? "#202624" : "#f5f3ed"));
        palette.setColor(QPalette::WindowText,QColor(dark ? "#f2f3ee" : "#25342d"));
        palette.setColor(QPalette::Base,QColor(dark ? "#2b3330" : "#ffffff"));
        palette.setColor(QPalette::AlternateBase,palette.color(QPalette::Window));
        palette.setColor(QPalette::Text,palette.color(QPalette::WindowText));
        palette.setColor(QPalette::Button,palette.color(QPalette::Base));
        palette.setColor(QPalette::ButtonText,palette.color(QPalette::Text));
        palette.setColor(QPalette::Highlight,QColor(dark ? "#9ac9b0" : "#276548"));
        palette.setColor(QPalette::HighlightedText,QColor(dark ? "#17291e" : "#ffffff"));
        palette.setColor(QPalette::Mid,QColor(dark ? "#8d9b93" : "#84968b"));
        for (const auto role : {QPalette::WindowText,QPalette::Text,QPalette::ButtonText})
            palette.setColor(QPalette::Disabled,role,QColor(dark ? "#adb7b0" : "#596b60"));
    }
    auto color = [&](QPalette::ColorRole role) {return palette.color(role).name(QColor::HexRgb);};
    const QString surface = color(QPalette::Window), text = color(QPalette::Text);
    // Stylesheet palette() expressions can retain old native palette values.
    // Resolve current colors explicitly, including nested scroll/form surfaces.
    const QString controls = QStringLiteral(
        "QLineEdit, QComboBox, QPushButton { padding: 6px 10px; border: 1px solid %1; border-radius: 7px; background-color: %2; color: %3; }"
        "QLineEdit:focus, QComboBox:focus, QPushButton:focus { border: 2px solid %4; }"
        "QPushButton:pressed { background-color: %5; }"
        "QLineEdit:disabled, QComboBox:disabled, QPushButton:disabled { color: %6; }"
        "QComboBox QAbstractItemView { background-color: %2; color: %3; selection-background-color: %4; selection-color: %7; }");
    const QString style = QStringLiteral("QDialog, QWidget { background-color: %1; color: %2; }")
        .arg(surface,color(QPalette::WindowText))
        + controls.arg(color(QPalette::Mid),color(QPalette::Base),text,
                       color(QPalette::Highlight),color(QPalette::AlternateBase),
                       palette.color(QPalette::Disabled,QPalette::Text).name(QColor::HexRgb),
                       color(QPalette::HighlightedText));
    dialog.setPalette(system ? QPalette() : palette);
    dialog.setStyleSheet(style); // Always replace; never append or grow styles.
}
class AppearanceObserver final : public QObject {
public:
    explicit AppearanceObserver(QDialog& dialog) : QObject(&dialog), dialog_(&dialog) {
        setObjectName(QStringLiteral("hamclockEditorAppearanceObserver"));
        if (qApp) qApp->installEventFilter(this);
    }
    void setMode(const QString& mode) {mode_ = mode; refresh();}
protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == qApp && event->type() == QEvent::ApplicationPaletteChange && mode_ == "system")
            refresh();
        return false;
    }
private:
    void refresh() {
        if (refreshing_ || !dialog_) return;
        refreshing_ = true;
        paint_appearance(*dialog_,mode_);
        refreshing_ = false;
    }
    QPointer<QDialog> dialog_;
    QString mode_ = "system";
    bool refreshing_ = false;
};
}
void apply_editor_appearance(QDialog& dialog, const QString& mode) {
    if (mode != "system" && mode != "light" && mode != "dark")
        throw std::invalid_argument("Invalid editor appearance");
    AppearanceObserver* observer = nullptr;
    for (auto* child : dialog.children())
        if ((observer = dynamic_cast<AppearanceObserver*>(child))) break;
    if (!observer) observer = new AppearanceObserver(dialog);
    observer->setMode(mode);
}
}
