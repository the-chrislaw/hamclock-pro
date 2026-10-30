// App Store metadata draft (docs/appstore/metadata.md): text fits Apple's field limits.
#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <iostream>

namespace {
int failures = 0;
void check(bool ok, const QString& label) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << label.toStdString() << '\n'; }
}

// Text of the blockquote following the line that starts with `header`, starting after line `from`.
QString quoteAfter(const QStringList& lines, const QString& header, int& from) {
    for (int i = from; i < lines.size(); ++i) {
        if (!lines[i].startsWith(header)) continue;
        QStringList text;
        int j = i + 1;
        while (j < lines.size() && !lines[j].startsWith('>')) ++j;
        for (; j < lines.size() && lines[j].startsWith('>'); ++j)
            text << lines[j].mid(lines[j].startsWith("> ") ? 2 : 1);
        from = j;
        return text.join('\n').trimmed();
    }
    from = lines.size();
    return {};
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QFile f(QStringLiteral(HAMCLOCK_APPSTORE_METADATA));
    check(f.open(QIODevice::ReadOnly | QIODevice::Text), "metadata file opens");
    const QStringList lines = QString::fromUtf8(f.readAll()).split('\n');

    // iOS then tvOS sections, in file order.
    for (const char* platform : {"iOS", "tvOS"}) {
        int from = 0;
        for (int i = 0; i < lines.size(); ++i)
            if (lines[i].startsWith(QStringLiteral("## %1 App").arg(platform))) { from = i; break; }
        const QString promo = quoteAfter(lines, "**Promotional text", from);
        const QString desc = quoteAfter(lines, "**Description", from);
        check(!promo.isEmpty() && promo.size() <= 170, QStringLiteral("%1 promotional text <= 170 (%2)").arg(platform).arg(promo.size()));
        check(!desc.isEmpty() && desc.size() <= 4000, QStringLiteral("%1 description <= 4000 (%2)").arg(platform).arg(desc.size()));
    }

    const QString all = lines.join('\n');
    const auto kw = QRegularExpression("\\*\\*Keywords\\*\\*[^\\n]*\\n\\s*\\n`([^`]+)`").match(all);
    check(kw.hasMatch(), "keywords found");
    const QString keywords = kw.captured(1);
    check(keywords.size() <= 100, QStringLiteral("keywords <= 100 (%1)").arg(keywords.size()));
    check(!keywords.contains(", "), "no spaces after commas");
    check(!keywords.toLower().contains("hamclock"), "keywords do not repeat the app name");

    const auto sub = QRegularExpression("\\| Subtitle \\| 30 \\| `([^`]+)`").match(all);
    check(sub.hasMatch() && sub.captured(1).size() <= 30, "subtitle <= 30");
    const auto name = QRegularExpression("\\| Name \\| 30 \\| `([^`]+)`").match(all);
    check(name.hasMatch() && name.captured(1).size() <= 30, "name <= 30");

    if (failures) return 1;
    std::cout << "appstore metadata ok\n";
    return 0;
}
