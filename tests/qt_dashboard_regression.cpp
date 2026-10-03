#include "../src/qt/station_dashboard.hpp"
#include "../src/qt/world_map_widget.hpp"
#include "../src/app/station_view.hpp"
#include <QApplication>
#include <QAccessible>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QEventLoop>
#include <QTimeZone>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace {
bool same(const hamclock::StationSettings& a,const hamclock::StationSettings& b){
    return a.callsign==b.callsign && a.appearance==b.appearance && a.de.latitude==b.de.latitude
        && a.de.longitude==b.de.longitude && a.dx.latitude==b.dx.latitude && a.dx.longitude==b.dx.longitude;
}
class PaintCounter : public QObject {
public:
    int paints=0;
    bool eventFilter(QObject*,QEvent* event) override {if(event->type()==QEvent::Paint)++paints;return false;}
};
QByteArray bytes(const QString& path){QFile file(path);if(!file.open(QIODevice::ReadOnly))throw std::runtime_error("fixture read failed");return file.readAll();}
}
int main(int argc,char** argv){
    QApplication app(argc,argv);QTemporaryDir temp;
    if(!temp.isValid())return 2;
    int failures=0,checks=0;
    auto check=[&](bool pass,const char* name){++checks;if(!pass){++failures;std::cerr<<"Dashboard regression failed: "<<name<<'\n';}};
    const auto directory=temp.path()+"/station";QDir().mkpath(directory);
    const auto path=directory+"/settings.conf";
    hamclock::StationSettings initial{{37.7749,-122.4194},{51.5074,-0.1278},"N0CALL","light"};
    hamclock::save_settings(path.toStdString(),initial);
    hamclock::StationDashboard dashboard(initial,path.toStdString());dashboard.show();app.processEvents();
    auto* edit=dashboard.findChild<QPushButton*>("dashboardEditStation");
    auto* map=dynamic_cast<hamclock::WorldMapWidget*>(dashboard.findChild<QWidget*>("dashboardWorldMap"));
    if(!edit || !map)throw std::runtime_error("missing dashboard controls");
    auto text=[&](const char* name){auto* label=dashboard.findChild<QLabel*>(name);if(!label)throw std::runtime_error("missing dashboard label");return label->text();};
    auto assert_view=[&](const hamclock::StationSettings& expected){
        const auto view=hamclock::make_station_view(expected);
        check(same(dashboard.stationSettings(),expected),"dashboard snapshot");
        for(const char* name:{"dashboardCallsign","dashboardDeGrid","dashboardDxGrid","dashboardDeCoordinates",
                              "dashboardDxCoordinates","dashboardDistance","dashboardBearing","dashboardRouteSummary"}) {
            auto* label=dashboard.findChild<QLabel*>(name);
            auto* accessible=QAccessible::queryAccessibleInterface(label);
            check(accessible && !label->text().isEmpty() && accessible->text(QAccessible::Name).contains(label->text()),
                  "actual accessible label name includes current station value");
        }
        auto* summary=dashboard.findChild<QLabel*>("dashboardRouteSummary");
        check(summary->accessibleDescription()==map->accessibleDescription(),"route summary exposes current map metadata");
        check(text("dashboardCallsign")==QString::fromStdString(view.callsign),"callsign matches snapshot");
        check(text("dashboardDeGrid")==QString::fromStdString(view.de_grid),"DE grid matches snapshot");
        check(text("dashboardDxGrid")==QString::fromStdString(view.dx_grid),"DX grid matches snapshot");
        check(text("dashboardDistance")==QString::fromStdString(view.distance_text),"distance matches snapshot");
        check(text("dashboardBearing")==QString::fromStdString(view.bearing_text),"bearing matches snapshot");
        check(text("dashboardRouteSummary")==map->routeSummary(),"route summary matches map");
        check(map->accessibleDescription().contains(QString::number(expected.de.latitude,'g',10)),"map DE reflects snapshot");
        check(map->accessibleDescription().contains(QString::number(expected.dx.longitude,'g',10)),"map DX reflects snapshot");
        check(dashboard.palette().color(QPalette::Window)==QColor(expected.appearance=="dark"?"#202624":"#f5f3ed"),"dashboard stored appearance applied");
    };
    assert_view(initial);
    check(edit && edit->minimumHeight()>=44,"reachable touch-sized edit button");
    const auto now=QDateTime::currentDateTimeUtc();
    auto clock=QDateTime::fromString(text("utcClock"),"yyyy-MM-dd HH:mm:ss 'UTC'");
    clock.setTimeZone(QTimeZone("UTC"));
    auto* utc_label=dashboard.findChild<QLabel*>("utcClock");
    check(QAccessible::queryAccessibleInterface(utc_label)->text(QAccessible::Name).contains(utc_label->text()),
          "actual accessible clock includes initial UTC value");
    check(clock.isValid() && std::abs(clock.secsTo(now))<=2 && text("utcClock").endsWith("UTC"),"clock presents actual UTC time");
    const auto clock_before=text("utcClock");PaintCounter counter;map->installEventFilter(&counter);
    QEventLoop clock_wait;QTimer::singleShot(1200,&clock_wait,&QEventLoop::quit);clock_wait.exec();
    check(text("utcClock")!=clock_before,"clock updates on its timer");
    check(QAccessible::queryAccessibleInterface(utc_label)->text(QAccessible::Name).contains(utc_label->text()) &&
          !QAccessible::queryAccessibleInterface(utc_label)->text(QAccessible::Name).contains(clock_before),
          "actual accessible clock updates with timer value");
    check(counter.paints==0,"clock timer does not repaint map");map->removeEventFilter(&counter);
    auto modal=[&](const std::function<void(QDialog*)>& action){
        bool visited=false;
        QTimer action_timer,watchdog;action_timer.setSingleShot(true);watchdog.setSingleShot(true);
        QObject::connect(&action_timer,&QTimer::timeout,&dashboard,[&](){
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if(!dialog || dialog->objectName()!="stationSettingsDialog"){check(false,"actual editor modal appeared");return;}
            visited=true;
            try{action(dialog);}catch(const std::exception& e){std::cerr<<e.what()<<'\n';check(false,"modal test action");dialog->reject();}
        });
        QObject::connect(&watchdog,&QTimer::timeout,&dashboard,[&](){check(false,"modal watchdog");if(auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget()))dialog->reject();});
        action_timer.start(0);watchdog.start(2000);edit->click();watchdog.stop();action_timer.stop();
        check(visited,"test exercised real editor");app.processEvents();
    };
    auto change=[&](QDialog* dialog,const char* callsign,const char* de_lat,const char* de_lon,const char* dx_lat,const char* dx_lon,const char* theme){
        const char* values[]={callsign,de_lat,de_lon,dx_lat,dx_lon};
        for(int i=0;i<5;++i){auto* field=dialog->findChild<QLineEdit*>(QString("stationField%1").arg(i));if(!field)throw std::runtime_error("missing editor field");field->setText(values[i]);}
        auto* choice=dialog->findChild<QComboBox*>("appearanceChoice");choice->setCurrentIndex(choice->findData(theme));
    };
    auto buttons=[](QDialog* dialog){auto* box=dialog->findChild<QDialogButtonBox*>();if(!box)throw std::runtime_error("missing editor buttons");return box;};
    const auto original_bytes=bytes(path);
    modal([&](QDialog* dialog){change(dialog,"W1TEST","10","20","30","40","dark");buttons(dialog)->button(QDialogButtonBox::Cancel)->click();});
    assert_view(initial);check(bytes(path)==original_bytes,"Cancel preserves disk bytes");
    modal([&](QDialog* dialog){change(dialog,"W1TEST","91","20","30","40","dark");buttons(dialog)->button(QDialogButtonBox::Save)->click();
        auto* message=dialog->findChild<QLabel*>("validationMessage");check(dialog->isVisible() && message && message->isVisible() && !message->text().isEmpty(),"invalid Save remains open with visible error");
        check(same(dashboard.stationSettings(),initial),"invalid Save leaves dashboard unchanged during modal");buttons(dialog)->button(QDialogButtonBox::Cancel)->click();});
    assert_view(initial);check(bytes(path)==original_bytes,"invalid Save preserves disk bytes");
    const hamclock::StationSettings updated{{10,20},{30,40},"W1TEST","dark"};
    modal([&](QDialog* dialog){change(dialog,"W1TEST","10","20","30","40","dark");buttons(dialog)->button(QDialogButtonBox::Save)->click();});
    assert_view(updated);check(same(hamclock::load_settings(path.toStdString()),updated),"successful Save refresh matches disk");
    const auto saved_bytes=bytes(path);
    const auto displaced=temp.path()+"/preserved-station";
    check(QDir().rename(directory,displaced),"fixture removes save parent while retaining disk bytes");
    modal([&](QDialog* dialog){change(dialog,"W2TEST","-10","-20","-30","-40","light");buttons(dialog)->button(QDialogButtonBox::Save)->click();
        auto* message=dialog->findChild<QLabel*>("validationMessage");check(dialog->isVisible() && message && message->isVisible() && !message->text().isEmpty(),"filesystem Save failure remains open with visible error");
        check(same(dashboard.stationSettings(),updated),"filesystem Save failure leaves dashboard unchanged during modal");buttons(dialog)->button(QDialogButtonBox::Cancel)->click();});
    assert_view(updated);check(bytes(displaced+"/settings.conf")==saved_bytes,"filesystem failure preserves retained source bytes");
    check(!QFile::exists(path),"filesystem failure does not invent configuration");
    std::cout<<checks<<" dashboard checks; "<<failures<<" failures\n";return failures?1:0;
}
