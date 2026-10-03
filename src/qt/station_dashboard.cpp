#include "station_dashboard.hpp"
#include "station_editor_dialog.hpp"
#include "editor_appearance.hpp"
#include "world_map_widget.hpp"
#include "../app/station_view.hpp"
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <stdexcept>
#include <utility>
namespace hamclock {
namespace {
QLabel* value_label(const char* name,const char* accessible,QWidget* parent) {
    auto* label=new QLabel(parent);label->setObjectName(name);
    label->setProperty("stationValuePrefix",QString::fromUtf8(accessible));
    label->setAccessibleName(accessible);label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);return label;
}
void update_value(QLabel* label,const QString& text) {
    label->setText(text);
    label->setAccessibleName(label->property("stationValuePrefix").toString()+": "+text);
}
void section(QVBoxLayout& layout,const char* title,QWidget* parent) {
    auto* label=new QLabel(title,parent);label->setTextFormat(Qt::PlainText);
    QFont font=label->font();font.setBold(true);label->setFont(font);layout.addWidget(label);
}
QString coordinate_text(Location location) {
    return QString("%1°, %2°").arg(location.latitude,0,'g',10).arg(location.longitude,0,'g',10);
}
}
StationDashboard::StationDashboard(const StationSettings& initial,const std::string& path,QWidget* parent)
    : QDialog(parent),settings_(initial),path_(path) {
    (void)make_station_view(initial);
    if(path.empty() || path.find('\0')!=std::string::npos)throw std::invalid_argument("Explicit dashboard settings path required");
    setObjectName("stationDashboard");setWindowTitle("HamClock Pro · Station dashboard");
    setAccessibleName("Station dashboard");resize(800,480);
    auto* outer=new QVBoxLayout(this);outer->setContentsMargins(16,12,16,12);outer->setSpacing(12);
    auto* header=new QHBoxLayout;
    auto* brand=new QLabel("HamClock Pro",this);brand->setTextFormat(Qt::PlainText);
    QFont brand_font=brand->font();brand_font.setBold(true);
    brand_font.setPointSizeF((brand_font.pointSizeF()>0?brand_font.pointSizeF():12)+2);brand->setFont(brand_font);
    header->addWidget(brand);header->addStretch();
    clock_=value_label("utcClock","Current UTC time",this);clock_->setWordWrap(false);
    clock_->setAlignment(Qt::AlignRight|Qt::AlignVCenter);clock_->setMinimumWidth(230);
    header->addWidget(clock_);outer->addLayout(header);
    auto* body=new QHBoxLayout;body->setSpacing(16);
    auto* sidebar=new QScrollArea(this);sidebar->setObjectName("stationSidebar");
    sidebar->setWidgetResizable(true);sidebar->setFrameShape(QFrame::NoFrame);
    sidebar->setMinimumWidth(210);sidebar->setMaximumWidth(270);
    auto* details=new QWidget(sidebar);auto* info=new QVBoxLayout(details);
    info->setContentsMargins(0,0,8,0);info->setSpacing(8);
    section(*info,"Station",details);
    callsign_=value_label("dashboardCallsign","Station callsign",details);
    QFont call_font=callsign_->font();call_font.setBold(true);
    call_font.setPointSizeF((call_font.pointSizeF()>0?call_font.pointSizeF():12)+5);callsign_->setFont(call_font);info->addWidget(callsign_);
    info->addSpacing(8);section(*info,"DE · Your station",details);
    de_grid_=value_label("dashboardDeGrid","DE Maidenhead grid",details);info->addWidget(de_grid_);
    de_coordinates_=value_label("dashboardDeCoordinates","DE latitude and longitude, degrees north and east",details);info->addWidget(de_coordinates_);
    info->addSpacing(8);section(*info,"DX · Destination",details);
    dx_grid_=value_label("dashboardDxGrid","DX Maidenhead grid",details);info->addWidget(dx_grid_);
    dx_coordinates_=value_label("dashboardDxCoordinates","DX latitude and longitude, degrees north and east",details);info->addWidget(dx_coordinates_);
    info->addSpacing(8);section(*info,"Short path",details);
    distance_=value_label("dashboardDistance","Short-path distance",details);info->addWidget(distance_);
    bearing_=value_label("dashboardBearing","Initial short-path true bearing",details);info->addWidget(bearing_);
    info->addStretch();
    auto* edit=new QPushButton("Edit station",details);edit->setObjectName("dashboardEditStation");edit->setMinimumHeight(44);
    edit->setAccessibleDescription("Open station settings. Dashboard updates only after a successful save.");
    info->addWidget(edit);sidebar->setWidget(details);body->addWidget(sidebar);
    auto* map_area=new QVBoxLayout;map_area->setSpacing(8);
    map_=new WorldMapWidget(this);map_->setObjectName("dashboardWorldMap");map_area->addWidget(map_,1);
    route_summary_=value_label("dashboardRouteSummary","Computed route summary",this);map_area->addWidget(route_summary_);
    body->addLayout(map_area,1);outer->addLayout(body,1);
    auto* status=new QLabel("Offline station tools · Coastline: Natural Earth 110m",this);
    status->setTextFormat(Qt::PlainText);status->setWordWrap(true);outer->addWidget(status);
    QObject::connect(edit,&QPushButton::clicked,this,[this](){editStation();});
    refreshStation();
    clock_timer_=new QTimer(this);clock_timer_->setObjectName("dashboardClockTimer");clock_timer_->setInterval(1000);
    auto tick=[this](){update_value(clock_,QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss 'UTC'"));};
    QObject::connect(clock_timer_,&QTimer::timeout,this,tick);tick();clock_timer_->start();
}
void StationDashboard::refreshStation() {
    const auto view=make_station_view(settings_);
    update_value(callsign_,QString::fromStdString(view.callsign));
    update_value(de_grid_,QString::fromStdString(view.de_grid));update_value(dx_grid_,QString::fromStdString(view.dx_grid));
    update_value(de_coordinates_,coordinate_text(view.de));update_value(dx_coordinates_,coordinate_text(view.dx));
    update_value(distance_,QString::fromStdString(view.distance_text));update_value(bearing_,QString::fromStdString(view.bearing_text));
    map_->setEndpoints(view.de,view.dx);map_->setAppearance(settings_.appearance);
    update_value(route_summary_,map_->routeSummary());
    route_summary_->setAccessibleDescription(map_->accessibleDescription());
    apply_editor_appearance(*this,QString::fromStdString(settings_.appearance));
}
void StationDashboard::editStation() {
    auto saved=show_station_editor(this,path_,settings_,false);
    if(!saved)return;
    settings_=std::move(*saved);refreshStation();
}
}
