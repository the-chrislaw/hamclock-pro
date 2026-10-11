#include "../src/qt/station_dashboard.hpp"
#include "../src/qt/stopwatch_dialog.hpp"
#include "../src/qt/countdown_dialog.hpp"
#include "../src/qt/world_map_widget.hpp"
#include "../src/app/station_view.hpp"
#include <QApplication>
#include <QAccessible>
#include <QComboBox>
#include <QSpinBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QEventLoop>
#include <QTimeZone>
#include <QFile>
#include <QFrame>
#include <QImage>
#include <QKeyEvent>
#include <QStyleHints>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <functional>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stdexcept>
namespace {
bool same(const hamclock::StationSettings& a,const hamclock::StationSettings& b){
    return a.callsign==b.callsign && a.appearance==b.appearance && a.de.latitude==b.de.latitude
        && a.de.longitude==b.de.longitude && a.dx.latitude==b.dx.latitude && a.dx.longitude==b.dx.longitude;
}
double luminance(const QColor& color){auto channel=[](double value){return value<=0.04045?value/12.92:std::pow((value+0.055)/1.055,2.4);};return .2126*channel(color.redF())+.7152*channel(color.greenF())+.0722*channel(color.blueF());}
double contrast(const QColor& a,const QColor& b){const auto x=luminance(a),y=luminance(b);return (std::max(x,y)+.05)/(std::min(x,y)+.05);}
class PaintCounter : public QObject {
public:
    int paints=0;
    bool eventFilter(QObject*,QEvent* event) override {if(event->type()==QEvent::Paint)++paints;return false;}
};
QByteArray bytes(const QString& path){QFile file(path);if(!file.open(QIODevice::ReadOnly))throw std::runtime_error("fixture read failed");return file.readAll();}
}
int main(int argc,char** argv){
    QApplication app(argc,argv);app.styleHints()->setTabFocusBehavior(Qt::TabFocusAllControls);QTemporaryDir temp;
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
        check(dashboard.palette().color(QPalette::Window)==QColor(expected.appearance=="dark"?"#22282c":"#eeede9"),"dashboard stored appearance applied");
    };
    assert_view(initial);
    check(edit && edit->minimumHeight()>=44,"reachable touch-sized edit button");
    const auto now=QDateTime::currentDateTimeUtc();
    auto clock=QDateTime::fromString(text("utcDate").left(10)+" "+text("utcClock"),"yyyy-MM-dd HH:mm:ss");
    clock.setTimeZone(QTimeZone("UTC"));
    auto* utc_label=dashboard.findChild<QLabel*>("utcClock");
    check(QAccessible::queryAccessibleInterface(utc_label)->text(QAccessible::Name).contains(utc_label->text()),
          "actual accessible clock includes initial UTC value");
    check(clock.isValid() && std::abs(clock.secsTo(now))<=2 && text("utcDate").endsWith("UTC"),"clock presents actual UTC time");
    const auto clock_before=text("utcClock");PaintCounter counter;map->installEventFilter(&counter);
    QEventLoop clock_wait;QTimer::singleShot(2100,&clock_wait,&QEventLoop::quit);clock_wait.exec();
    check(text("utcClock")!=clock_before,"clock updates on its timer");
    check(QAccessible::queryAccessibleInterface(utc_label)->text(QAccessible::Name).contains(utc_label->text()) &&
          !QAccessible::queryAccessibleInterface(utc_label)->text(QAccessible::Name).contains(clock_before),
          "actual accessible clock updates with timer value");
    check(counter.paints==0,"clock timer does not repaint map");map->removeEventFilter(&counter);
    auto* sidebar=dashboard.findChild<QFrame*>("stationSidebar");
    auto* instruments=dashboard.findChild<QFrame*>("dashboardInstruments");
    auto* map_card=dashboard.findChild<QFrame*>("dashboardMapCard");
    auto* appearance=dashboard.findChild<QComboBox*>("dashboardAppearance");
    auto* paths=dashboard.findChild<QPushButton*>("dashboardRouteToggle");
    auto* stopwatch=dashboard.findChild<QPushButton*>("dashboardStopwatch");
    auto* countdown=dashboard.findChild<QPushButton*>("dashboardCountdown");
    check(countdown && countdown->minimumHeight()>=44 && countdown->minimumWidth()>=44,"reachable touch-sized countdown entry");
    check(stopwatch && stopwatch->minimumHeight()>=44 && stopwatch->minimumWidth()>=44,"reachable touch-sized stopwatch entry");
    check(sidebar && sidebar->isAncestorOf(utc_label),"UTC clock belongs to grouped station sidebar");
    for(const char* name:{"dashboardFluxPaneState","dashboardPropagationPaneState","dashboardSunPaneState","dashboardSpaceWeatherPaneState"})
        check(text(name)=="Unavailable","unimplemented instruments state unavailable explicitly");
    check(text("dashboardSpotState")=="Not connected","cluster contains honest connection state");
    check(paths && paths->height()>=44 && appearance->height()>=44,"map and appearance actions have real 44px targets");
    auto bounds=[&](QWidget* widget){return QRect(widget->mapTo(&dashboard,QPoint(0,0)),widget->size());};
    for(const QSize size:{QSize(800,480),QSize(1280,800)}){
        dashboard.resize(size);app.processEvents();
        check(dashboard.size()==size,"requested compact and large client sizes remain supported");
        check(dashboard.rect().contains(bounds(sidebar)) && dashboard.rect().contains(bounds(instruments)) && dashboard.rect().contains(bounds(map_card)),"all main cards fit inside client bounds");
        check(bounds(instruments).bottom()<bounds(map_card).top(),"instrument strip sits above map card");
        check(map_card->height()>instruments->height(),"map card dominates workspace vertically");
        auto* utility=dashboard.findChild<QLabel*>("dashboardSpaceWeatherPaneState");check(utility->fontMetrics().horizontalAdvance(utility->text())<=utility->width(),"utility unavailable state remains unclipped");
        check(map->mapBounds().width()>100 && map->mapBounds().height()>50,"real geography remains usable at both sizes");
    }
    // Exercise the actual dashboard entry and caller-owned timer across two
    // modal lifetimes; these event-loop checks are distinct from injected view tests.
    QString held_elapsed;
    bool first_open=false, second_open=false;
    QTimer::singleShot(0,&dashboard,[&](){
        auto* dialog=dynamic_cast<hamclock::StopwatchDialog*>(QApplication::activeModalWidget());
        first_open=dialog!=nullptr;check(first_open,"dashboard opens actual stopwatch dialog");
        if(!dialog)return;
        check(dialog->perform(hamclock::StopwatchControl::Run),"dashboard stopwatch starts");
        QTimer::singleShot(80,dialog,[&,dialog](){
            check(dialog->perform(hamclock::StopwatchControl::Lap),"dashboard stopwatch holds display");
            auto* elapsed=dialog->findChild<QLabel*>("stopwatchElapsed");
            held_elapsed=elapsed?elapsed->text():QString();
            check(!held_elapsed.isEmpty(),"dashboard stopwatch has held value");
            dialog->reject();
        });
    });
    stopwatch->click();app.processEvents();
    check(first_open && QApplication::focusWidget()==stopwatch,"stopwatch closes and returns dashboard focus");
    QEventLoop closed_wait;QTimer::singleShot(90,&closed_wait,&QEventLoop::quit);closed_wait.exec();
    QTimer::singleShot(0,&dashboard,[&](){
        auto* dialog=dynamic_cast<hamclock::StopwatchDialog*>(QApplication::activeModalWidget());
        second_open=dialog!=nullptr;check(second_open,"dashboard reopens stopwatch");
        if(!dialog)return;
        auto* elapsed=dialog->findChild<QLabel*>("stopwatchElapsed");
        check(elapsed && elapsed->text()==held_elapsed,"held display survives closing and reopening");
        check(dialog->perform(hamclock::StopwatchControl::Run),"Run releases hold after reopening");
        check(elapsed && elapsed->text()>held_elapsed,"shared running time includes time with dialog closed");
        check(dialog->perform(hamclock::StopwatchControl::Reset),"dashboard stopwatch resets explicitly");
        dialog->reject();
    });
    stopwatch->click();app.processEvents();
    check(second_open,"second stopwatch lifecycle observed");
    bool countdown_first=false,countdown_second=false;
    QTimer::singleShot(0,&dashboard,[&](){
        auto* dialog=dynamic_cast<hamclock::CountdownDialog*>(QApplication::activeModalWidget());
        countdown_first=dialog!=nullptr;check(countdown_first,"dashboard opens actual countdown dialog");
        if(!dialog)return;
        auto* minutes=dialog->findChild<QSpinBox*>("countdownMinutes");
        check(minutes && minutes->value()==1,"countdown initial replacement duration is explicit one minute");
        if(minutes)minutes->setValue(2);
        check(dialog->perform(hamclock::CountdownControl::SetAndReset),"countdown explicit configuration applies");
        check(dialog->perform(hamclock::CountdownControl::Run),"dashboard countdown starts");
        check(dialog->perform(hamclock::CountdownControl::Pause),"dashboard countdown pauses");
        dialog->reject();
    });
    countdown->click();app.processEvents();
    check(countdown_first && QApplication::focusWidget()==countdown,"countdown closes and restores dashboard focus");
    QTimer::singleShot(0,&dashboard,[&](){
        auto* dialog=dynamic_cast<hamclock::CountdownDialog*>(QApplication::activeModalWidget());
        countdown_second=dialog!=nullptr;check(countdown_second,"dashboard reopens shared countdown");
        if(!dialog)return;
        auto* configured=dialog->findChild<QLabel*>("countdownConfigured");
        auto* run=dialog->findChild<QPushButton*>("countdownRun");
        check(configured && configured->text().contains("00:02:00"),"configured countdown survives view closure");
        check(run && run->text()=="Resume","paused countdown survives view closure");
        check(dialog->perform(hamclock::CountdownControl::Run),"reopened countdown resumes");
        check(dialog->perform(hamclock::CountdownControl::Reset),"reopened countdown explicitly resets");
        dialog->reject();
    });
    countdown->click();app.processEvents();
    check(countdown_second,"second countdown modal lifecycle observed");
    auto key=[&](int code,Qt::KeyboardModifiers modifiers=Qt::NoModifier){auto* focused=QApplication::focusWidget();QKeyEvent event(QEvent::KeyPress,code,modifiers);if(focused)QApplication::sendEvent(focused,&event);app.processEvents();};
    edit->setFocus();key(Qt::Key_Tab);check(QApplication::focusWidget()==appearance,"keyboard reaches appearance after Edit station");
    key(Qt::Key_Tab);check(QApplication::focusWidget()==stopwatch,"keyboard reaches stopwatch after appearance");
    key(Qt::Key_Tab);check(QApplication::focusWidget()==countdown,"keyboard reaches countdown after stopwatch");
    key(Qt::Key_Tab);check(QApplication::focusWidget()==paths,"keyboard reaches map path control after countdown");
    key(Qt::Key_Backtab,Qt::ShiftModifier);check(QApplication::focusWidget()==countdown,"reverse keyboard path returns to countdown");
    key(Qt::Key_Backtab,Qt::ShiftModifier);check(QApplication::focusWidget()==stopwatch,"reverse keyboard path returns to stopwatch");
    key(Qt::Key_Backtab,Qt::ShiftModifier);check(QApplication::focusWidget()==appearance,"reverse keyboard path returns to appearance");
    const auto preserved=bytes(path);const auto app_palette=QApplication::palette();
    auto theme=[&](const char* mode){appearance->setCurrentIndex(appearance->findData(mode));app.processEvents();app.processEvents();};
    auto colors=[&](){auto* footer=dashboard.findChild<QLabel*>("dashboardStatus");auto* state=dashboard.findChild<QLabel*>("dashboardFluxPaneState");
        check(contrast(footer->palette().color(QPalette::WindowText),dashboard.palette().color(QPalette::Window))>=4.5,"rendered footer text has AA normal-text contrast");
        check(contrast(state->palette().color(QPalette::WindowText),instruments->palette().color(QPalette::Window))>=4.5,"instrument unavailable text has AA normal-text contrast");
        edit->setFocus();app.processEvents();QImage action(edit->size(),QImage::Format_ARGB32);edit->render(&action);check(contrast(action.pixelColor(edit->width()/2,1),action.pixelColor(5,edit->height()/2))>=3,"rendered keyboard focus border contrasts with action surface");};
    theme("dark");colors();check(dashboard.palette().color(QPalette::Window)==QColor("#22282c"),"dashboard uses approved slate dark canvas");
    QImage rendered(dashboard.size(),QImage::Format_ARGB32);dashboard.render(&rendered);
    check(rendered.pixelColor(0,0)==QColor("#22282c"),"rendered dark canvas matches semantic palette");
    theme("light");colors();dashboard.render(&rendered);check(dashboard.palette().color(QPalette::Window)==QColor("#eeede9"),"dashboard uses approved warm light canvas");
    theme("system");
    for(const QColor color:{QColor("#101010"),QColor("#fafafa")}){
        auto palette=app_palette;palette.setColor(QPalette::Window,color);QApplication::setPalette(palette);app.processEvents();app.processEvents();
        check(dashboard.palette().color(QPalette::Window)==QColor(color.lightness()<128?"#22282c":"#eeede9"),"System preview follows live application palette");
        check(utc_label->accessibleName().contains(utc_label->text()),"theme refresh preserves actual accessible clock");
    }
    QApplication::setPalette(app_palette);theme("light");
    check(same(dashboard.stationSettings(),initial) && bytes(path)==preserved,"appearance previews never claim or write saved settings");
    paths->click();check(paths->text()=="Show paths" && map->accessibleDescription().contains("hidden"),"map toolbar hides computed route");
    paths->click();check(!map->routeSegments().empty(),"map toolbar restores computed route");
    dashboard.resize(800,480);app.processEvents();
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
