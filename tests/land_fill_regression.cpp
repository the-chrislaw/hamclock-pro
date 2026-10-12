#include "../src/qt/land_fill_path.hpp"
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

using namespace hamclock;
namespace {
int checks = 0, failures = 0;
void check(bool condition, const char* name) {
    ++checks;
    if (!condition) { ++failures; std::cerr << "Land fill failure: " << name << '\n'; }
}
using Points = std::vector<LandFillPoint>;
Points rectangle(double west, double south, double east, double north) {
    return {{west,south},{west,north},{east,north},{east,south},{west,south}};
}
LandFillResult make(const Points& p, const std::vector<LandFillRing>& r,
                    const std::vector<LandFillRecord>& records) {
    return make_land_fill_paths({p.data(),p.size(),r.data(),r.size(),records.data(),records.size()});
}
bool contains(const LandFillResult& result, double lon, double lat) {
    const auto point = project_land_fill_point({lon,lat});
    return point && std::any_of(result.paths.begin(),result.paths.end(),
        [&](const QPainterPath& path){ return path.contains(*point); });
}
void reject(const Points& p, const std::vector<LandFillRing>& r,
            const std::vector<LandFillRecord>& records, LandFillError error) {
    const auto result=make(p,r,records);
    check(result.error==error && result.paths.empty(),"explicit error, no partial output");
}
}

