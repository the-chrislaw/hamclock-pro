#include "../src/providers/noaa_kp_parser.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
namespace {
    int checks = 0, failures = 0;
    void check(bool ok, const char* label) {
        ++checks;
        if (!ok) {
            ++failures;
            std::cerr<<"FAIL "<<label<<'\n';
        }
    }
    std::string row(std::string kp = "3.33", std::string time = "2026-10-11T15:00:00") {
        return "{\"time_tag\":\""+time+"\",\"Kp\":"+kp+",\"a_running\":18,\"station_count\":8}";
    }
}
int main() {
    using namespace hamclock;
    auto parse = [](const std::string& s) {
        return parse_noaa_kp(s,"Original synthetic NOAA schema fixture");
    };
    auto good = parse("["+row()+"]");
    check(good.error == NoaaKpParseError::None && good.document,"object schema accepted");
    check(good.document && good.document->rows[0].raw_time_tag == "2026-10-11T15:00:00","raw zone-less timestamp retained");
    check(good.document && good.document->rows[0].kp == 3.33 && good.document->rows[0].a_running == 18 && good.document->rows[0].station_count == 8,"three numeric fields preserved");
    for (auto x: {
        "0","9","0.00","3.33e0","null"
    }) {
        auto r = parse("["+row(x)+"]");
        check(r.document !=  nullptr,"numeric boundaries or explicit null accepted");
        if (r.document)check(r.document->rows[0].kp.has_value() == (std::string(x) !=  "null"),"null differs zero");
    }
    for (auto x: {
        "-0.01","9.01","true","false","\"3.33\"","[]","{}","1e999"
    }) {
        auto r = parse("["+row(x)+"]");
        check(r.error !=  NoaaKpParseError::None && !r.document,"invalid Kp rejects complete result");
    }
    for (auto t: {
        "2024-02-29T23:59:59","2000-02-29T00:00:00","0001-01-01T00:00:00","9999-12-31T23:59:59"
    })check(parse("["+row("0", t)+"]").document !=  nullptr,"valid civil boundary");
    for (auto t: {
        "2023-02-29T00:00:00","1900-02-29T00:00:00","0000-01-01T00:00:00","2026-13-01T00:00:00","2026-01-00T00:00:00","2026-04-31T00:00:00","2026-10-11T24:00:00","2026-10-11T23:60:00","2026-10-11T23:59:60","2026-10-11T15:00:00Z","2026-10-11 15:00:00"
    }) {
        auto r = parse("["+row("0", t)+"]");
        check(r.error == NoaaKpParseError::InvalidTimestamp && !r.document,"invalid timestamp rejected without UTC guessing");
    }
    for (auto text: {
        "{}","null","\"x\"","[[\"time_tag\",\"Kp\"],[\"x\",3]]","[null]","[true]","[{}]","["
    }) {
        auto r = parse(text);
        check(r.error !=  NoaaKpParseError::None && !r.document,"wrong schema or malformed rejected");
    }
    check(parse("[]").document && parse("[]").document->rows.empty(),"empty successful array distinct rejection");
    check(parse("").error == NoaaKpParseError::EmptyInput,"empty bytes rejected");
    for (auto label: {
        "", "bad\nlabel"
    })check(parse_noaa_kp("[]", label).error == NoaaKpParseError::InvalidProvenance,"invalid provenance");
    check(parse_noaa_kp("[]", std::string(96,'x')).document !=  nullptr,"maximum label accepted");
    check(parse_noaa_kp("[]", std::string(97,'x')).error == NoaaKpParseError::InvalidProvenance,"one-over label rejected");
    std::string dup = row();
    dup.insert(dup.size()-1,",\"Kp\":4");
    check(parse("["+dup+"]").error == NoaaKpParseError::DuplicateKey,"duplicate required key rejected");
    dup = row();
    dup.insert(dup.size()-1,",\"\\u004bp\":4");
    check(parse("["+dup+"]").error == NoaaKpParseError::DuplicateKey,"escape-equivalent duplicate key rejected");
    dup = row();
    dup.insert(dup.size()-1,",\"other\":{\"x\":0,\"x\":1}");
    check(parse("["+dup+"]").error == NoaaKpParseError::DuplicateKey,"duplicate nested unknown key rejected");
    auto extras = row();
    extras.insert(extras.size()-1,",\"future\":[null,{\"s\":\"brackets [ } : \\\"\"}]");
    auto e = parse("["+extras+"]");
    check(e.document && e.document->rows[0].ignored_field_count == 1,"bounded unknown field ignored safely");
    std::string nulls = "[{\"time_tag\":\"2026-10-11T00:00:00\",\"Kp\":null,\"a_running\":null,\"station_count\":null}]";
    auto n = parse(nulls);
    check(n.document && !n.document->rows[0].kp && !n.document->rows[0].a_running && !n.document->rows[0].station_count,"all null metadata retained");
    for (auto val: {
        "-1","1.5","true","\"8\"","2147483648"
    }) {
        auto s = row();
        auto at = s.find("\"station_count\":8");
        s.replace(at, 17,"\"station_count\":"+std::string(val));
        check(!parse("["+s+"]").document,"bad count rejected");
    }
    check(!parse("["+row()+","+row("false")+"]").document,"late bad row publishes no partial document");
    std::string many = "[";
    for (std::size_t i = 0;i<kNoaaKpMaximumRows;++i) {
        if (i)many+= ',';
        many+= row();
    }
many+= ']';
    check(parse(many).document && parse(many).document->rows.size() == kNoaaKpMaximumRows,"row limit accepted");
    many.insert(many.size()-1,","+row());
    check(parse(many).error == NoaaKpParseError::TooManyRows,"row one-over rejected");
    std::string size = "[]";
    size.append(kNoaaKpMaximumBytes-size.size(),' ');
    check(parse(size).document !=  nullptr,"byte limit accepted");
    size+= ' ';
    check(parse(size).error == NoaaKpParseError::TooManyBytes,"byte one-over rejected before parsing");
    std::string nesting(9,'[');
    nesting+= '0';
    nesting+= std::string(9,']');
    check(parse(nesting).error == NoaaKpParseError::TooDeep,"depth overflow rejected before Qt parsing");
    std::string key(kNoaaKpMaximumKeyBytes+1,'x');
    auto keys = row();
    keys.insert(keys.size()-1,",\""+key+"\":0");
    check(parse("["+keys+"]").error == NoaaKpParseError::InvalidKey,"key length overflow rejected");
    std::string invalid = "["+row()+"]";
    invalid.insert(1, std::string(1, char(0xff)));
    check(!parse(invalid).document,"invalid UTF8 rejected");
    std::string caller = "["+row()+"]";
    auto owned = parse(caller);
    caller.assign(200,'x');
    check(owned.document && owned.document->rows[0].kp == 3.33,"owned result survives caller mutation");
    for (auto value: {
        "1e-9999","1e9999"
    }) {
        auto r = parse("["+row(value)+"]");
        check(r.error == NoaaKpParseError::InvalidNumber && !r.document,"underflow/overflow Kp rejected");
        auto t = row();
        t.insert(t.size()-1,",\"unknown\":"+std::string(value));
        check(parse("["+t+"]").error == NoaaKpParseError::InvalidNumber,"unknown number underflow/overflow rejected");
    }
    for (auto suffix: {
        std::string("\"\\ud800\""), std::string("\"\\udfff\""), std::string("\"\\ud800x\"")
    }) {
        auto t = row();
        t.insert(t.size()-1,",\"s\":"+suffix);
        check(!parse("["+t+"]").document,"unpaired escaped surrogate rejects");
    }
    auto paired = row();
    paired.insert(paired.size()-1,",\"s\":\"\\ud83d\\ude00\"");
    check(parse("["+paired+"]").document !=  nullptr,"paired surrogate accepted unknown string");
    for (auto garbage: {
        std::string("\xef\xbb\xbf", 3)+"[]", std::string("[]\xc2", 3), std::string("[\"\xc0\xaf\"]", 6)
    })check(!parse(garbage).document,"BOM/truncated/overlong UTF8 rejected");
    std::string members = row();
    for (unsigned i = 0;i<28;++i)members.insert(members.size()-1,",\"x"+std::to_string(i)+"\":0");
    check(parse("["+members+"]").document !=  nullptr,"32 member exact limit");
    members.insert(members.size()-1,",\"oneover\":0");
    check(parse("["+members+"]").error == NoaaKpParseError::TooManyMembers,"33 members reject");
    auto exactkey = row();
    exactkey.insert(exactkey.size()-1,",\""+std::string(128,'k')+"\":0");
    check(parse("["+exactkey+"]").document !=  nullptr,"decoded key128 exact limit");
    auto deep = row();
    deep.insert(deep.size()-1,",\"nested\":"+std::string(6,'[')+"null"+std::string(6,']'));
    check(parse("["+deep+"]").document !=  nullptr,"depth8 exact limit");
    deep = row();
    deep.insert(deep.size()-1,",\"nested\":"+std::string(7,'[')+"null"+std::string(7,']'));
    check(parse("["+deep+"]").error == NoaaKpParseError::TooDeep,"depth9 row nested rejects");
    for (auto fractional: {
        "8.0","8e0","8.00000000000000000001"
    }) {
        auto t = row();
        auto pos = t.find("\"station_count\":8");
        t.replace(pos, 17,"\"station_count\":"+std::string(fractional));
        check(parse("["+t+"]").error == NoaaKpParseError::InvalidNumber,"lexical metadata integer requirement");
    }
    for (const auto spelling : {"01", ".1", "1.", "1e", "1e+", "-01", "--1", "+1"}) {
        check(!parse("[" + row(spelling) + "]").document, "strict JSON number grammar required");
        auto unknown = row();
        unknown.insert(unknown.size() - 1, ",\"unknown\":" + std::string(spelling));
        check(!parse("[" + unknown + "]").document, "unknown field number grammar required");
    }
    std::cout<<checks<<" NOAA Kp parser checks, "<<failures<<" failures\n";
    return failures?1:0;
}
