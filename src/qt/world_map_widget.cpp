#include "world_map_widget.hpp"
#include "land_fill_path.hpp"
#include "../map/land.hpp"
#include "../map/coastline.hpp"
#include "../map/path.hpp"
#include "../map/projection.hpp"
#include <QAccessible>
#include <QEvent>
#include <QPainter>
#include <QPalette>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>
namespace hamclock {
WorldMapWidget::WorldMapWidget(QWidget* parent) : QWidget(parent) {
    setObjectName("worldMap");
    setAccessibleName("World map");
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    // Convert the immutable generated views without aliasing distinct types.
    // Paths own their data; build once, never reparse geography during paint.
    std::vector<LandFillPoint> points;
    std::vector<LandFillRing> rings;
    std::vector<LandFillRecord> records;
    points.reserve(std::size(land_points));
    rings.reserve(std::size(land_rings));
    records.reserve(std::size(land_records));
    for (const auto& point : land_points) points.push_back({point.longitude,point.latitude});
    for (const auto& ring : land_rings) rings.push_back({ring.start,ring.count});
    for (const auto& record : land_records) records.push_back({record.first_ring,record.count});
    auto fill=make_land_fill_paths({points.data(),points.size(),rings.data(),rings.size(),records.data(),records.size()});
    if (fill.error!=LandFillError::None) throw std::runtime_error("Bundled land geometry could not be prepared");
    land_=std::move(fill.paths);
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
void WorldMapWidget::setSolarTime(UtcSeconds utc) {
    // The Sun moves 0.25 degrees a minute; repaint at most once per minute.
    const auto minute=utc.value/60;
    if(sun_ && minute==solar_minute_)return;
    solar_minute_=minute;sun_=subsolar_point({minute*60});update();
}
void WorldMapWidget::clearSolarTime() {if(!sun_)return;sun_.reset();update();}
std::optional<QPointF> WorldMapWidget::subsolarPixel() const {
    if(!sun_)return std::nullopt;
    return endpointPixel({sun_->latitude_deg,sun_->longitude_deg});
}
QPolygonF WorldMapWidget::nightPolygon() const {
    QPolygonF polygon;
    if(!sun_)return polygon;
    const auto bounds=mapBounds();
    auto point=[&](double lon,double lat){return QPointF(bounds.left()+(lon+180.0)/360.0*bounds.width(),bounds.top()+(90.0-lat)/180.0*bounds.height());};
    // Night is on the side of the terminator away from the Sun's hemisphere.
    const bool night_south=sun_->latitude_deg>=0;
    for(int lon=-180;lon<=180;++lon)polygon<<point(lon,terminator_latitude_deg(*sun_,lon));
    polygon<<point(180,night_south?-90:90)<<point(-180,night_south?-90:90);
    return polygon;
}
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
    QString description="Equirectangular world land and coastline from Natural Earth 110m. North is up. ";
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
    const QColor sea(dark?"#374a51":"#dce8eb"),coast(dark?"#8b9fa3":"#647c7e"),route(dark?"#b4d9e8":"#315e77");
    const QColor land(dark?"#29363b":"#d6ddce");
    QPainter painter(this);painter.setRenderHint(QPainter::Antialiasing);
    // Keep the letterbox in the active ocean theme; widget palettes may be
    // overridden by native style inheritance. Geographic bounds stay 2:1.
    painter.fillRect(rect(),sea);
    const auto bounds=mapBounds();if(bounds.isEmpty())return;
    painter.setClipRect(bounds);painter.fillRect(bounds,sea);
    painter.save();painter.translate(bounds.topLeft());painter.scale(bounds.width(),bounds.height());
    painter.setPen(Qt::NoPen);painter.setBrush(land);
    for (const auto& path : land_) painter.drawPath(path);
    QPen pen(coast);pen.setWidthF(1);pen.setCosmetic(true);painter.setPen(pen);painter.setBrush(Qt::NoBrush);
    painter.drawPath(coast_);painter.restore();
    if(sun_){
        painter.setPen(Qt::NoPen);painter.setBrush(QColor(0,0,0,dark?90:55));painter.drawPolygon(nightPolygon());
        const auto sun=*subsolarPixel();const QColor sun_color(dark?"#f2c14e":"#d68f00");
        painter.setPen(QPen(sun_color,2));painter.setBrush(Qt::NoBrush);painter.drawEllipse(sun,5,5);
        painter.drawLine(QPointF(sun.x()-8,sun.y()),QPointF(sun.x()+8,sun.y()));painter.drawLine(QPointF(sun.x(),sun.y()-8),QPointF(sun.x(),sun.y()+8));
    }
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
