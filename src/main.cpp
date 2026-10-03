// Original HamClock Pro dashboard foundation. Illustrative, offline data only.
#include <SDL.h>
#include "app/commands.hpp"
#include "domain/geometry.hpp"
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

namespace {
constexpr int W=800,H=480;
struct Box { int x,y,w,h; bool contains(int px,int py) const { return px>=x&&py>=y&&px<x+w&&py<y+h; } };
constexpr std::array<Box,3> panes={Box{205,68,190,112},Box{402,68,190,112},Box{599,68,190,112}};
constexpr std::array<const char*,5> titles={"SPACE WEATHER","BAND CONDITIONS","SOLAR ACTIVITY","DX SPOTS","SATELLITES"};
struct Model { std::array<unsigned,3> selected{0,1,2}; bool click(int x,int y) { for(unsigned i=0;i<panes.size();++i) if(panes[i].contains(x,y)) { selected[i]=(selected[i]+1)%titles.size(); return true; } return false; } };
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
 color(r,229,238,245);text(r,24,113,"DEMO STATION",2);color(r,138,158,177);text(r,24,141,"LOCATION NOT SET");
 color(r,229,238,245);text(r,24,175,"DX / TARGET",2);color(r,138,158,177);text(r,24,206,"SELECT A DESTINATION");
 text(r,24,241,"NO LIVE RADIO DATA");text(r,24,261,"NO NETWORK REQUESTS");
 color(r,98,207,181);text(r,24,308,"SUN / MOON",2);color(r,138,158,177);text(r,24,339,"NOT CALCULATED");
 text(r,24,416,"ESC TO EXIT");text(r,24,438,"CLICK PANE TO CYCLE");
 for(unsigned i=0;i<panes.size();++i) { const auto b=panes[i];panel(r,b);color(r,229,238,245);text(r,b.x+10,b.y+10,titles[m.selected[i]]);color(r,138,158,177);text(r,b.x+10,b.y+29,"ILLUSTRATIVE PREVIEW");color(r,98,207,181);
  for(int n=0;n<10;++n) { const int height=12+((n*17+i*11)%37);fill(r,{b.x+12+n*16,b.y+94-height,8,height}); }
 }
 Box map{205,188,584,280};panel(r,map);color(r,138,158,177);text(r,217,200,"WORLD VIEW / ORIGINAL SCHEMATIC / NOT A NAVIGATION MAP");
 // Deliberately schematic original land silhouettes; no external map assets.
 color(r,29,49,64);for(int x=map.x+24;x<map.x+map.w;x+=48)SDL_RenderDrawLine(r,x,map.y+34,x,map.y+map.h-24);
 for(int y=map.y+42;y<map.y+map.h-18;y+=36)SDL_RenderDrawLine(r,map.x+12,y,map.x+map.w-12,y);
 const std::vector<std::vector<SDL_Point>> shapes={{{246,258},{280,238},{315,251},{344,245},{328,273},{299,284},{286,308},{267,284},{246,258}},{{306,313},{329,324},{343,356},{327,395},{312,414},{299,365},{306,313}},{{465,252},{493,244},{515,262},{510,279},{531,303},{508,348},{484,360},{469,330},{463,296},{465,252}},{{520,256},{560,235},{608,248},{650,239},{701,267},{678,284},{646,280},{620,312},{587,299},{563,278},{520,256}},{{646,362},{686,345},{723,372},{710,393},{672,400},{646,362}}};
 color(r,81,135,143);for(const auto&s:shapes)SDL_RenderDrawLines(r,s.data(),static_cast<int>(s.size()));
 color(r,98,207,181);SDL_RenderDrawLine(r,283,280,536,278);fill(r,{280,277,6,6});fill(r,{533,275,6,6});
 color(r,138,158,177);text(r,217,447,"DEMO GEOMETRY / FEATURES NOT YET IMPLEMENTED");SDL_RenderPresent(r);
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
 if(argc==2&&std::strcmp(argv[1],"--self-test")==0) { bool ok=test();std::puts(ok?"Pane geometry and interaction tests passed":"Pane tests failed");return (ok&&hamclock::run_domain_tests()==0&&hamclock::run_command_tests()==0)?0:1; }
 const bool headless=argc==3&&std::strcmp(argv[1],"--headless-frame")==0;
 if(argc!=1&&!headless) {std::fprintf(stderr,"Usage: %s [--self-test | --headless-frame PATH.bmp | --calculate DE_LAT DE_LON DX_LAT DX_LON]\n",argv[0]);return 2;}
 if(SDL_Init(headless?0:SDL_INIT_VIDEO)!=0){std::fprintf(stderr,"SDL init: %s\n",SDL_GetError());return 1;}
 SDL_Window*w=nullptr;SDL_Surface*s=nullptr;SDL_Renderer*r=nullptr;
 if(headless){s=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_ARGB8888);if(s)r=SDL_CreateSoftwareRenderer(s);}
 else { w=SDL_CreateWindow("HamClock Pro - Offline foundation",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_RESIZABLE);if(w)r=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);if(w&&!r)r=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE); }
 if(!r){std::fprintf(stderr,"Renderer: %s\n",SDL_GetError());if(s)SDL_FreeSurface(s);if(w)SDL_DestroyWindow(w);SDL_Quit();return 1;}
 SDL_RenderSetLogicalSize(r,W,H);Model model;int result=0;
 if(headless){render(r,model);if(SDL_SaveBMP(s,argv[2])!=0){std::fprintf(stderr,"Save frame: %s\n",SDL_GetError());result=1;}}
 else { bool running=true;while(running){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT||(e.type==SDL_KEYDOWN&&e.key.keysym.sym==SDLK_ESCAPE))running=false;if(e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT)model.click(e.button.x,e.button.y);}render(r,model);SDL_Delay(25);} }
 SDL_DestroyRenderer(r);if(s)SDL_FreeSurface(s);if(w)SDL_DestroyWindow(w);SDL_Quit();return result;
}
