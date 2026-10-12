#include "../src/qt/world_map_widget.hpp"
#include <QApplication>
#include <QImage>
#include <cmath>
#include <iostream>
int main(int argc,char** argv){
 QApplication app(argc,argv);hamclock::WorldMapWidget map;int failures=0;
 auto check=[&](bool yes,const char* name){if(!yes){++failures;std::cerr<<"Terminator regression failed: "<<name<<'\n';}};
 map.resize(800,480);map.setAppearance("light");
 check(!map.subsolarPixel()&&map.nightPolygon().isEmpty(),"no shade before a time is set");
 // 2026-06-21 12:00 UTC is within a day of the June solstice: the Sun is near
 // 23.4 N, so night lies south of the terminator and the north pole is lit.
 const hamclock::UtcSeconds when{1782043200};
 map.setSolarTime(when);
 const auto sun=hamclock::subsolar_point(when);
 check(std::abs(sun.latitude_deg-23.44)<0.1,"solstice declination (NOAA solar calculator)");
 const auto marker=map.subsolarPixel();check(marker.has_value(),"marker present");
 const auto bounds=map.mapBounds();
 const auto poly=map.nightPolygon();check(poly.size()>300,"polygon has terminator samples");
 // Every terminator vertex (all but the two closing corners) is at 0 elevation.
 for(int i=0;i<poly.size()-2;++i){
  const double lon=(poly[i].x()-bounds.left())/bounds.width()*360.0-180.0,lat=90.0-(poly[i].y()-bounds.top())/bounds.height()*180.0;
  if(std::abs(hamclock::solar_elevation_deg(sun,lat,lon))>1e-6){check(false,"vertex on terminator");break;}
 }
 const QPointF anti=map.endpointPixel({-sun.latitude_deg,sun.longitude_deg>0?sun.longitude_deg-180:sun.longitude_deg+180});
 check(poly.containsPoint(anti,Qt::OddEvenFill),"antipode is in night");
 check(marker&&!poly.containsPoint(*marker,Qt::OddEvenFill),"subsolar point is in day");
 check(!poly.containsPoint(map.endpointPixel({85,0}),Qt::OddEvenFill),"north pole lit at June solstice");
 check(poly.containsPoint(map.endpointPixel({-85,sun.longitude_deg+180>180?sun.longitude_deg-180:sun.longitude_deg+180}),Qt::OddEvenFill),"south pole region dark");
 QImage shaded(map.size(),QImage::Format_ARGB32);map.render(&shaded);
 map.clearSolarTime();QImage plain(map.size(),QImage::Format_ARGB32);map.render(&plain);
 check(shaded.pixelColor(anti.toPoint())!=plain.pixelColor(anti.toPoint()),"night pixel is darker than unshaded");
 check(shaded.pixelColor(anti.toPoint()).lightness()<plain.pixelColor(anti.toPoint()).lightness(),"shade darkens");
 check(shaded.pixelColor(2,2)==plain.pixelColor(2,2),"letterbox untouched");
 check(map.nightPolygon().isEmpty(),"clear removes shade");
 return failures?1:0;
}
