// Original HamClock Pro dashboard foundation. Illustrative, offline data only.
#include <SDL.h>
#include "app/commands.hpp"
#include "app/station_view.hpp"
#include "domain/geometry.hpp"
#include "platform/settings.hpp"
#include "map/coastline.hpp"
#include "map/projection.hpp"
#include "map/path.hpp"
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <optional>
#include <algorithm>
#include <utility>
#include <vector>

namespace {
constexpr int W=800,H=480;
struct Box { int x,y,w,h; bool contains(int px,int py) const { return px>=x&&py>=y&&px<x+w&&py<y+h; } };
constexpr std::array<Box,3> panes={Box{205,68,190,112},Box{402,68,190,112},Box{599,68,190,112}};
constexpr std::array<const char*,5> titles={"SPACE WEATHER","BAND CONDITIONS","SOLAR ACTIVITY","DX SPOTS","SATELLITES"};
struct Model { std::array<unsigned,3> selected{0,1,2}; std::optional<hamclock::StationView> station; std::vector<hamclock::Location> route; bool click(int x,int y) { for(unsigned i=0;i<panes.size();++i) if(panes[i].contains(x,y)) { selected[i]=(selected[i]+1)%titles.size(); return true; } return false; } };
// Five-column glyphs, authored for this prototype. Bits run top to bottom.
const unsigned char* glyph(char c) {
 static const unsigned char letters[][5]={
 {126,9,9,9,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},
 {127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},
 {65,65,127,65,65},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
 {127,2,12,2,127},{127,2,4,8,127},{62,65,65,65,62},{127,9,9,9,6},
 {62,65,81,33,94},{127,9,25,41,70},{38,73,73,73,50},{1,1,127,1,1},
 {63,64,64,64,63},{31,32,64,32,31},{127,32,24,32,127},{99,20,8,20,99},
 {3,4,120,4,3},{97,81,73,69,67}};
 static const unsigned char digits[][5]={{62,81,73,69,62},{0,66,127,64,0},{98,81,73,73,70},{34,65,73,73,54},{24,20,18,127,16},{39,69,69,69,57},{62,73,73,73,48},{1,113,9,5,3},{54,73,73,73,54},{6,73,73,73,62}};
 static const unsigned char blank[5]={0,0,0,0,0}, colon[5]={0,0,36,0,0}, dash[5]={8,8,8,8,8}, dot[5]={0,96,96,0,0}, slash[5]={64,32,16,8,4};
 if(c>='A'&&c<='Z') return letters[c-'A']; if(c>='0'&&c<='9') return digits[c-'0'];
 switch(c) { case ':':return colon; case '-':return dash; case '.':return dot; case '/':return slash; default:return blank; }
}
void color(SDL_Renderer* r,Uint8 a,Uint8 b,Uint8 c) { SDL_SetRenderDrawColor(r,a,b,c,255); }
void text(SDL_Renderer*r,int x,int y,const std::string&s,int scale=1) { for(char c:s) { const auto*g=glyph(c); for(int i=0;i<5;++i)for(int j=0;j<7;++j)if(g[i]&(1<<j)){SDL_Rect q{x+i*scale,y+j*scale,scale,scale};SDL_RenderFillRect(r,&q);}x+=6*scale; } }
void fill(SDL_Renderer*r,Box b) { SDL_Rect q{b.x,b.y,b.w,b.h};SDL_RenderFillRect(r,&q); }
void panel(SDL_Renderer*r,Box b) { color(r,19,32,48);fill(r,b);color(r,43,60,77);SDL_Rect q{b.x,b.y,b.w,b.h};SDL_RenderDrawRect(r,&q); }
void render(SDL_Renderer*r,const Model&m) {
 color(r,9,17,28);SDL_RenderClear(r);
 color(r,229,238,245);text(r,16,17,"HAMCLOCK PRO",2);
 color(r,98,207,181);text(r,16,42,"NATIVE FOUNDATION / DEMO / OFFLINE");
 const auto now=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); std::tm tm{};
#if defined(_WIN32)
 gmtime_s(&tm,&now);
#else
 gmtime_r(&now,&tm);
#endif
 char clock[32];std::strftime(clock,sizeof(clock),"%H:%M:%S UTC",&tm);
 color(r,229,238,245);text(r,600,17,clock,2); color(r,138,158,177);text(r,600,43,"SYSTEM CLOCK");
 panel(r,{12,68,183,400});color(r,98,207,181);text(r,24,82,"DE / HOME",2);
 color(r,229,238,245);text(r,24,113,m.station?m.station->callsign:"DEMO STATION");color(r,138,158,177);text(r,24,141,m.station?m.station->de_grid:"LOCATION NOT SET");
 color(r,229,238,245);text(r,24,175,"DX / TARGET",2);color(r,138,158,177);text(r,24,206,m.station?m.station->dx_grid:"SELECT A DESTINATION");
 text(r,24,241,m.station?m.station->distance_text:"NO LIVE RADIO DATA");text(r,24,261,m.station?m.station->bearing_text:"NO NETWORK REQUESTS");
 color(r,98,207,181);text(r,24,308,"SUN / MOON",2);color(r,138,158,177);text(r,24,339,"NOT CALCULATED");
 text(r,24,416,"ESC TO EXIT");text(r,24,438,"CLICK PANE TO CYCLE");
 for(unsigned i=0;i<panes.size();++i) { const auto b=panes[i];panel(r,b);color(r,229,238,245);text(r,b.x+10,b.y+10,titles[m.selected[i]]);color(r,138,158,177);text(r,b.x+10,b.y+29,"ILLUSTRATIVE PREVIEW");color(r,98,207,181);
  for(int n=0;n<10;++n) { const int height=12+((n*17+i*11)%37);fill(r,{b.x+12+n*16,b.y+94-height,8,height}); }
 }
 Box map{205,188,584,280};panel(r,map);color(r,138,158,177);text(r,217,200,"WORLD VIEW / REAL COASTLINES / EQUIRECTANGULAR");
 const Box earth{287,226,420,210};
 auto pixel = [&](hamclock::MapPoint p) { return SDL_Point{earth.x+static_cast<int>(p.x*earth.w),earth.y+static_cast<int>(p.y*earth.h)}; };
 color(r,29,49,64);
 for(int i=0;i<=12;++i)SDL_RenderDrawLine(r,earth.x+i*earth.w/12,earth.y,earth.x+i*earth.w/12,earth.y+earth.h);
 for(int i=0;i<=6;++i)SDL_RenderDrawLine(r,earth.x,earth.y+i*earth.h/6,earth.x+earth.w,earth.y+i*earth.h/6);
 color(r,125,177,183);
 for(const auto&part:hamclock::coastline_parts) for(std::size_t i=1;i<part.count;++i) {
  const auto a=hamclock::coastline_points[part.start+i-1],b=hamclock::coastline_points[part.start+i];
  const auto split=hamclock::map_split_equirectangular({a.latitude,a.longitude},{b.latitude,b.longitude});
  for(std::size_t j=0;j<split.count;++j){const auto first=pixel(split.segments[j].first),second=pixel(split.segments[j].second);SDL_RenderDrawLine(r,first.x,first.y,second.x,second.y);}
 }
 if(m.station){
  color(r,98,207,181);
  for(std::size_t i=1;i<m.route.size();++i){const auto split=hamclock::map_split_equirectangular(m.route[i-1],m.route[i]);for(std::size_t j=0;j<split.count;++j){const auto a=pixel(split.segments[j].first),b=pixel(split.segments[j].second);SDL_RenderDrawLine(r,a.x,a.y,b.x,b.y);}}
  for(const auto&entry:std::array<std::pair<hamclock::Location,const char*>,2>{{{m.station->de,"DE"},{m.station->dx,"DX"}}}){const auto p=pixel(hamclock::map_project_equirectangular(entry.first));fill(r,{p.x-2,p.y-2,5,5});text(r,std::min(p.x+5,earth.x+earth.w-12),std::max(earth.y,p.y-9),entry.second);}
 }
 color(r,138,158,177);text(r,217,447,m.station?(m.route.empty()?"NATURAL EARTH / ROUTE UNDEFINED / OFFLINE":"NATURAL EARTH / SHORT PATH GEOMETRY / OFFLINE"):"NATURAL EARTH 110M / OFFLINE / STATION NOT SET");SDL_RenderPresent(r);
}
bool test() {
 Model m; if(m.click(-1,80)||m.click(800,480)||m.click(395,70))return false;
 if(!m.click(205,68)||m.selected[0]!=1||m.selected[1]!=1)return false;
 for(int i=0;i<4;++i)m.click(205,68);if(m.selected[0]!=0)return false;
 if(!m.click(788,179)||m.selected[2]!=3||m.click(789,180))return false;
 for(const auto&b:panes) if(b.x<0||b.y<0||b.x+b.w>W||b.y+b.h>H)return false;
 return true;
}
}
int main(int argc,char**argv) {
 if(argc>=2&&std::strcmp(argv[1],"--calculate")==0) return hamclock::calculate_command(argc,argv);
 if(argc>=2&&(std::strcmp(argv[1],"--save-station")==0||std::strcmp(argv[1],"--load-station")==0)) return hamclock::settings_command(argc,argv);
 if(argc==2&&std::strcmp(argv[1],"--self-test")==0) { bool ok=test();std::puts(ok?"Pane geometry and interaction tests passed":"Pane tests failed");return (ok&&hamclock::run_domain_tests()==0&&hamclock::run_command_tests()==0&&hamclock::run_settings_tests()==0&&hamclock::run_map_tests()==0&&hamclock::run_station_view_tests()==0&&hamclock::run_path_tests()==0)?0:1; }
 const bool station_mode=(argc==3||argc==5)&&std::strcmp(argv[1],"--station")==0;
 const bool headless=(argc==3&&std::strcmp(argv[1],"--headless-frame")==0)||(station_mode&&argc==5&&std::strcmp(argv[3],"--headless-frame")==0);
 if(argc!=1&&!headless&&!(station_mode&&argc==3)) {std::fprintf(stderr,"Usage: %s [--station PATH [--headless-frame PATH.bmp] | --self-test | --headless-frame PATH.bmp | --calculate DE_LAT DE_LON DX_LAT DX_LON | --save-station PATH CALLSIGN DE_LAT DE_LON DX_LAT DX_LON system|light|dark | --load-station PATH]\n",argv[0]);return 2;}
 Model model;
 if(station_mode){try{model.station=hamclock::make_station_view(hamclock::load_settings(argv[2]));if(hamclock::geometry(model.station->de,model.station->dx).status!=hamclock::BearingStatus::Antipodal)model.route=hamclock::map_short_path(model.station->de,model.station->dx);}catch(const std::exception&error){std::fprintf(stderr,"Station startup error: %s\n",error.what());return 2;}}
 if(SDL_Init(headless?0:SDL_INIT_VIDEO)!=0){std::fprintf(stderr,"SDL init: %s\n",SDL_GetError());return 1;}
 SDL_Window*w=nullptr;SDL_Surface*s=nullptr;SDL_Renderer*r=nullptr;
 if(headless){s=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);if(s)r=SDL_CreateSoftwareRenderer(s);}
 else { w=SDL_CreateWindow("HamClock Pro - Offline foundation",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_RESIZABLE);if(w)r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);if(w&&!r)r=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE); }
 if(!r){std::fprintf(stderr,"Renderer: %s\n",SDL_GetError());if(s)SDL_FreeSurface(s);if(w)SDL_DestroyWindow(w);SDL_Quit();return 1;}
 SDL_RenderSetLogicalSize(r,W,H);int result=0;
 if(headless){render(r,model);if(SDL_SaveBMP(s,argv[station_mode?4:2])!=0){std::fprintf(stderr,"Save frame: %s\n",SDL_GetError());result=1;}}
 else { bool running=true,dirty=true;std::time_t previous=0;while(running){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT||(e.type==SDL_KEYDOWN&&e.key.keysym.sym==SDLK_ESCAPE))running=false;if(e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT)dirty=model.click(e.button.x,e.button.y)||dirty;if(e.type==SDL_WINDOWEVENT)dirty=true;}const auto now=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());if(running&&(dirty||now!=previous)){render(r,model);previous=now;dirty=false;}SDL_Delay(25);} }
 SDL_DestroyRenderer(r);if(s)SDL_FreeSurface(s);if(w)SDL_DestroyWindow(w);SDL_Quit();return result;
}
