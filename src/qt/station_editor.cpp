#include "../app/station_edit.hpp"
#include "editor_appearance.hpp"
#include <QApplication>
#include <QAccessible>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

namespace {
void require_new_destination(const std::string& path) {
    struct stat status {};
    if (::lstat(path.c_str(), &status) == 0)
        throw std::runtime_error("New station destination already exists; use --station to edit it");
    if (errno != ENOENT)
        throw std::runtime_error(std::string("Inspect new station destination: ") + std::strerror(errno));
}

}

int main(int argc, char** argv) {
    if (argc != 3 || (std::string(argv[1]) != "--station" && std::string(argv[1]) != "--new-station")) {
        std::cerr << "Usage: hamclock-station-editor --station PATH\n"
                     "       hamclock-station-editor --new-station PATH\n";
        return 2;
    }
    const bool creating = std::string(argv[1]) == "--new-station";
    const std::string path = argv[2];
    if (path.empty()) { std::cerr << "A settings path is required\n"; return 2; }
    hamclock::StationSettings initial;
    try {
        if (creating) {
            require_new_destination(path);
            initial = {{0,0},{0,0},"N0CALL","system"};
        } else initial = hamclock::load_settings(path);
    } catch (const std::exception& error) {
        std::cerr << "Station editor could not open settings: " << error.what() << '\n';
        return 2;
    }
    // No user-selected Qt plugin options are interpreted by this explicit CLI.
    int qt_argc = 1;
    QApplication app(qt_argc,argv);
    QApplication::setApplicationName("HamClock Pro station editor");
    hamclock::StationEdit draft(initial);
    QDialog dialog;
    dialog.setWindowTitle("HamClock Pro · Station settings");
    dialog.setAccessibleName("Station settings");
    dialog.resize(620,430);
    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18,16,18,16);
    layout->setSpacing(12);
    auto* heading = new QLabel("Station settings",&dialog);
    QFont title_font = heading->font();title_font.setPointSizeF((title_font.pointSizeF() > 0 ? title_font.pointSizeF() : 12.0)+4);title_font.setBold(true);
    heading->setFont(title_font);
    layout->addWidget(heading);
    auto* explanation = new QLabel(creating
        ? "New unsaved draft. N0CALL and zero coordinates are placeholders—enter your station before saving."
        : "Edit your station and destination. Changes are written only when you choose Save.",&dialog);
    explanation->setWordWrap(true);
    explanation->setTextFormat(Qt::PlainText);
    layout->addWidget(explanation);
    auto* scroll = new QScrollArea(&dialog);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* form_widget = new QWidget(scroll);
    auto* form = new QFormLayout(form_widget);
    form->setContentsMargins(0,0,6,0);
    form->setVerticalSpacing(10);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    const std::array<hamclock::StationField,5> fields = {
        hamclock::StationField::Callsign,hamclock::StationField::DeLatitude,
        hamclock::StationField::DeLongitude,hamclock::StationField::DxLatitude,
        hamclock::StationField::DxLongitude};
    const std::array<const char*,5> names = {"Callsign","DE latitude","DE longitude","DX latitude","DX longitude"};
    std::array<QLineEdit*,5> edits {};
    for (std::size_t index=0;index<fields.size();++index) {
        auto* input = new QLineEdit(QString::fromStdString(draft.get(fields[index])),form_widget);
        input->setObjectName(QString("stationField%1").arg(static_cast<int>(index)));
        input->setMaxLength(index == 0 ? 16 : 64);
        input->setMinimumHeight(44);
        input->setAccessibleName(names[index]);
        input->setAccessibleDescription(index == 0 ? "1 to 16 ASCII letters, numbers, slash or hyphen"
            : (index%2 == 1 ? "Signed decimal degrees north, between -90 and 90" : "Signed decimal degrees east, between -180 and 180"));
        auto* label = new QLabel(names[index],form_widget);
        label->setBuddy(input);
        form->addRow(label,input);
        edits[index] = input;
        if (index) QWidget::setTabOrder(edits[index-1],input);
    }
    auto* appearance = new QComboBox(form_widget);
    appearance->setObjectName("appearanceChoice");
    appearance->addItem("Follow system","system");appearance->addItem("Light","light");appearance->addItem("Dark","dark");
    appearance->setCurrentIndex(appearance->findData(QString::fromStdString(initial.appearance)));
    appearance->setMinimumHeight(44);
    appearance->setAccessibleName("Appearance");
    auto* appearance_label = new QLabel("Appearance",form_widget);appearance_label->setBuddy(appearance);
    form->addRow(appearance_label,appearance);
    QWidget::setTabOrder(edits.back(),appearance);
    scroll->setWidget(form_widget);layout->addWidget(scroll,1);
    auto* error_label = new QLabel(&dialog);
    error_label->setObjectName("validationMessage");error_label->setWordWrap(true);error_label->setTextFormat(Qt::PlainText);
    error_label->setAccessibleName("Station validation or save error");error_label->setVisible(false);
    layout->addWidget(error_label);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,&dialog);
    auto* save = buttons->button(QDialogButtonBox::Save);
    auto* cancel = buttons->button(QDialogButtonBox::Cancel);
    save->setMinimumHeight(44);cancel->setMinimumHeight(44);
    save->setDefault(true);
    QWidget::setTabOrder(appearance,save);QWidget::setTabOrder(save,cancel);
    layout->addWidget(buttons);
    auto apply_appearance = [&]() {
        hamclock::apply_editor_appearance(dialog,appearance->currentData().toString());
    };
    QObject::connect(appearance,QOverload<int>::of(&QComboBox::currentIndexChanged),&dialog,[&](int){apply_appearance();});
    apply_appearance();
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,[&](){draft.cancel();dialog.reject();});
    QObject::connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]() {
        try {
            for (std::size_t index=0;index<fields.size();++index)
                if (!draft.set(fields[index],edits[index]->text().toStdString()))
                    throw std::invalid_argument("A field exceeds its supported byte length");
            if (!draft.set(hamclock::StationField::Appearance,appearance->currentData().toString().toStdString()))
                throw std::invalid_argument("Invalid appearance choice");
            const auto settings=draft.validated();
            if (creating) hamclock::save_new_settings(path,settings);
            else hamclock::save_settings(path,settings);
            dialog.accept();
        } catch (const std::exception& error) {
            error_label->setText(QString("Could not save: %1").arg(QString::fromUtf8(error.what())));
            error_label->setVisible(true);
            error_label->setAccessibleName(error_label->text());
            error_label->setAccessibleDescription(error_label->text());
            QAccessibleEvent alert(error_label,QAccessible::Alert);
            QAccessible::updateAccessibility(&alert);
        }
    });
    edits.front()->setFocus();
    (void)dialog.exec();
    return 0;
}
