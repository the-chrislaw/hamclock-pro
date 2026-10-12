#include "station_dashboard.hpp"
#include "station_editor_dialog.hpp"
#include "stopwatch_dialog.hpp"
#include "countdown_dialog.hpp"
#include "big_clock_dialog.hpp"
#include "big_clock_civil_adapter.hpp"
#include "dashboard_appearance.hpp"
#include "world_map_widget.hpp"
#include "../app/station_view.hpp"
#include <QComboBox>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <cmath>
namespace hamclock {
namespace {
QLabel* label(const QString& text,const char* role,QWidget* parent,bool secondary=false){
 auto* item=new QLabel(text,parent);item->setTextFormat(Qt::PlainText);item->setWordWrap(true);
 item->setProperty("dashboardRole",role);item->setProperty("dashboardSecondary",secondary);return item;
}
QLabel* value_label(const char* name,const char* accessible,QWidget* parent,const char* role="body"){
 auto* item=label({},role,parent);item->setObjectName(name);item->setProperty("stationValuePrefix",accessible);item->setAccessibleName(accessible);return item;
}
void update_value(QLabel* item,const QString& text){item->setText(text);item->setAccessibleName(item->property("stationValuePrefix").toString()+": "+text);}
QFrame* divider(QWidget* parent,bool vertical=false){auto* line=new QFrame(parent);line->setProperty("dashboardDivider",true);if(vertical)line->setFixedWidth(1);else line->setFixedHeight(1);return line;}
QFrame* section(QWidget* parent,const char* name){auto* frame=new QFrame(parent);frame->setObjectName(name);frame->setProperty("dashboardSection",true);return frame;}
QVBoxLayout* section_layout(QFrame* frame){auto* layout=new QVBoxLayout(frame);layout->setContentsMargins(12,10,12,10);layout->setSpacing(3);return layout;}
QString coordinate_text(Location location){return QString("%1° %2, %3° %4").arg(std::abs(location.latitude),0,'g',7).arg(location.latitude<0?"S":"N").arg(std::abs(location.longitude),0,'g',7).arg(location.longitude<0?"W":"E");}
}
StationDashboard::StationDashboard(const StationSettings& initial,const std::string& path,QWidget* parent):QDialog(parent),settings_(initial),path_(path){
 (void)make_station_view(initial);if(path.empty()||path.find('\0')!=std::string::npos)throw std::invalid_argument("Explicit dashboard settings path required");
 stopwatch_clock_.start();
 setObjectName("stationDashboard");setWindowTitle("HamClock Pro · Station dashboard");setAccessibleName("Station dashboard");resize(800,480);
 outer_=new QVBoxLayout(this);outer_->setContentsMargins(12,8,12,8);outer_->setSpacing(8);
 auto* masthead=new QWidget(this);masthead->setObjectName("dashboardMasthead");masthead->setFixedHeight(44);
 auto* header=new QHBoxLayout(masthead);header->setContentsMargins(0,0,0,0);header->setSpacing(8);
 auto* brand=label("HamClock Pro","brand",masthead);header->addWidget(brand);header->addStretch();
 auto* edit=new QPushButton("Edit station",masthead);edit->setObjectName("dashboardEditStation");edit->setMinimumSize(104,44);edit->setFocusPolicy(Qt::StrongFocus);
 edit->setAccessibleDescription("Open station settings. Dashboard updates only after a successful save.");header->addWidget(edit);
 auto* appearance_caption=label("Appearance","caption",masthead,true);appearance_caption->setObjectName("dashboardAppearanceCaption");header->addWidget(appearance_caption);appearance_=new QComboBox(masthead);appearance_->setObjectName("dashboardAppearance");
 appearance_->addItem("System","system");appearance_->addItem("Light","light");appearance_->addItem("Dark","dark");appearance_->setMinimumHeight(44);appearance_->setAccessibleName("Appearance preview");
 appearance_->setAccessibleDescription("Preview appearance. Save your preference in station settings.");header->addWidget(appearance_);
 auto* stopwatch=new QPushButton("Stopwatch",masthead);stopwatch->setObjectName("dashboardStopwatch");stopwatch->setMinimumSize(96,44);stopwatch->setFocusPolicy(Qt::StrongFocus);stopwatch->setAccessibleName("Open stopwatch");
 stopwatch->setAccessibleDescription("Open the stopwatch. Closing its window preserves running elapsed time. Timers are not saved when the application exits.");header->addWidget(stopwatch);
 auto* countdown=new QPushButton("Countdown",masthead);countdown->setObjectName("dashboardCountdown");countdown->setMinimumSize(104,44);countdown->setFocusPolicy(Qt::StrongFocus);countdown->setAccessibleName("Open countdown");
 countdown->setAccessibleDescription("Open countdown controls. Initial duration is one minute. Closing preserves counting; timers are not saved when the application exits.");header->addWidget(countdown);
 auto* big_clock=new QPushButton("Big clock",masthead);big_clock->setObjectName("dashboardBigClock");big_clock->setMinimumSize(88,44);big_clock->setFocusPolicy(Qt::StrongFocus);big_clock->setAccessibleName("Open Big Clock");
 big_clock->setAccessibleDescription("Open the operating-system UTC clock. Synchronization evidence and DE conversion are unavailable. Display options are session only.");header->addWidget(big_clock);outer_->addWidget(masthead);
 body_=new QHBoxLayout;body_->setSpacing(12);
 sidebar_=new QFrame(this);sidebar_->setObjectName("stationSidebar");sidebar_->setProperty("dashboardCard",true);sidebar_->setFixedWidth(166);
 auto* side=new QVBoxLayout(sidebar_);side->setContentsMargins(0,0,0,0);side->setSpacing(0);
 auto* station=section(sidebar_,"dashboardStationSection");auto* station_layout=section_layout(station);
 station_layout->addWidget(label("Your station","caption",station,true));callsign_=value_label("dashboardCallsign","Station callsign",station,"callsign");station_layout->addWidget(callsign_);
 clock_=value_label("utcClock","Current UTC time",station,"clock");clock_->setWordWrap(false);station_layout->addWidget(clock_);
 date_=value_label("utcDate","Current UTC date",station,"caption");date_->setProperty("dashboardSecondary",true);station_layout->addWidget(date_);side->addWidget(station);side->addWidget(divider(sidebar_));
 auto* de=section(sidebar_,"dashboardDeSection");auto* de_layout=section_layout(de);de_layout->addWidget(label("DE · Your station","caption",de,true));
 de_grid_=value_label("dashboardDeGrid","DE Maidenhead grid",de,"location");de_layout->addWidget(de_grid_);
 de_coordinates_=value_label("dashboardDeCoordinates","DE latitude and longitude",de);de_coordinates_->setProperty("dashboardSecondary",true);de_layout->addWidget(de_coordinates_);side->addWidget(de);side->addWidget(divider(sidebar_));
 auto* dx=section(sidebar_,"dashboardDxSection");auto* dx_layout=section_layout(dx);dx_layout->addWidget(label("DX · Destination","caption",dx,true));
 dx_grid_=value_label("dashboardDxGrid","DX Maidenhead grid",dx,"location");dx_layout->addWidget(dx_grid_);
 dx_coordinates_=value_label("dashboardDxCoordinates","DX latitude and longitude",dx);dx_coordinates_->setProperty("dashboardSecondary",true);dx_layout->addWidget(dx_coordinates_);side->addWidget(dx);side->addWidget(divider(sidebar_));
 auto* spots=section(sidebar_,"dashboardSpotsSection");auto* spots_layout=section_layout(spots);spots_layout->addWidget(label("DX cluster","caption",spots,true));
 auto* spot_state=label("Not connected","body",spots,true);spot_state->setObjectName("dashboardSpotState");spots_layout->addWidget(spot_state);
 auto* spot_scroll=new QScrollArea(spots);spot_scroll->setObjectName("dashboardSpotList");spot_scroll->setWidgetResizable(true);spot_scroll->setMinimumHeight(0);spot_scroll->setFrameShape(QFrame::NoFrame);
 auto* spot_list=new QWidget(spot_scroll);spot_list->setObjectName("spotList");spot_scroll->setWidget(spot_list);spots_layout->addWidget(spot_scroll,1);side->addWidget(spots,1);body_->addWidget(sidebar_);
 auto* workspace=new QWidget(this);workspace->setObjectName("dashboardWorkspace");auto* work=new QVBoxLayout(workspace);work->setContentsMargins(0,0,0,0);work->setSpacing(8);
 instruments_=new QFrame(workspace);instruments_->setObjectName("dashboardInstruments");instruments_->setProperty("dashboardCard",true);instruments_->setFixedHeight(122);
 auto* strip=new QHBoxLayout(instruments_);strip->setContentsMargins(0,0,0,0);strip->setSpacing(0);
 const char* titles[]={"Solar flux","Propagation","The Sun","Space wx"};const char* names[]={"dashboardFluxPane","dashboardPropagationPane","dashboardSunPane","dashboardSpaceWeatherPane"};
 const char* states[]={"No solar flux source","Model not connected","Solar imagery not connected","No data"};
 for(int i=0;i<4;++i){if(i)strip->addWidget(divider(instruments_,true));auto* pane=section(instruments_,names[i]);auto* pane_layout=section_layout(pane);
  pane_layout->addWidget(label(titles[i],"paneTitle",pane));pane_layout->addStretch();auto* missing=label("Unavailable","unavailable",pane,true);missing->setObjectName(QString(names[i])+"State");pane_layout->addWidget(missing);
  pane_layout->addWidget(label(states[i],"caption",pane,true));pane_layout->addStretch();if(i==3){pane->setFixedWidth(104);strip->addWidget(pane);}else strip->addWidget(pane,i==0?105:100);
 }
 work->addWidget(instruments_);
 auto* map_card=new QFrame(workspace);map_card->setObjectName("dashboardMapCard");map_card->setProperty("dashboardCard",true);
 auto* map_area=new QVBoxLayout(map_card);map_area->setContentsMargins(10,0,10,10);map_area->setSpacing(6);
 auto* toolbar=new QWidget(map_card);toolbar->setObjectName("dashboardMapToolbar");toolbar->setFixedHeight(44);auto* tools=new QHBoxLayout(toolbar);tools->setContentsMargins(0,0,0,0);tools->setSpacing(8);
 tools->addWidget(label("World map","paneTitle",toolbar));tools->addStretch();route_toggle_=new QPushButton("Hide paths",toolbar);route_toggle_->setObjectName("dashboardRouteToggle");route_toggle_->setMinimumSize(96,44);route_toggle_->setFocusPolicy(Qt::StrongFocus);route_toggle_->setCheckable(true);route_toggle_->setChecked(true);tools->addWidget(route_toggle_);map_area->addWidget(toolbar);
 map_=new WorldMapWidget(map_card);map_->setObjectName("dashboardWorldMap");map_area->addWidget(map_,1);
 auto* path_row=new QWidget(map_card);path_row->setObjectName("dashboardPathRow");auto* path_layout=new QHBoxLayout(path_row);path_layout->setContentsMargins(0,0,0,0);path_layout->setSpacing(12);
 distance_=value_label("dashboardDistance","Short-path distance",path_row);bearing_=value_label("dashboardBearing","Initial short-path true bearing",path_row);
 path_layout->addWidget(distance_);path_layout->addWidget(bearing_);path_layout->addStretch();map_area->addWidget(path_row);
 route_summary_=value_label("dashboardRouteSummary","Computed route summary",map_card,"caption");route_summary_->setProperty("dashboardSecondary",true);map_area->addWidget(route_summary_);work->addWidget(map_card,1);body_->addWidget(workspace,1);outer_->addLayout(body_,1);
 auto* status=label("Offline station tools · Natural Earth 110m · Equirectangular coastline","caption",this,true);status->setObjectName("dashboardStatus");outer_->addWidget(status);
 QWidget::setTabOrder(edit,appearance_);QWidget::setTabOrder(appearance_,stopwatch);QWidget::setTabOrder(stopwatch,countdown);QWidget::setTabOrder(countdown,big_clock);QWidget::setTabOrder(big_clock,route_toggle_);
 QObject::connect(edit,&QPushButton::clicked,this,[this](){editStation();});
 QObject::connect(stopwatch,&QPushButton::clicked,this,[this](){showStopwatch();});
 QObject::connect(countdown,&QPushButton::clicked,this,[this](){showCountdown();});
 QObject::connect(big_clock,&QPushButton::clicked,this,[this](){showBigClock();});
 QObject::connect(appearance_,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int){const auto mode=appearance_->currentData().toString().toStdString();apply_dashboard_appearance(*this,mode);map_->setAppearance(mode);});
 QObject::connect(route_toggle_,&QPushButton::toggled,this,[this](bool visible){route_visible_=visible;map_->setRouteVisible(visible);route_toggle_->setText(visible?"Hide paths":"Show paths");route_summary_->setAccessibleDescription(map_->accessibleDescription());});
 refreshStation();adaptLayout();clock_timer_=new QTimer(this);clock_timer_->setObjectName("dashboardClockTimer");clock_timer_->setInterval(1000);
 auto tick=[this](){const auto now=QDateTime::currentDateTimeUtc();
 const auto source=now.isValid()?std::optional<UtcSeconds>{UtcSeconds{now.toSecsSinceEpoch()}}:std::nullopt;
 const auto converted=adapt_big_clock_utc_civil(source);
 big_clock_input_=BigClockDigitalInput{{converted.civil,std::nullopt},std::nullopt,SyncStatus::Unknown};
 update_value(clock_,now.isValid()?now.toString("HH:mm:ss"):QString("Unavailable"));update_value(date_,now.toString("yyyy-MM-dd 'UTC'"));clock_->setAccessibleDescription("Operating-system UTC clock: "+now.toString(Qt::ISODate));};
 QObject::connect(clock_timer_,&QTimer::timeout,this,tick);tick();clock_timer_->start();
}
void StationDashboard::refreshStation(){
 const auto view=make_station_view(settings_);update_value(callsign_,QString::fromStdString(view.callsign));
 update_value(de_grid_,QString::fromStdString(view.de_grid));update_value(dx_grid_,QString::fromStdString(view.dx_grid));
 update_value(de_coordinates_,coordinate_text(view.de));update_value(dx_coordinates_,coordinate_text(view.dx));
 update_value(distance_,QString::fromStdString(view.distance_text));update_value(bearing_,QString::fromStdString(view.bearing_text));
 map_->setEndpoints(view.de,view.dx);map_->setAppearance(settings_.appearance);map_->setRouteVisible(route_visible_);
 update_value(route_summary_,map_->routeSummary());route_summary_->setAccessibleDescription(map_->accessibleDescription());
 QSignalBlocker blocked(appearance_);appearance_->setCurrentIndex(appearance_->findData(QString::fromStdString(settings_.appearance)));apply_dashboard_appearance(*this,settings_.appearance);
}
void StationDashboard::editStation(){auto* origin=findChild<QPushButton*>("dashboardEditStation");auto saved=show_station_editor(this,path_,settings_,false);if(saved){settings_=std::move(*saved);refreshStation();}if(origin)origin->setFocus();}
void StationDashboard::showStopwatch(){
 auto* origin=findChild<QPushButton*>("dashboardStopwatch");
 try {
  StopwatchDialog dialog(stopwatch_state_,stopwatch_presentation_,
      [this](){return MonotonicNanoseconds{stopwatch_clock_.nsecsElapsed()};},
      appearance_->currentData().toString().toStdString(),this);
  dialog.exec();
 } catch(const std::exception&) {
  auto* status=findChild<QLabel*>("dashboardStatus");
  if(status){status->setText("Stopwatch could not open. Station settings are unchanged.");status->setAccessibleName(status->text());}
 }
 activateWindow();
 if(origin)origin->setFocus();
}
void StationDashboard::showCountdown(){
 auto* origin=findChild<QPushButton*>("dashboardCountdown");
 try {
  CountdownDialog dialog(countdown_service_,
      [this](){return MonotonicNanoseconds{stopwatch_clock_.nsecsElapsed()};},
      appearance_->currentData().toString().toStdString(),this);
  dialog.exec();
 } catch(const std::exception&) {
  auto* status=findChild<QLabel*>("dashboardStatus");
  if(status){status->setText("Countdown could not open. Station settings are unchanged.");status->setAccessibleName(status->text());}
 }
 activateWindow();
 if(origin)origin->setFocus();
}
void StationDashboard::showBigClock(){
 auto* origin=findChild<QPushButton*>("dashboardBigClock");
 try {
  BigClockDialog dialog(big_clock_preferences_,[this](){return big_clock_input_;},appearance_->currentData().toString().toStdString(),this);
  dialog.setAccessibleDescription("Operating-system UTC source. Synchronization evidence unavailable; DE conversion is not implemented. Display preferences are session only.");
  dialog.exec();
  big_clock_preferences_=dialog.preferences();
 } catch(const std::exception&) {
  auto* status=findChild<QLabel*>("dashboardStatus");
  if(status){status->setText("Big Clock could not open. Station settings are unchanged.");status->setAccessibleName(status->text());}
 }
 activateWindow();
 if(origin)origin->setFocus();
}
void StationDashboard::resizeEvent(QResizeEvent* event){QDialog::resizeEvent(event);if(body_&&clock_&&sidebar_&&instruments_)adaptLayout();}
void StationDashboard::adaptLayout(){
 const bool large=width()>=1050;if(auto* caption=findChild<QLabel*>("dashboardAppearanceCaption"))caption->setVisible(large);outer_->setContentsMargins(large?32:12,large?26:8,large?32:12,large?20:8);outer_->setSpacing(large?16:8);body_->setSpacing(large?16:12);sidebar_->setFixedWidth(large?190:166);instruments_->setFixedHeight(large?175:122);
 for(auto* frame:findChildren<QFrame*>())if(frame->property("dashboardSection").toBool() && frame->layout()){frame->layout()->setContentsMargins(large?12:10,large?10:7,large?12:10,large?10:7);frame->layout()->setSpacing(large?3:2);}
 for(auto* item:findChildren<QLabel*>()){const auto role=item->property("dashboardRole").toString();if(role.isEmpty())continue;QFont font=item->font();int size=large?13:12;
  if(role=="brand")size=large?17:15;else if(role=="clock")size=large?36:30;else if(role=="callsign")size=large?27:23;else if(role=="location")size=large?17:14;else if(role=="caption")size=large?12:11;else if(role=="unavailable")size=large?14:12;
  font.setPixelSize(size);font.setBold(role=="brand"||role=="callsign");item->setFont(font);
 }
 // Platform fonts differ (e.g. DejaVu on Raspberry Pi OS is wider than macOS
 // system text), so size the narrow utility pane from its actual state text.
 constexpr int kUtilityPaneMinimumWidth=104;
 auto* weather=findChild<QWidget*>("dashboardSpaceWeatherPane");auto* weather_state=findChild<QLabel*>("dashboardSpaceWeatherPaneState");
 if(weather && weather_state && weather->layout()){const auto margins=weather->layout()->contentsMargins();
  weather->setFixedWidth(std::max(kUtilityPaneMinimumWidth,weather_state->fontMetrics().horizontalAdvance(weather_state->text())+margins.left()+margins.right()));}
}
}
