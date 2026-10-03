#include "../src/domain/geometry.hpp"
#include "../src/domain/locator_input.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
namespace {
std::string expected(unsigned longitude_cell,unsigned latitude_cell,unsigned precision){
 std::string grid;grid+=static_cast<char>('A'+longitude_cell/240);grid+=static_cast<char>('A'+latitude_cell/240);
 if(precision>=4){grid+=static_cast<char>('0'+longitude_cell%240/24);grid+=static_cast<char>('0'+latitude_cell%240/24);}
 if(precision==6){grid+=static_cast<char>('a'+longitude_cell%24);grid+=static_cast<char>('a'+latitude_cell%24);}
 return grid;
}
}
int main(){
 int failures=0,checks=0;
 auto check=[&](bool okay,const char* name){++checks;if(!okay){++failures;if(failures<=12)std::cerr<<"Locator boundary regression failed: "<<name<<'\n';}};
 check(hamclock::maidenhead(hamclock::location_from_maidenhead("AA00ac"),6)=="AA00ac","reported southwest corner round trip");
 const double negative_infinity=-std::numeric_limits<double>::infinity(),positive_infinity=std::numeric_limits<double>::infinity();
 // Exhaust every interior canonical boundary on both axes at all precisions.
 // Expected cell symbols are derived from the integral lattice, not encoder output.
 for(unsigned i=1;i<4320;++i){
  const double latitude=static_cast<double>(i)/24.0-90.0;
  const double longitude=static_cast<double>(i)/12.0-180.0;
  for(unsigned precision:{2U,4U,6U}){
   check(hamclock::maidenhead({latitude,0},precision)==expected(2160,i,precision),"latitude exact boundary enters north");
   check(hamclock::maidenhead({std::nextafter(latitude,negative_infinity),0},precision)==expected(2160,i-1,precision),"latitude immediate below enters south");
   check(hamclock::maidenhead({std::nextafter(latitude,positive_infinity),0},precision)==expected(2160,i,precision),"latitude immediate above remains north");
   check(hamclock::maidenhead({0,longitude},precision)==expected(i,2160,precision),"longitude exact boundary enters east");
   check(hamclock::maidenhead({0,std::nextafter(longitude,negative_infinity)},precision)==expected(i-1,2160,precision),"longitude immediate below enters west");
   check(hamclock::maidenhead({0,std::nextafter(longitude,positive_infinity)},precision)==expected(i,2160,precision),"longitude immediate above remains east");
  }
 }
 for(unsigned precision:{2U,4U,6U}){
  check(hamclock::maidenhead({-90,-180},precision)==expected(0,0,precision),"southwest endpoints");
  check(hamclock::maidenhead({std::nextafter(-90.0,positive_infinity),std::nextafter(-180.0,positive_infinity)},precision)==expected(0,0,precision),"inside southwest endpoint neighbours");
  check(hamclock::maidenhead({90,180},precision)==expected(0,4319,precision),"north endpoint and east wraps west");
  check(hamclock::maidenhead({std::nextafter(90.0,negative_infinity),std::nextafter(180.0,negative_infinity)},precision)==expected(4319,4319,precision),"inside northeast endpoint neighbours");
 }
 for(const auto bad:{hamclock::Location{std::nextafter(-90.0,negative_infinity),0},
                     hamclock::Location{std::nextafter(90.0,positive_infinity),0},
                     hamclock::Location{0,std::nextafter(-180.0,negative_infinity)},
                     hamclock::Location{0,std::nextafter(180.0,positive_infinity)}}){
  bool rejected=false;try{(void)hamclock::maidenhead(bad);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected,"outside endpoint neighbours rejected");
 }
 // Axes are independently classified. Pair every axis corner with representative
 // opposite-axis field/square/subsquare/zero/end transitions, rather than treating
 // a selected interior point as evidence for fractional-corner round trips.
 for(unsigned i=0;i<4320;++i)for(unsigned partner:{0U,1U,23U,24U,239U,240U,2159U,2160U,4319U}){
  for(const auto& grid:{expected(i,partner,6),expected(partner,i,6)})
   check(hamclock::maidenhead(hamclock::location_from_maidenhead(grid),6)==grid,"paired six-character decoder corner round trip");
 }
 failures+=hamclock::run_domain_tests();
 std::cout<<checks<<" locator boundary checks plus existing domain tests; "<<failures<<" failures\n";
 return failures?1:0;
}
