#include "../src/domain/locator_input.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
int main(){
 int failures=0,checks=0;
 auto check=[&](bool pass,const char* name){++checks;if(!pass){++failures;std::cerr<<"Locator input regression failed: "<<name<<'\n';}};
 auto fixture=[&](const std::string& grid,double latitude,double longitude){
  const auto point=hamclock::location_from_maidenhead(grid);
  check(std::abs(point.latitude-latitude)<1e-12 && std::abs(point.longitude-longitude)<1e-12,"independent southwest corner fixture");
 };
 fixture("AA",-90,-180);fixture("JJ",0,0);fixture("RR",80,160);
 fixture("AA00",-90,-180);fixture("JJ00",0,0);fixture("RR99",89,178);
 fixture("AA00aa",-90,-180);fixture("JJ00ab",1.0/24,0);fixture("JJ00ba",0,1.0/12);
 fixture("JJ00xx",23.0/24,23.0/12);fixture("RR99xx",89+23.0/24,178+23.0/12);
 // CM87: C=2,M=12 fields, square8/7 => west=-124,north=37.
 fixture("CM87ss",37.75,-122.5);fixture("IO91wm",51.5,-1.0/6);
 fixture("aA00aA",-90,-180);fixture("cm87SS",37.75,-122.5);
 for(const std::string bad:{"","A","AAA","AA0","AA000","AA00aaa","AA00aa00","SA","AS","JJAA","JJ-0","JJ00ya","JJ00ay"," JJ00","JJ00 ","JJ00a ","JJ00a\n","JJ00!a"}){
  bool rejected=false;try{(void)hamclock::location_from_maidenhead(bad);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected,"malformed length or pair rejected");
 }
 for(const auto& bad:{std::string("JJ00a\0",6),std::string("\xff\xff",2),std::string(100000,'A')}){
  bool rejected=false;try{(void)hamclock::location_from_maidenhead(bad);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected,"NUL nonASCII and oversized input rejected");
 }
 // All 324 fields and all 32,400 squares have integer-degree corners.
 // Round-trip these exact boundaries independently of fractional rounding.
 for(unsigned x=0;x<18;++x)for(unsigned y=0;y<18;++y){
  std::string grid;grid+=static_cast<char>('A'+x);grid+=static_cast<char>('A'+y);
  const auto point=hamclock::location_from_maidenhead(grid);
  check(hamclock::valid_location(point)&&hamclock::maidenhead(point,2)==grid,"every field integer corner round trip");
  for(unsigned sx=0;sx<10;++sx)for(unsigned sy=0;sy<10;++sy){
   const auto square=grid+static_cast<char>('0'+sx)+static_cast<char>('0'+sy);
   const auto sw=hamclock::location_from_maidenhead(square);
   check(hamclock::valid_location(sw)&&hamclock::maidenhead(sw,4)==square,"every square integer corner round trip");
  }
 }
 // Fractional corners may lie one ULP on either side of a cell boundary.
 // Verify encoding safely inside representative cells and geographic bounds;
 // do not move the decoded southwest corner to hide that numerical issue.
 for(unsigned x:{0U,8U,17U})for(unsigned y:{0U,9U,17U})
  for(unsigned sx:{0U,5U,9U})for(unsigned sy:{0U,5U,9U})
   for(unsigned ax=0;ax<24;++ax)for(unsigned ay=0;ay<24;++ay){
    std::string grid;grid+=static_cast<char>('A'+x);grid+=static_cast<char>('A'+y);
    grid+=static_cast<char>('0'+sx);grid+=static_cast<char>('0'+sy);
    grid+=static_cast<char>('a'+ax);grid+=static_cast<char>('a'+ay);
    const auto sw=hamclock::location_from_maidenhead(grid);
    const hamclock::Location center{sw.latitude+1.0/48,sw.longitude+1.0/24};
    check(hamclock::valid_location(sw)&&sw.latitude<90 && sw.longitude<180 &&
          hamclock::maidenhead(center,6)==grid,"subsquare bounds and independent interior encode");
   }
 std::cout<<checks<<" locator input checks; "<<failures<<" failures\n";return failures?1:0;
}
