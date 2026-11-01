// HamQSL port; mirrors src/widgets/solarhamqsl/tests (the oracle) on the same fixture, plus the
// minimal XML reader it runs on.
#include "../hamqsl.hpp"
#include "../hcpcore.h"
#include "../xml_mini.hpp"
#include "test_util.hpp"
#include <string>

using namespace hcptest;

int main() {
    using namespace hcp;
    const auto body = fixture("solarxml.xml");
    check(!body.empty(), "fixture present");

    const auto r = hamqsl::parse(body);
    check(r.updated_ms == 1791965460000LL, "updated time 2026-10-14 08:11Z");
    check(r.hf.size() == 4 && r.hf[0].name == "80m-40m" && r.hf[0].day == "Poor" && r.hf[0].night == "Fair",
          "first HF band");
    check(r.hf.size() == 4 && r.hf[3].name == "12m-10m" && r.hf[3].day == "Fair" && r.hf[3].night == "Poor",
          "last HF band");
    check(r.vhf.size() == 5 && r.vhf[0].name == "vhf-aurora" && r.vhf[0].status == "Band Closed", "VHF items");

    check(hamqsl::parse("").hf.empty() && hamqsl::parse("<html>").hf.empty() &&
              hamqsl::parse("<solar><calculatedconditions><band name=\"a\" time=\"day\">Good</band>").hf.empty(),
          "garbage and truncated documents rejected");
    const auto half = hamqsl::parse("<solar><solardata><updated> 1 Jan 2026 0000 GMT</updated><calculatedconditions>"
                                    "<band name=\"20m\" time=\"day\">Good</band></calculatedconditions></solardata></solar>");
    check(half.hf.empty(), "band with only a day value dropped");
    check(hamqsl::condition_rank("Good") == 3 && hamqsl::condition_rank("Fair") == 2 && hamqsl::condition_rank("Poor") == 1 &&
              hamqsl::condition_rank("?") == 0,
          "ranks");
    check(hamqsl::parse_updated(" 1 Jan 2026 0000 GMT") == 1767225600000LL && hamqsl::parse_updated("14 Oct 2026 0811") == 0 &&
              hamqsl::parse_updated("14 Foo 2026 0811 GMT") == 0,
          "updated format");

    // The XML reader on its own.
    const auto doc = xml::parse("<?xml version=\"1.0\"?><!-- c --><a x='1' y=\"&lt;2&#65;\"><b/>t&amp;u<![CDATA[<v>]]></a>");
    check(doc && doc->name == "a" && doc->attribute("y") == "<2A" && doc->children.size() == 1 && doc->text == "t&u<v>",
          "XML: declaration, comment, attributes, entities, CDATA");
    for (const char* bad : {"", "<a>", "<a></b>", "<a></a><b/>", "<a>x</a>y", "<a x=1/>", "<a x='1' x='2'/>", "<a>&nope;</a>",
                            "<a><</a>", "<a x='1'y='2'/>"})
        check(!xml::parse(bad), "XML: malformed rejected");

    HcpHamqsl abi;
    check(hcp_hamqsl_parse(body.data(), body.size(), &abi) == 1 && abi.hf_count == 4 && abi.vhf_count == 5 &&
              std::string(abi.hf[1].name) == "30m-20m" && abi.hf[0].day_rank == 1 && abi.hf[0].night_rank == 2 &&
              abi.updated_ms == r.updated_ms,
          "C ABI");
    check(hcp_hamqsl_parse("<html>", 6, &abi) == 0 && abi.hf_count == 0, "C ABI rejects garbage");
    return finish("hamqsl");
}
