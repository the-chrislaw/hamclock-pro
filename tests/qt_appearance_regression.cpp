#include "../src/qt/editor_appearance.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <iostream>
#include <stdexcept>
int main(int argc, char** argv) {
    QApplication app(argc,argv);
    QPalette system;
    system.setColor(QPalette::Window,QColor("#111819"));
    system.setColor(QPalette::WindowText,QColor("#e8eeef"));
    system.setColor(QPalette::Base,QColor("#182224"));
    system.setColor(QPalette::Text,QColor("#e8eeef"));
    system.setColor(QPalette::Button,QColor("#182224"));
    system.setColor(QPalette::ButtonText,QColor("#e8eeef"));
    system.setColor(QPalette::Mid,QColor("#809093"));
    system.setColor(QPalette::Highlight,QColor("#a0ccce"));
    system.setColor(QPalette::HighlightedText,QColor("#111819"));
    app.setPalette(system);
    QDialog dialog;dialog.resize(360,280);
    auto* layout=new QVBoxLayout(&dialog);
    auto* label=new QLabel("Station settings",&dialog);
    auto* edit=new QLineEdit(&dialog);
    auto* combo=new QComboBox(&dialog);combo->addItems({"System","Light","Dark"});
    auto* button=new QPushButton("Save",&dialog);
    edit->setMinimumHeight(44);combo->setMinimumHeight(44);button->setMinimumHeight(44);
    auto* scroll = new QScrollArea(&dialog);scroll->setWidgetResizable(true);scroll->setMinimumHeight(80);
    auto* form = new QWidget(scroll);auto* nested = new QVBoxLayout(form);
    nested->setContentsMargins(14,14,14,14);nested->addWidget(edit);
    scroll->setWidget(form);
    layout->addWidget(label);layout->addWidget(scroll);layout->addWidget(combo);layout->addWidget(button);
    dialog.show();app.processEvents();
    int failures=0,checks=0;
    auto check=[&](bool passed,const char* description){++checks;if(!passed){++failures;std::cerr<<"Appearance regression failed: "<<description<<'\n';}};
    auto palette_check=[&](const QColor& text,const QColor& base){
        check(label->palette().color(QPalette::WindowText)==text,"label text palette");
        check(edit->palette().color(QPalette::Text)==text,"input text palette");
        check(edit->palette().color(QPalette::Base)==base,"input base palette");
        check(button->palette().color(QPalette::ButtonText)==text,"button text palette");
        check(button->palette().color(QPalette::Button)==base,"button base palette");
        check(combo->palette().color(QPalette::Text)==text,"combo text palette");
    };
    for(int cycle=0;cycle<3;++cycle){
        for(const QString& mode : {QString("dark"),QString("light"),QString("system")}){
            hamclock::apply_editor_appearance(dialog,mode);app.processEvents();
            label->ensurePolished();edit->ensurePolished();button->ensurePolished();combo->ensurePolished();
            const QColor text(mode=="light"?"#25342d":mode=="dark"?"#f2f3ee":"#e8eeef");
            const QColor base(mode=="light"?"#ffffff":mode=="dark"?"#2b3330":"#182224");
            palette_check(text,base);
            const auto before=dialog.styleSheet();
            hamclock::apply_editor_appearance(dialog,mode);
            check(dialog.styleSheet()==before,"idempotent stylesheet without growth");
            edit->clearFocus();button->clearFocus();combo->clearFocus();app.processEvents();
            QImage image(edit->size(),QImage::Format_ARGB32);image.fill(Qt::magenta);edit->render(&image);
            check(image.pixelColor(edit->width()/2,edit->height()/2)==base,"rendered input background");
            QImage surface(form->size(),QImage::Format_ARGB32);surface.fill(Qt::magenta);form->render(&surface);
            const QColor background(mode=="light"?"#f5f3ed":mode=="dark"?"#202624":"#111819");
            check(surface.pixelColor(5,5)==background,"rendered nested form background");
        }
    }
    QPalette changed_system=system;
    changed_system.setColor(QPalette::Window,QColor("#faf9f6"));
    changed_system.setColor(QPalette::WindowText,QColor("#151515"));
    changed_system.setColor(QPalette::Base,QColor("#fafafa"));
    changed_system.setColor(QPalette::Text,QColor("#151515"));
    changed_system.setColor(QPalette::Button,QColor("#fafafa"));
    changed_system.setColor(QPalette::ButtonText,QColor("#151515"));
    app.setPalette(changed_system);app.processEvents();
    palette_check(QColor("#151515"),QColor("#fafafa"));
    QImage live_image(edit->size(),QImage::Format_ARGB32);edit->render(&live_image);
    check(live_image.pixelColor(edit->width()/2,edit->height()/2)==QColor("#fafafa"),"live System rendered input update");
    QImage live_form(form->size(),QImage::Format_ARGB32);form->render(&live_form);
    check(live_form.pixelColor(5,5)==QColor("#faf9f6"),"live System nested surface update");
    for(const QString& explicit_mode : {QString("light"),QString("dark")}) {
        hamclock::apply_editor_appearance(dialog,explicit_mode);app.processEvents();
        const auto fixed=dialog.styleSheet();
        app.setPalette(system);app.processEvents();
        check(dialog.styleSheet()==fixed,"explicit theme unaffected by system palette change");
        app.setPalette(changed_system);app.processEvents();
    }
    hamclock::apply_editor_appearance(dialog,"system");app.processEvents();
    check(dialog.findChildren<QObject*>("hamclockEditorAppearanceObserver",Qt::FindDirectChildrenOnly).size()==1,"single lifetime-scoped appearance observer");
    const auto style=dialog.styleSheet();const auto palette=dialog.palette();
    bool rejected=false;try{hamclock::apply_editor_appearance(dialog,"invalid");}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && dialog.styleSheet()==style && dialog.palette()==palette,"invalid theme preserves state");
    std::cout<<checks<<" appearance checks; "<<failures<<" failures\n";
    return failures?1:0;
}
