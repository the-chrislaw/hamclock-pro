#include "../src/domain/clock_state.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace {
using namespace hamclock;
static_assert(!std::is_convertible_v<UtcSeconds, MonotonicNanoseconds>);
static_assert(!std::is_convertible_v<MonotonicNanoseconds, UtcSeconds>);
static_assert(!std::is_convertible_v<UtcOffsetSeconds, UtcSeconds>);
static_assert(!std::is_convertible_v<std::int64_t, UtcSeconds>);
bool same(const ClockSnapshot& a,const ClockSnapshot& b){
 return a.actual_utc.value==b.actual_utc.value && a.display_utc.value==b.display_utc.value
     && a.preview==b.preview && a.elapsed.value==b.elapsed.value && a.synchronization==b.synchronization;
}
}
int main(){
 using namespace hamclock;
 int failures=0,checks=0;
 auto check=[&](bool pass,const char* name){++checks;if(!pass){++failures;std::cerr<<"Clock regression failed: "<<name<<'\n';}};
 auto invalid=[&](auto action,const char* name){bool expected=false;try{action();}catch(const std::invalid_argument&){expected=true;}catch(...){ }check(expected,name);};
 auto overflow=[&](auto action,const char* name){bool expected=false;try{action();}catch(const std::overflow_error&){expected=true;}catch(...){ }check(expected,name);};
 constexpr auto lo=std::numeric_limits<std::int64_t>::min(),hi=std::numeric_limits<std::int64_t>::max();
 invalid([&](){ClockState state({0},{0},{{-1}});},"negative freshness policy rejected");
 ClockState clock({1709164799},{100},{{20}});
 check(clock.snapshot().actual_utc.value==1709164799 && clock.snapshot().display_utc.value==1709164799,"initial supplied UTC retained");
 check(!clock.snapshot().preview && clock.snapshot().elapsed.value==0 && clock.snapshot().synchronization==SyncStatus::Unknown,"initial state has exact Unknown quality and zero elapsed");
 auto snap=clock.advance({1709164800},{110});
 check(snap.actual_utc.value==1709164800 && snap.elapsed.value==10,"injected leap-day midnight epoch boundary advances");
 check(snap.synchronization==SyncStatus::Unknown,"reading actual UTC alone never implies synchronization");
 snap=clock.advance({1709251200},{120},SyncEvidence{SyncStatus::Synchronized,{100}});
 check(snap.synchronization==SyncStatus::Synchronized,"evidence exactly maximum age accepted");
 check(snap.elapsed.value==20,"elapsed independent of day-size UTC jump");
 snap=clock.advance({1},{121},SyncEvidence{SyncStatus::Synchronized,{100}});
 check(snap.synchronization==SyncStatus::Unknown,"stale evidence falls back to exact Unknown");
 check(snap.actual_utc.value==1 && snap.elapsed.value==21,"backward UTC jump preserves monotonic elapsed");
 snap=clock.advance({2},{122},SyncEvidence{SyncStatus::Synchronized,{123}});
 check(snap.synchronization==SyncStatus::Unknown,"future evidence is Unknown");
 snap=clock.advance({3},{123},SyncEvidence{SyncStatus::Unsynchronized,{122}});
 check(snap.synchronization==SyncStatus::Unsynchronized,"fresh explicit Unsynchronized preserved");
 snap=clock.advance({4},{123},SyncEvidence{SyncStatus::Unknown,{123}});
 check(snap.synchronization==SyncStatus::Unknown && snap.elapsed.value==23,"unknown evidence and repeated monotonic allowed");
 clock.advance({5},{124},SyncEvidence{SyncStatus::Synchronized,{124}});
 const auto real=clock.snapshot();clock.set_preview_offset({3600});snap=clock.snapshot();
 check(snap.preview && snap.display_utc.value==3605,"preview displays checked offset");
 check(snap.actual_utc.value==real.actual_utc.value && snap.elapsed.value==real.elapsed.value && snap.synchronization==real.synchronization,"preview changes neither source UTC nor elapsed nor quality");
 snap=clock.advance({6},{125});
 check(snap.preview && snap.display_utc.value==3606 && snap.elapsed.value==25,"preview advances using independent actual UTC and monotonic time");
 check(snap.synchronization==SyncStatus::Unknown,"absent evidence cannot retain previous healthy status");
 clock.set_preview_offset({-3600});check(clock.snapshot().display_utc.value==-3594,"negative offset supported without source mutation");
 clock.set_preview_offset({0});check(clock.snapshot().preview && clock.snapshot().display_utc.value==6,"explicit zero-offset preview remains labelled");
 clock.reset_preview();check(!clock.snapshot().preview && clock.snapshot().display_utc.value==6 && clock.snapshot().elapsed.value==25,"reset restores actual display without duration reset");
 clock.reset_preview();check(!clock.snapshot().preview,"reset is idempotent");
 auto previous=clock.snapshot();
 invalid([&](){clock.advance({99},{124});},"reversed monotonic rejected");check(same(clock.snapshot(),previous),"reverse rejection preserves snapshot");
 invalid([&](){clock.advance({99},{126},SyncEvidence{static_cast<SyncStatus>(99),{126}});},"unknown enum rejected");check(same(clock.snapshot(),previous),"invalid enum preserves snapshot and last monotonic");
 invalid([&](){clock.advance({99},{126},SyncEvidence{static_cast<SyncStatus>(-1),{0}});},"negative invalid enum rejected even if stale");
 clock.advance({7},{125});check(clock.snapshot().elapsed.value==25,"rejected update does not advance monotonic cursor");
 ClockState max({hi},{0},{{0}});previous=max.snapshot();
 overflow([&](){max.set_preview_offset({1});},"positive display overflow rejected");check(same(max.snapshot(),previous),"failed positive preview keeps real mode");
 max.set_preview_offset({-1});check(max.snapshot().display_utc.value==hi-1,"upper UTC boundary valid offset");
 previous=max.snapshot();overflow([&](){max.set_preview_offset({1});},"failed replacement preview offset rejected");check(same(max.snapshot(),previous),"failed replacement preserves earlier valid preview offset");max.advance({hi-1},{1});check(max.snapshot().display_utc.value==hi-2,"failed replacement retains prior offset on later advance");
 ClockState min({lo},{0},{{0}});previous=min.snapshot();
 overflow([&](){min.set_preview_offset({-1});},"negative display overflow rejected");check(same(min.snapshot(),previous),"failed negative preview preserves state");
 min.set_preview_offset({1});check(min.snapshot().display_utc.value==lo+1,"lower UTC boundary valid offset");
 ClockState future({0},{0},{{0}});future.set_preview_offset({hi});previous=future.snapshot();
 overflow([&](){future.advance({1},{1});},"future update checks retained offset overflow");check(same(future.snapshot(),previous),"failed advance preserves full preview snapshot");
 future.advance({0},{0});check(future.snapshot().elapsed.value==0,"failed advance does not move monotonic cursor");
 future.reset_preview();future.advance({hi},{1});check(future.snapshot().display_utc.value==hi && !future.snapshot().preview,"reset recovers from future offset overflow");
 ClockState under({0},{0},{{0}});under.set_preview_offset({lo});previous=under.snapshot();overflow([&](){under.advance({-1},{1});},"negative future preview overflow rejected");check(same(under.snapshot(),previous),"negative future overflow leaves state intact");under.reset_preview();under.advance({-1},{1});check(under.snapshot().display_utc.value==-1,"reset recovers negative preview overflow");
 ClockState epoch({-1},{-2},{{0}});snap=epoch.advance({0},{-1});check(snap.actual_utc.value==0 && snap.display_utc.value==0 && snap.synchronization==SyncStatus::Unknown && snap.elapsed.value==1,"POSIX epoch zero is valid UTC, never absent source time");
 ClockState delta({0},{lo},{{0}});snap=delta.advance({0},{-1});
 check(snap.elapsed.value==hi,"largest representable elapsed interval accepted");previous=delta.snapshot();
 overflow([&](){delta.advance({1},{0});},"elapsed subtraction overflow rejected");check(same(delta.snapshot(),previous),"elapsed overflow preserves state");
 ClockState evidence({0},{hi},{{hi}});
 snap=evidence.advance({1},{hi},SyncEvidence{SyncStatus::Synchronized,{lo}});
 check(snap.synchronization==SyncStatus::Unknown,"unrepresentably old evidence Unknown without arithmetic overflow");
 snap=evidence.advance({1},{hi},SyncEvidence{SyncStatus::Unsynchronized,{hi}});
 check(snap.synchronization==SyncStatus::Unsynchronized,"maximum counter evidence fresh");
 ClockState exact({0},{0},{{0}});
 check(exact.advance({0},{1},SyncEvidence{SyncStatus::Synchronized,{0}}).synchronization==SyncStatus::Unknown,"zero-age policy refuses previous instant");
 check(exact.advance({0},{1},SyncEvidence{SyncStatus::Synchronized,{1}}).synchronization==SyncStatus::Synchronized,"zero-age policy accepts exact instant");
 for(std::int64_t offset:{lo,hi}){ClockState full({0},{0},{{0}});full.set_preview_offset({offset});check(full.snapshot().display_utc.value==offset,"full signed offset supported at zero UTC");}
 for(int i=0;i<100;++i){ClockState independent({i},{-1000},{{10}});independent.set_preview_offset({-i});const auto value=independent.advance({i*1000},{i});check(value.elapsed.value==i+1000 && value.actual_utc.value==i*1000 && value.display_utc.value==i*1000-i,"synthetic UTC and preview changes leave elapsed invariant");}
 std::cout<<checks<<" clock-state checks; "<<failures<<" failures\n";return failures?1:0;
}
