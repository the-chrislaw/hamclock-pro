#include "dashboard_appearance.hpp"
#include <QApplication>
#include <QColor>
#include <QDialog>
#include <QEvent>
#include <QObject>
#include <QPalette>
#include <QPointer>
#include <QTimer>
#include <QVariant>
#include <stdexcept>
namespace hamclock {
namespace {
void paint_dashboard(QDialog& dialog,const std::string& mode){
 const bool dark=mode=="dark" || (mode=="system" && QApplication::palette().color(QPalette::Window).lightness()<128);
 const QString canvas=dark?"#22282c":"#eeede9",surface=dark?"#2b3236":"#fafaf7";
 const QString ink=dark?"#e2e7e6":"#303a42",secondary=dark?"#97a4a8":"#637074";
 const QString rule=dark?"#41494b":"#e0e3df",accent=dark?"#9cb6c5":"#487991";
 QPalette palette;
 for(auto role:{QPalette::WindowText,QPalette::Text,QPalette::ButtonText})palette.setColor(role,QColor(ink));
 palette.setColor(QPalette::Window,QColor(canvas));palette.setColor(QPalette::Base,QColor(surface));
 palette.setColor(QPalette::Button,QColor(surface));palette.setColor(QPalette::Mid,QColor(rule));
 palette.setColor(QPalette::Highlight,QColor(accent));palette.setColor(QPalette::HighlightedText,QColor(dark?"#22282c":"#ffffff"));
 dialog.setPalette(palette);
 dialog.setProperty("dashboardResolvedDark",dark);
 dialog.setStyleSheet(QStringLiteral(
  "QDialog#stationDashboard { background-color: %1; color: %3; }"
  "QFrame[dashboardCard=\"true\"] { background-color: %2; border: 1px solid %5; border-radius: 12px; }"
  "QFrame[dashboardSection=\"true\"], QWidget#dashboardWorkspace, QWidget#spotList { background: transparent; border: none; }"
  "QLabel { background: transparent; color: %3; border: none; }"
  "QLabel[dashboardSecondary=\"true\"] { color: %4; }"
  "QWidget#dashboardMasthead QLabel[dashboardSecondary=\"true\"], QLabel#dashboardStatus { color: %8; }"
  "QFrame[dashboardDivider=\"true\"] { background-color: %5; border: none; }"
  "QScrollArea { background: transparent; border: none; }"
  "QPushButton, QComboBox { color: %3; background-color: %2; border: 1px solid %4; border-radius: 7px; padding: 5px 9px; }"
  "QPushButton:focus, QComboBox:focus { border: 2px solid %6; }"
  "QPushButton:pressed { background-color: %1; }"
  "QComboBox QAbstractItemView { background-color: %2; color: %3; selection-background-color: %6; selection-color: %7; }")
  .arg(canvas,surface,ink,secondary,rule,accent,dark?"#22282c":"#ffffff",dark?secondary:QString("#606d71")));
}
class DashboardAppearanceObserver final:public QObject{
 QPointer<QDialog> dialog_;bool pending_=false;
public:
 explicit DashboardAppearanceObserver(QDialog& dialog):QObject(&dialog),dialog_(&dialog){setObjectName("dashboardAppearanceObserver");qApp->installEventFilter(this);}
protected:
 bool eventFilter(QObject*,QEvent* event)override{
  if(event->type()==QEvent::ApplicationPaletteChange && !pending_ && dialog_ && dialog_->property("dashboardAppearanceMode").toString()=="system"){
   pending_=true;QTimer::singleShot(0,this,[this](){pending_=false;if(dialog_ && dialog_->property("dashboardAppearanceMode").toString()=="system")paint_dashboard(*dialog_,"system");});
  }
  return false;
 }
};
}
void apply_dashboard_appearance(QDialog& dialog,const std::string& mode){
 if(mode!="system" && mode!="light" && mode!="dark")throw std::invalid_argument("Invalid dashboard appearance");
 if(!dialog.findChild<QObject*>("dashboardAppearanceObserver",Qt::FindDirectChildrenOnly))
  new DashboardAppearanceObserver(dialog);
 dialog.setProperty("dashboardAppearanceMode",QString::fromStdString(mode));paint_dashboard(dialog,mode);
}
}
