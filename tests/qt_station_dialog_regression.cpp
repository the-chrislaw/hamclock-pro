#include "../src/qt/station_editor_dialog.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProxyStyle>
#include <QTimer>
#include <fstream>
#include <functional>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
// Simulate the documented Mac button ordering under the offscreen platform;
// this reproduces the native StyleChange tab-chain regression deterministically.
class MacButtonLayout : public QProxyStyle {
public:
 MacButtonLayout():QProxyStyle("Fusion"){}
 int styleHint(StyleHint hint,const QStyleOption* option=nullptr,const QWidget* widget=nullptr,
               QStyleHintReturn* data=nullptr) const override {
  if(hint==QStyle::SH_DialogButtonLayout)return QDialogButtonBox::MacLayout;
  return QProxyStyle::styleHint(hint,option,widget,data);
 }
};
int main(int argc,char** argv){
 QApplication app(argc,argv);
 if(app.arguments().contains("--mac-button-layout"))app.setStyle(new MacButtonLayout);
 int failures=0,checks=0;
 auto check=[&](bool okay,const char* name){++checks;if(!okay){++failures;std::cerr<<"Station dialog regression failed: "<<name<<'\n';}};
 char pattern[]="/tmp/hamclock-dialog-XXXXXX";char* created=mkdtemp(pattern);
 if(!created){std::cerr<<"Cannot create dialog fixture directory\n";return 1;}
 const std::string directory(created),path=directory+"/station.conf";
 const hamclock::StationSettings initial{{37.7,-122.4},{51.5,-0.1},"N0CALL","system"};
 auto run=[&](const std::string& target,bool creating,const std::function<void(QDialog*)>& action){
  QTimer::singleShot(0,&app,[&](){
   auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
   check(dialog && dialog->objectName()=="stationSettingsDialog","actual station modal available");
   if(!dialog)return;
   action(dialog);
   // Guard against a broken default-key action hanging the test suite.
   if(dialog->isVisible()){check(false,"test action must close modal");dialog->reject();}
  });
  return hamclock::show_station_editor(nullptr,target,initial,creating);
 };
 auto buttons=[](QDialog* dialog){return dialog->findChild<QDialogButtonBox*>();};
 auto field=[](QDialog* dialog,int index){return dialog->findChild<QLineEdit*>(QString("stationField%1").arg(index));};
 try{
  auto focus_cancel=run(path,true,[&](QDialog* dialog){
   auto* theme=dialog->findChild<QComboBox*>("appearanceChoice");
   auto* save=buttons(dialog)->button(QDialogButtonBox::Save);
   auto* cancel=buttons(dialog)->button(QDialogButtonBox::Cancel);
   for(const QString& mode:{QString("dark"),QString("light"),QString("system")}) {
   theme->setCurrentIndex(theme->findData(mode));
   if(mode=="system") {
    QPalette live=app.palette();live.setColor(QPalette::Window,QColor("#fafafa"));
    app.setPalette(live);app.processEvents();
   }
   theme->setFocus();app.processEvents();
   QKeyEvent tab(QEvent::KeyPress,Qt::Key_Tab,Qt::NoModifier);QApplication::sendEvent(theme,&tab);
   check(QApplication::focusWidget()==save,"appearance Tab reaches actual Save");
   QKeyEvent next(QEvent::KeyPress,Qt::Key_Tab,Qt::NoModifier);QApplication::sendEvent(save,&next);
   check(QApplication::focusWidget()==cancel,"Save Tab reaches actual Cancel");
   QKeyEvent previous(QEvent::KeyPress,Qt::Key_Backtab,Qt::ShiftModifier);QApplication::sendEvent(cancel,&previous);
   check(QApplication::focusWidget()==save,"Cancel Shift Tab reaches actual Save");
   }
   cancel->click();
  });check(!focus_cancel,"keyboard traversal Cancel saves nothing");
  auto saved=run(path,true,[&](QDialog* dialog){
   auto* callsign=field(dialog,0);check(callsign && callsign->text()=="N0CALL","baseline displayed");
   callsign->setText("N1TEST");field(dialog,1)->setText("42");
   auto* theme=dialog->findChild<QComboBox*>("appearanceChoice");theme->setCurrentIndex(theme->findData("dark"));
   check(buttons(dialog)->button(QDialogButtonBox::Save)->isDefault(),"Save is default action");
   QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);QApplication::sendEvent(callsign,&enter);
  });
  check(saved && saved->callsign=="N1TEST" && saved->de.latitude==42 && saved->appearance=="dark","returned settings after default Save");
  check(hamclock::load_settings(path).callsign=="N1TEST","new settings persisted");
  struct stat status{};check(stat(path.c_str(),&status)==0 && (status.st_mode&0777)==0600,"new configuration mode0600");
  const auto before=hamclock::load_settings(path);
  auto cancelled=run(path,false,[&](QDialog* dialog){field(dialog,0)->setText("N2CANCEL");buttons(dialog)->button(QDialogButtonBox::Cancel)->click();});
  check(!cancelled && hamclock::load_settings(path).callsign==before.callsign,"Cancel does not return or save settings");
  cancelled=run(path,false,[&](QDialog* dialog){field(dialog,1)->setText("88");QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QApplication::sendEvent(dialog,&escape);});
  check(!cancelled && hamclock::load_settings(path).de.latitude==before.de.latitude,"Escape preserves existing settings");
  cancelled=run(path,false,[&](QDialog* dialog){
   field(dialog,1)->setText("91");buttons(dialog)->button(QDialogButtonBox::Save)->click();
   auto* error=dialog->findChild<QLabel*>("validationMessage");
   check(dialog->isVisible() && error->isVisible() && error->text().contains("Could not save"),"invalid Save remains open with error");
   check(field(dialog,1)->text()=="91","failed validation retains draft");
   check(hamclock::load_settings(path).de.latitude==before.de.latitude,"invalid Save preserves configuration");
   buttons(dialog)->button(QDialogButtonBox::Cancel)->click();
  });check(!cancelled,"invalid then Cancel returns no saved result");
  saved=run(path,false,[&](QDialog* dialog){field(dialog,0)->setText("N3EDIT");buttons(dialog)->button(QDialogButtonBox::Save)->click();});
  check(saved && saved->callsign=="N3EDIT" && hamclock::load_settings(path).callsign=="N3EDIT","existing configuration successful Save");
  const auto race=directory+"/race.conf";
  cancelled=run(race,true,[&](QDialog* dialog){
   hamclock::save_new_settings(race,initial);field(dialog,0)->setText("N4RACE");buttons(dialog)->button(QDialogButtonBox::Save)->click();
   check(dialog->isVisible() && field(dialog,0)->text()=="N4RACE" && dialog->findChild<QLabel*>("validationMessage")->isVisible(),"new destination race retains draft with error");
   check(hamclock::load_settings(race).callsign==initial.callsign,"new destination race cannot overwrite");
   buttons(dialog)->button(QDialogButtonBox::Cancel)->click();
  });check(!cancelled,"race failure returns no saved result");unlink(race.c_str());
  cancelled=run(directory+"/missing/new.conf",true,[&](QDialog* dialog){
   buttons(dialog)->button(QDialogButtonBox::Save)->click();
   check(dialog->isVisible()&&dialog->findChild<QLabel*>("validationMessage")->isVisible(),"filesystem failure remains open");
   buttons(dialog)->button(QDialogButtonBox::Cancel)->click();
  });check(!cancelled,"filesystem failure returns no saved result");
 }catch(const std::exception& error){++failures;std::cerr<<"Unexpected dialog test exception: "<<error.what()<<'\n';}
 unlink(path.c_str());check(rmdir(directory.c_str())==0,"fixture cleanup no leaked temporary files");
 std::cout<<checks<<" station modal checks; "<<failures<<" failures\n";return failures?1:0;
}
