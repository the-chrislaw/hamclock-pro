// DXCC prefix table: parser on a tiny synthetic file and lookups against the bundled AD1C cty.dat, offline.
#include "../dxcc_table.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <iostream>

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    using hamclock::dxcluster::DxccTable;

    DxccTable small;
    const QString text =
        "Sample Land:   14:  27:  EU:   50.00:   -10.00:    -1.0:  S:\r\n"
        "    S,SA(15)[30],=SX1ABC<1.5/2.5>{AS}~2.0~;\r\n"
        "Broken:   x:\r\n"
        "    B;\r\n"
        "Truncated:   1:  2:  NA:   1.0:   2.0:    3.0:  T:\r\n"
        "    T,TT";
    check(small.parse(text) == 1, "one complete entity, broken and truncated dropped");
    const auto* e = small.lookup("sa7xyz");
    check(e && e->name == "Sample Land" && e->cq_zone == 14 && e->itu_zone == 27 && e->continent == "EU", "fields");
    check(e && e->lon == 10.0 && e->lat == 50.0, "west-positive longitude flipped to east-positive");
    check(small.lookup("SX1ABC") && small.lookup("T1AA") == nullptr, "exact alias; truncated entity not kept");
    check(small.parse("garbage") == 0 && small.lookup("S1AA") == nullptr, "unusable input leaves an empty table");

    // Bundled file, loaded from the repository copy.
    const QString path = QDir(QString::fromLocal8Bit(argc > 1 ? argv[1] : ".")).filePath("../../data/cty.dat");
    QFile file(path);
    check(file.open(QIODevice::ReadOnly), "cty.dat readable");
    DxccTable t;
    check(t.parse(QString::fromLatin1(file.readAll())) > 300, "more than 300 entities");
    auto name = [&](const char* call) { const auto* x = t.lookup(call); return x ? x->name : QString(); };
    check(name("K1ABC") == "United States", "K1ABC");
    check(name("W7DX") == "United States", "W7DX");
    check(name("G4ABC") == "England", "G4ABC");
    check(name("DL1ABC") == "Fed. Rep. of Germany", "DL1ABC");
    check(name("JA1ABC") == "Japan", "JA1ABC");
    check(name("VK2ABC") == "Australia", "VK2ABC");
    check(name("3Y/LB5SH") == "Bouvet" || name("LB5SH") != "Bouvet", "exact call alias does not leak to prefix");
    check(name("EA8/K1ABC") == "Canary Islands", "prefix part wins: EA8/K1ABC");
    check(name("K1ABC/EA8") == "Canary Islands", "prefix part wins: K1ABC/EA8");
    check(name("DL1ABC/P") == "Fed. Rep. of Germany", "/P suffix ignored");
    check(name("DL1ABC/3") == "Fed. Rep. of Germany", "/digit suffix ignored");
    check(name("K1ABC/MM").isEmpty() && name("K1ABC/AM").isEmpty(), "maritime/aeronautical mobile has no entity");
    check(name("").isEmpty() && name("//").isEmpty() && name("QQ1QQ").isEmpty(), "junk and unknown calls");
    check(name("1/2/3/4").isEmpty(), "too many parts");
    const auto* us = t.lookup("K1ABC");
    check(us && us->continent == "NA" && us->lon < -60 && us->lon > -130 && us->lat > 30, "US centre is in North America");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