int main(int argc,char** argv) {
    QGuiApplication app(argc,argv);
    for (const auto point : {LandFillPoint{-180,90},LandFillPoint{180,90},
                            LandFillPoint{-180,-90},LandFillPoint{180,-90},LandFillPoint{0,0}}) {
        const auto projected=project_land_fill_point(point);
        check(projected.has_value(),"valid endpoint projection");
        check(projected && projected->x()==(point.longitude+180)/360 &&
              projected->y()==(90-point.latitude)/180,"independent normalized reference");
    }
    const auto near_east=project_land_fill_point({std::nextafter(180.,0.),0});
    const auto near_west=project_land_fill_point({std::nextafter(-180.,0.),0});
    check(near_east && near_east->x()>0.99 && near_west && near_west->x()<0.01,"interior seam side retained");
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity(),181.,-181.})
        check(!project_land_fill_point({bad,0}),"invalid longitude rejected");
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),91.,-91.})
        check(!project_land_fill_point({0,bad}),"invalid latitude rejected");

    Points p=rectangle(-20,-20,20,20),hole=rectangle(-10,-10,10,10);
    std::reverse(hole.begin(),hole.end());p.insert(p.end(),hole.begin(),hole.end());
    auto donut=make(p,{{0,5},{5,5}},{{0,2}});
    check(donut.error==LandFillError::None && donut.paths.size()==1,"one path per record");
    check(donut.paths[0].fillRule()==Qt::WindingFill,"explicit winding fill");
    check(contains(donut,15,0) && !contains(donut,0,0) && !contains(donut,30,0),"donut land hole ocean");
    Points hole_first=hole,outer=rectangle(-20,-20,20,20);hole_first.insert(hole_first.end(),outer.begin(),outer.end());
    const auto reversed_order=make(hole_first,{{0,5},{5,5}},{{0,2}});
    check(contains(reversed_order,15,0) && !contains(reversed_order,0,0),"hole order does not classify");
    std::reverse(p.begin(),p.begin()+5);std::reverse(p.begin()+5,p.end());
    auto reversed_winding=make(p,{{0,5},{5,5}},{{0,2}});
    check(contains(reversed_winding,15,0) && !contains(reversed_winding,0,0),"all directions reversed preserves region");
    auto island=rectangle(-2,-2,2,2);p=outer;p.insert(p.end(),hole.begin(),hole.end());p.insert(p.end(),island.begin(),island.end());
    auto nested=make(p,{{0,5},{5,5},{10,5}},{{0,3}});
    check(contains(nested,0,0) && !contains(nested,5,0) && contains(nested,15,0),"nested island in hole");
    p=rectangle(-80,-10,-60,10);auto second=rectangle(60,-10,80,10);p.insert(p.end(),second.begin(),second.end());
    auto disjoint=make(p,{{0,5},{5,5}},{{0,2}});
    check(contains(disjoint,-70,0) && contains(disjoint,70,0) && !contains(disjoint,0,0),"multiple exteriors");
    p=outer;p.insert(p.end(),outer.begin(),outer.end());
    auto overlap=make(p,{{0,5},{5,5}},{{0,1},{1,1}});
    check(overlap.paths.size()==2 && contains(overlap,0,0),"independent overlapping records do not cancel");

    // Only the polar closure spans 360 degrees. The nonpolar top is
    // explicitly stepped at Greenwich, matching the source acceptance rule.
    Points antarctic={{-180,-90},{-180,-80},{0,-80},{180,-80},{180,-90},{-180,-90}};
    auto polar=make(antarctic,{{0,antarctic.size()}},{{0,1}});
    check(polar.error==LandFillError::None && contains(polar,0,-85) && !contains(polar,0,-70),"full width Antarctica survives");
    check(contains(polar,-175,-85) && contains(polar,175,-85),"Antarctic polar closure retains both seam sides");
    reject(rectangle(-180,-90,180,-80),{{0,5}},{{0,1}},LandFillError::UnsupportedSeam);
    p=rectangle(170,40,180,50);second=rectangle(-180,40,-170,50);p.insert(p.end(),second.begin(),second.end());
    auto cut=make(p,{{0,5},{5,5}},{{0,1},{1,1}});
    check(contains(cut,175,45) && contains(cut,-175,45) && !contains(cut,0,45),"explicit seam polygons do not fill Greenwich");
    reject(rectangle(-170,40,170,50),{{0,5}},{{0,1}},LandFillError::UnsupportedSeam);
    reject(rectangle(-180,-80,180,-70),{{0,5}},{{0,1}},LandFillError::UnsupportedSeam);

    // Synthetic attached loop, preserving a shared nonadjacent vertex; source
    // topology is interpreted, not silently repaired.
    Points contact={{0,0},{-2,0},{-1,1},{0,0},{0,10},{10,10},{10,0},{0,0}};
    auto loop=make(contact,{{0,8}},{{0,1}});
    check(loop.error==LandFillError::None && contains(loop,5,5) && contains(loop,-1,0.25),"attached loop retained by winding model");
    check(loop.paths[0].elementCount()>=8,"contact vertices retained");

    // Untouched Natural Earth 4.1.0 land record 79, ring 78 (zero-based),
    // original points 1981..1994; source SHP SHA-256:
    // 8689e6932b8e370e2ca4587cf3ba21e460b1235db37b6ed3c172c35b4a6088de.
    // Archive SHA-256:
    // 1926c621afd6ac67c3f36639bb1236134a48d82226dc675d3e3df53d02d2a3de.
    // Natural Earth public-domain data: naturalearthdata.com/about/terms-of-use/.
    // No endpoint normalization changes this ring. Repeated point 3 and the
    // attached microtriangle remain; this fixture is not topology certification.
    const Points source79={
        {-132.7100078844312,54.04000931542345},
        {-132.71000850490572,54.040009263721345},
        {-132.7100076974615,54.040009263721345},
        {-132.7100078844312,54.04000931542345},
        {-131.7499895840034,54.12000438090914},
        {-132.049480347351,52.984621487024526},
        {-131.1790425218266,52.180432847698285},
        {-131.57782954982312,52.18237071390925},
        {-132.18042842677858,52.639707139692405},
        {-132.54999243231387,53.10001496033223},
        {-133.05461117875552,53.41146881775529},
        {-133.2396644827927,53.851080227262315},
        {-133.1800040417118,54.169975490935315},
        {-132.7100078844312,54.04000931542345}};
    const auto actual_contact=make(source79,{{0,source79.size()}},{{0,1}});
    check(actual_contact.error==LandFillError::None && actual_contact.paths.size()==1,
          "source record79 accepts attached microloop unchanged");
    // Independent exact binary-rational winding calculation of these samples:
    // (-132,53.5) has winding -1; (-134,53.5) has winding 0.
    check(contains(actual_contact,-132,53.5) && !contains(actual_contact,-134,53.5),
          "source record79 land and nearby ocean containment");
    if (!actual_contact.paths.empty()) {
        const auto& path=actual_contact.paths[0];
        check(path.elementCount()==static_cast<int>(source79.size()),"source contact vertices retained exactly");
        for (std::size_t i=0;i<source79.size() && i<static_cast<std::size_t>(path.elementCount());++i) {
            const auto expected=*project_land_fill_point(source79[i]);
            const auto actual=path.elementAt(static_cast<int>(i));
            check(actual.x==expected.x() && actual.y==expected.y(),"source contact point order and projection preserved");
        }
    }

    // Enlarge only the painter viewport, never repair the source geometry.
    // These interior samples exercise the complete original contact outline;
    // the microtriangle itself remains subpixel and is not certified here.
    if (!actual_contact.paths.empty()) {
        for (const QSize size : {QSize(180,120),QSize(360,240),QSize(720,480)}) {
            QImage image(size,QImage::Format_ARGB32);image.fill(Qt::magenta);
            QPainter painter(&image);const QRectF bounds(4,4,size.width()-8,size.height()-8);
            painter.setClipRect(bounds);painter.fillRect(bounds,Qt::blue);
            const auto northwest=*project_land_fill_point({-135,55});
            const auto southeast=*project_land_fill_point({-130,51});
            const double sx=bounds.width()/(southeast.x()-northwest.x());
            const double sy=bounds.height()/(southeast.y()-northwest.y());
            painter.translate(bounds.topLeft());painter.scale(sx,sy);painter.translate(-northwest);
            painter.setPen(Qt::NoPen);painter.setBrush(Qt::green);
            painter.drawPath(actual_contact.paths[0]);painter.end();
            const auto sample=[&](double lon,double lat) {
                const auto q=*project_land_fill_point({lon,lat});
                return image.pixelColor(static_cast<int>(bounds.left()+(q.x()-northwest.x())*sx),
                                        static_cast<int>(bounds.top()+(q.y()-northwest.y())*sy));
            };
            check(image.pixelColor(0,0)==QColor(Qt::magenta),"source record79 local raster clipped");
            check(sample(-132,53.5)==QColor(Qt::green),"source record79 local raster land");
            check(sample(-134,53.5)==QColor(Qt::blue),"source record79 local raster ocean");
        }
    }

    reject({}, {}, {},LandFillError::Empty);
    reject(outer,{{1,4}},{{0,1}},LandFillError::InvalidOffsets);
    reject(outer,{{0,4}},{{0,1}},LandFillError::InvalidOffsets);
    reject(outer,{{0,5}},{{1,1}},LandFillError::InvalidOffsets);
    reject(outer,{{0,5}},{{0,0}},LandFillError::InvalidOffsets);
    reject(outer,{{0,std::numeric_limits<std::size_t>::max()}},{{0,1}},LandFillError::InvalidOffsets);
    auto open=outer;open.back()={1,1};reject(open,{{0,5}},{{0,1}},LandFillError::InvalidRing);
    reject({{0,0},{1,1},{2,2},{0,0}},{{0,4}},{{0,1}},LandFillError::InvalidRing);
    reject({{0,0},{1,1},{0,0},{0,0}},{{0,4}},{{0,1}},LandFillError::InvalidRing);
    for (const auto point : {LandFillPoint{181,0},LandFillPoint{0,91},LandFillPoint{NAN,0},LandFillPoint{0,INFINITY}}) {
        auto invalid=outer;invalid[2]=point;reject(invalid,{{0,5}},{{0,1}},LandFillError::InvalidCoordinate);
    }
    const LandFillRing ring{0,5};const LandFillRecord record{0,1};
    for (const auto view : {LandFillView{nullptr,5,&ring,1,&record,1},
                           LandFillView{outer.data(),5,nullptr,1,&record,1},
                           LandFillView{outer.data(),5,&ring,1,nullptr,1}})
        check(make_land_fill_paths(view).error==LandFillError::InvalidView,"null view rejected before access");
    for (const auto view : {LandFillView{outer.data(),land_fill_max_points+1,&ring,1,&record,1},
                           LandFillView{outer.data(),5,&ring,land_fill_max_rings+1,&record,1},
                           LandFillView{outer.data(),5,&ring,1,&record,land_fill_max_records+1}})
        check(make_land_fill_paths(view).error==LandFillError::CountLimit,"count cap before access");
    auto repeated=outer;repeated.insert(repeated.begin(),outer.front());
    check(make(repeated,{{0,6}},{{0,1}}).error==LandFillError::None,"consecutive repeated point not dropped");

    for (const QSize size : {QSize(360,180),QSize(720,360),QSize(73,41)}) {
        QImage image(size,QImage::Format_ARGB32);image.fill(Qt::magenta);
        QPainter painter(&image);const QRectF bounds(4,4,size.width()-8,size.height()-8);
        painter.setClipRect(bounds);painter.fillRect(bounds,Qt::blue);
        painter.translate(bounds.topLeft());painter.scale(bounds.width(),bounds.height());
        painter.setPen(Qt::NoPen);painter.setBrush(Qt::green);painter.drawPath(donut.paths[0]);painter.end();
        check(image.pixelColor(0,0)==QColor(Qt::magenta),"paint stays within viewport");
        const auto pixel=[&](double lon,double lat) {const auto q=*project_land_fill_point({lon,lat});return image.pixelColor(
            static_cast<int>(bounds.left()+q.x()*bounds.width()),static_cast<int>(bounds.top()+q.y()*bounds.height()));};
        check(pixel(0,0)==QColor(Qt::blue),"raster hole remains ocean");
        check(pixel(15,0)==QColor(Qt::green),"raster exterior remains land");
        check(pixel(100,0)==QColor(Qt::blue),"raster distant ocean remains ocean");
    }
    std::cout<<checks<<" synthetic and pinned-source land fill checks; "<<failures<<" failures\n";
    return failures?1:0;
}
