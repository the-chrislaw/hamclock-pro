#include "../src/qt/world_map_widget.hpp"
#include <QApplication>
#include <QImage>
#include <QPalette>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

int main(int argc,char** argv) {
    QApplication app(argc,argv);
    hamclock::WorldMapWidget map;
    int checks=0,failures=0;
    const auto check=[&](bool yes,const char* name){
        ++checks;if(!yes){++failures;std::cerr<<"Land widget failure: "<<name<<'\n';}
    };
    const auto image_of=[&](){QImage image(map.size(),QImage::Format_ARGB32);image.fill(Qt::magenta);map.render(&image);return image;};
    const auto pixel=[&](const QImage& image,double lat,double lon){
        const auto point=map.endpointPixel({lat,lon});
        return image.pixelColor(static_cast<int>(point.x()),static_cast<int>(point.y()));
    };
    // Source geometry classifies these cities as land. At world-map resolution
    // a cosmetic coast stroke can cover the point pixel. A three-pixel local
    // neighborhood must still contain the actual fill, not just the outline.
    const auto nearby_land=[&](const QImage& image,double lat,double lon,QColor color){
        const auto p=map.endpointPixel({lat,lon});
        for(int y=static_cast<int>(p.y())-1;y<=static_cast<int>(p.y())+1;++y)
            for(int x=static_cast<int>(p.x())-1;x<=static_cast<int>(p.x())+1;++x)
                if(image.pixelColor(x,y)==color)return true;
        return false;
    };
    for(const QSize size:{QSize(800,480),QSize(1280,800)}) {
        map.resize(size);
        for(const bool dark:{false,true}) {
            for(const bool system:{false,true}) {
                QPalette palette;palette.setColor(QPalette::Window,QColor(dark?"#111111":"#fafafa"));
                map.setPalette(palette);map.setAppearance(system?"system":dark?"dark":"light");
                map.clearEndpoints();map.setRouteVisible(true);
                const QColor land(dark?"#29363b":"#d6ddce"),sea(dark?"#374a51":"#dce8eb"),route(dark?"#b4d9e8":"#315e77");
                const auto image=image_of();
                check(image.pixelColor(0,0)==sea,"letterbox follows active ocean theme");
                check(pixel(image,20,20)==land && pixel(image,-25,135)==land,"safe continental interiors filled");
                check(nearby_land(image,51.5074,-0.1278,land),"London neighborhood contains land fill");
                check(nearby_land(image,35.6895,139.6917,land),"Tokyo neighborhood contains land fill");
                check(nearby_land(image,34.0522,-118.2437,land),"Los Angeles neighborhood contains land fill");
                check(pixel(image,0,-30)==sea && pixel(image,0,-140)==sea,"Atlantic and Pacific interiors remain ocean");
                check(pixel(image,42,51)==sea,"Caspian source hole remains ocean");
                check(pixel(image,-85,0)==land,"full-width Antarctica remains land");
                map.setEndpoints({20,0},{20,30});
                const auto with_route=image_of();
                bool line_on_land=false;
                // Select the middle of a route segment whose unadorned center
                // is land. Its rendered route must appear above the fill.
                const auto lines=map.routeSegments();
                for(const auto& line:lines) {
                    const QPointF p=(line.p1()+line.p2())/2;
                    const int x=static_cast<int>(std::lround(p.x())),y=static_cast<int>(std::lround(p.y()));
                    if(image.pixelColor(x,y)==land && with_route.pixelColor(x,y)==route){line_on_land=true;break;}
                }
                check(line_on_land,"computed route visible above continental fill");
                // Circle/square centers have ocean brush and therefore expose
                // whether markers were overwritten by the underlying land.
                check(pixel(image,20,0)==land && pixel(image,20,30)==land,"both marker fixtures start in land interiors");
                check(pixel(with_route,20,0)==sea && pixel(with_route,20,30)==sea,"DE and DX marker centers remain above land fill");
                for(const auto location:{hamclock::Location{20,0},hamclock::Location{20,30}}) {
                    const auto point=map.endpointPixel(location);
                    bool outline=false;
                    for(int y=static_cast<int>(point.y())-5;y<=static_cast<int>(point.y())+5;++y)
                        for(int x=static_cast<int>(point.x())-5;x<=static_cast<int>(point.x())+5;++x)
                            outline=outline || with_route.pixelColor(x,y)==route;
                    check(outline,"endpoint outline visible above land fill");
                }
                const auto before=map.accessibleDescription();
                bool rejected=false;
                try{map.setEndpoints({std::numeric_limits<double>::quiet_NaN(),0},{0,0});}
                catch(const std::invalid_argument&){rejected=true;}
                check(rejected && map.accessibleDescription()==before && image_of()==with_route,"invalid endpoints retain rendered state");
                rejected=false;try{map.setAppearance("invalid");}catch(const std::invalid_argument&){rejected=true;}
                check(rejected && image_of()==with_route,"invalid appearance retains rendered state");
                map.setRouteVisible(false);const auto hidden=image_of();
                bool restored_land=false;
                for(const auto& line:lines) {
                    const QPointF p=(line.p1()+line.p2())/2;
                    const int x=static_cast<int>(std::lround(p.x())),y=static_cast<int>(std::lround(p.y()));
                    if(image.pixelColor(x,y)==land && with_route.pixelColor(x,y)==route && hidden.pixelColor(x,y)==land){restored_land=true;break;}
                }
                check(restored_land,"hiding route restores geographic land fill");
                check(hidden.pixelColor(0,0)==sea,"overlays stay out of letterbox");
            }
        }
    }
    std::cout<<checks<<" filled map widget checks; "<<failures<<" failures\n";
    return failures?1:0;
}
