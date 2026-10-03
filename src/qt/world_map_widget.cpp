#include "world_map_widget.hpp"
#include "../map/coastline.hpp"
#include "../map/path.hpp"
#include "../map/projection.hpp"
#include <QAccessible>
#include <QEvent>
#include <QPainter>
#include <QPalette>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace hamclock {
WorldMapWidget::WorldMapWidget(QWidget* parent) : QWidget(parent) {
    setObjectName("worldMap");
    setAccessibleName("World map");
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    for (const auto& part : coastline_parts)
        for (std::size_t i=1; i<part.count; ++i) {
            const auto a=coastline_points[part.start+i-1], b=coastline_points[part.start+i];
            const auto split=map_split_equirectangular({a.latitude,a.longitude},{b.latitude,b.longitude});
            for (std::size_t j=0;j<split.count;++j) {
                const auto& line=split.segments[j];
                coast_.moveTo(line.first.x,line.first.y);
                coast_.lineTo(line.second.x,line.second.y);
            }
        }
    describe();
}
void WorldMapWidget::setEndpoints(Location de,Location dx) {
    const auto result=hamclock::geometry(de,dx); // Validate before modifying widget state.
    std::vector<Location> route;
    if (result.status!=BearingStatus::Antipodal) route=map_short_path(de,dx);
    QString summary=QString::number(std::llround(result.distance_km))+" kilometres; ";
    summary += result.status==BearingStatus::Defined
        ? QString::number(std::llround(result.short_bearing_deg)%360)+" degrees true, short path."
        : "bearing undefined; "+QString(result.status==BearingStatus::Antipodal?"antipodal endpoints.":"coincident endpoints.");
    de_=de;dx_=dx;route_=std::move(route);summary_=summary;
    describe();update();
}
void WorldMapWidget::clearEndpoints() {
    de_.reset();dx_.reset();route_.clear();summary_.clear();describe();update();
}
void WorldMapWidget::setAppearance(const std::string& mode) {
    if (mode!="system" && mode!="light" && mode!="dark") throw std::invalid_argument("Invalid map appearance");
    appearance_=mode;update();
}
void WorldMapWidget::setRouteVisible(bool visible) {route_visible_=visible;describe();update();}
QRectF WorldMapWidget::mapBounds() const {
    const double available_width=std::max(0,width()-16), available_height=std::max(0,height()-16);
    const double w=std::min(available_width,available_height*2.0), h=w/2.0;
    return {(width()-w)/2.0,(height()-h)/2.0,w,h};
}
QPointF WorldMapWidget::endpointPixel(Location location) const {
    const auto p=map_project_equirectangular(location);const auto bounds=mapBounds();
    return {bounds.left()+p.x*bounds.width(),bounds.top()+p.y*bounds.height()};
}
std::vector<QLineF> WorldMapWidget::routeSegments() const {
    std::vector<QLineF> lines;
    const auto bounds=mapBounds();
    for (std::size_t i=1;i<route_.size();++i) {
        const auto split=map_split_equirectangular(route_[i-1],route_[i]);
        for (std::size_t j=0;j<split.count;++j) {
            const auto& line=split.segments[j];
            lines.emplace_back(bounds.left()+line.first.x*bounds.width(),bounds.top()+line.first.y*bounds.height(),
                               bounds.left()+line.second.x*bounds.width(),bounds.top()+line.second.y*bounds.height());
        }
    }
    return lines;
}
QString WorldMapWidget::routeSummary() const {return de_?summary_:QString("Station endpoints not set.");}
QSize WorldMapWidget::sizeHint() const {return {600,330};}
QSize WorldMapWidget::minimumSizeHint() const {return {160,100};}
void WorldMapWidget::describe() {
    QString description="Equirectangular world coastline from Natural Earth 110m. North is up. ";
    description += routeSummary();
    if(de_) description += QString(" DE circle: latitude %1, longitude %2. DX square: latitude %3, longitude %4.")
        .arg(de_->latitude,0,'g',10).arg(de_->longitude,0,'g',10).arg(dx_->latitude,0,'g',10).arg(dx_->longitude,0,'g',10);
    if(!route_visible_) description += " Route hidden.";
    setAccessibleDescription(description);
    QAccessibleEvent event(this,QAccessible::DescriptionChanged);QAccessible::updateAccessibility(&event);
}
void WorldMapWidget::changeEvent(QEvent* event) {
    if(event->type()==QEvent::PaletteChange || event->type()==QEvent::ApplicationPaletteChange)update();
    QWidget::changeEvent(event);
}
void WorldMapWidget::paintEvent(QPaintEvent*) {
    const bool dark=appearance_=="dark" || (appearance_=="system" && palette().color(QPalette::Window).lightness()<128);
    const QColor sea(dark?"#24383c":"#dce8eb"),coast(dark?"#9fb5b7":"#647c7e"),route(dark?"#b4d9e8":"#315e77");
    QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(),palette().color(QPalette::Window));
    const auto bounds=mapBounds();if(bounds.isEmpty())return;
    painter.setClipRect(bounds);painter.fillRect(bounds,sea);
    painter.save();painter.translate(bounds.topLeft());painter.scale(bounds.width(),bounds.height());
    QPen pen(coast);pen.setWidthF(1);pen.setCosmetic(true);painter.setPen(pen);painter.setBrush(Qt::NoBrush);
    painter.drawPath(coast_);painter.restore();
    if(route_visible_){painter.setPen(QPen(route,2));for(const auto& line:routeSegments())painter.drawLine(line);}
    if(de_){
        painter.setPen(QPen(route,2));painter.setBrush(sea);
        const auto de=endpointPixel(*de_),dx=endpointPixel(*dx_);
        painter.drawEllipse(de,4,4);painter.drawRect(QRectF(dx.x()-4,dx.y()-4,8,8));
        painter.setPen(route);
        for(const auto& item : {std::pair<QPointF,QString>{de,"DE"},{dx,"DX"}}){
            const auto metrics=painter.fontMetrics();const double label_width=metrics.horizontalAdvance(item.second);
            // A tiny but nonempty letterboxed map can be shorter than the
            // font ascent. Avoid reversed clamp bounds and clipped labels.
            if(bounds.height()<metrics.height() || bounds.width()<label_width)continue;
            const double x=std::clamp(item.first.x()+7,bounds.left(),std::max(bounds.left(),bounds.right()-label_width));
            const double y=std::clamp(item.first.y()-7,bounds.top()+metrics.ascent(),bounds.bottom());
            painter.drawText(QPointF(x,y),item.second);
        }
    }
}
}
