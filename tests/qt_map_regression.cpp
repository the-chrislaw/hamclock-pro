#include "../src/qt/world_map_widget.hpp"
#include <QApplication>
#include <QImage>
#include <QPalette>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
int main(int argc,char** argv){
 QApplication app(argc,argv);hamclock::WorldMapWidget map;int failures=0,checks=0;
 auto check=[&](bool yes,const char* name){++checks;if(!yes){++failures;std::cerr<<"Map widget regression failed: "<<name<<'\n';}};
 auto near=[](double a,double b){return std::abs(a-b)<1e-8;};
 for(const QSize size:{QSize(640,400),QSize(800,480),QSize(300,500),QSize(12,12)}){
  map.resize(size);const auto b=map.mapBounds();
  check(near(b.width(),b.height()*2),"world aspect ratio");
  check(b.left()>=0 && b.top()>=0 && b.right()<=size.width() && b.bottom()<=size.height(),"bounds fit widget");
 }
 map.setEndpoints({0,0},{0,90});
 for(const QSize tiny:{QSize(16,16),QSize(17,17),QSize(18,18),QSize(33,17),QSize(17,33),QSize(40,40)}) {
  map.resize(tiny);QImage tiny_image(tiny,QImage::Format_ARGB32);tiny_image.fill(Qt::magenta);map.render(&tiny_image);
  check(tiny_image.pixelColor(0,0)!=QColor(Qt::magenta),"tiny endpoint paint completes with safe label bounds");
 }
 map.resize(640,400);map.setAppearance("light");map.setEndpoints({0,0},{0,90});
 const auto bounds=map.mapBounds();const auto de=map.endpointPixel({0,0}),dx=map.endpointPixel({0,90});
 check(near(de.x(),320)&&near(de.y(),200)&&near(dx.x(),476)&&near(dx.y(),200),"independent equatorial fixture");
 check(map.routeSummary().contains("90 degrees true"),"east bearing summary");
 for(const auto& line:map.routeSegments())check(near(line.y1(),200)&&near(line.y2(),200),"equatorial route horizontal");
 map.setEndpoints({0,179},{0,-179});
 bool touches_left=false,touches_right=false;
 for(const auto& line:map.routeSegments()){
  check(std::abs(line.x2()-line.x1())<bounds.width()/10,"dateline route has no world chord");
  touches_left=touches_left||near(line.x1(),bounds.left())||near(line.x2(),bounds.left());
  touches_right=touches_right||near(line.x1(),bounds.right())||near(line.x2(),bounds.right());
 }
 check(touches_left&&touches_right,"dateline splits to both edges");
 map.setEndpoints({0,0},{0,180});check(map.routeSegments().empty()&&map.routeSummary().contains("antipodal"),"antipodal route not invented");
 const auto description=map.accessibleDescription();bool rejected=false;
 try{map.setEndpoints({std::numeric_limits<double>::quiet_NaN(),0},{0,1});}catch(const std::invalid_argument&){rejected=true;}
 check(rejected&&map.accessibleDescription()==description,"invalid endpoint retains state");
 map.clearEndpoints();map.setAppearance("light");map.show();app.processEvents();
 QImage image(map.size(),QImage::Format_ARGB32);map.render(&image);
 check(image.pixelColor(12,200)==QColor("#dce8eb"),"rendered light ocean");
 auto geography_pixel=[&](double latitude,double longitude){const auto p=map.endpointPixel({latitude,longitude});return image.pixelColor(static_cast<int>(p.x()),static_cast<int>(p.y()));};
 check(geography_pixel(20,20)==QColor("#d6ddce"),"Sahara has real land fill");
 check(geography_pixel(0,-30)==QColor("#dce8eb"),"Atlantic remains ocean");
 check(geography_pixel(42,51)==QColor("#dce8eb"),"Caspian interior hole remains ocean");
 check(geography_pixel(-85,0)==QColor("#d6ddce"),"Antarctic polar closure remains land");
 map.setAppearance("dark");map.render(&image);check(image.pixelColor(12,200)==QColor("#374a51"),"rendered dark ocean");
 check(image.pixelColor(0,0)==QColor("#374a51"),"dark letterbox follows ocean rather than inherited light palette");
 QPalette palette;palette.setColor(QPalette::Window,QColor("#111111"));map.setPalette(palette);map.setAppearance("system");map.render(&image);
 check(image.pixelColor(12,200)==QColor("#374a51"),"system dark palette");
 palette.setColor(QPalette::Window,QColor("#fafafa"));map.setPalette(palette);map.render(&image);
 check(image.pixelColor(12,200)==QColor("#dce8eb"),"system palette change reflected");
 map.setAppearance("light");map.setEndpoints({0,0},{0,90});map.setRouteVisible(true);map.render(&image);
 const QColor route_pixel=image.pixelColor(424,200);
 check(route_pixel==QColor("#315e77"),"computed route rendered in Indian Ocean fixture");
 map.setRouteVisible(false);map.render(&image);
 check(image.pixelColor(424,200)==QColor("#dce8eb")&&image.pixelColor(424,200)!=route_pixel,
       "route hidden without painting invented geography");
 check(map.accessibleDescription().contains("Route hidden"),"route state described");
 check(map.accessibleName()=="World map"&&map.accessibleDescription().contains("Natural Earth"),"map has readable semantics");
 std::cout<<checks<<" map widget checks; "<<failures<<" failures\n";return failures?1:0;
}
