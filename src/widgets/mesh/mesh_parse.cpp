#include "mesh_parse.hpp"
#include <cmath>
#include <cstring>

namespace hamclock::mesh {

namespace {

struct Reader {
    const uchar* p;
    const uchar* end;
    bool ok = true;
    bool done() const { return p >= end || !ok; }
    quint64 varint() {
        quint64 v = 0;
        for (int shift = 0; shift < 64; shift += 7) {
            if (p >= end) { ok = false; return 0; }
            const uchar b = *p++;
            v |= quint64(b & 0x7f) << shift;
            if (!(b & 0x80)) return v;
        }
        ok = false;
        return 0;
    }
    quint32 fixed32() {
        if (end - p < 4) { ok = false; return 0; }
        const quint32 v = quint32(p[0]) | quint32(p[1]) << 8 | quint32(p[2]) << 16 | quint32(p[3]) << 24;
        p += 4;
        return v;
    }
    QByteArray bytes() {
        const quint64 n = varint();
        if (!ok || n > quint64(end - p)) { ok = false; return {}; }
        QByteArray out(reinterpret_cast<const char*>(p), int(n));
        p += n;
        return out;
    }
    // Skips a field of the given wire type (value not wanted).
    void skip(int wire) {
        switch (wire) {
        case 0: varint(); break;
        case 1: if (end - p < 8) ok = false; else p += 8; break;
        case 2: bytes(); break;
        case 5: fixed32(); break;
        default: ok = false;
        }
    }
};

Reader reader(const QByteArray& b) {
    const auto* d = reinterpret_cast<const uchar*>(b.constData());
    return {d, d + b.size()};
}

} // namespace

QByteArray want_config_frame(quint32 config_id) {
    QByteArray body;
    body.append(char(0x18)); // ToRadio.want_config_id = field 3, varint
    quint32 v = config_id;
    do {
        uchar b = v & 0x7f;
        v >>= 7;
        if (v) b |= 0x80;
        body.append(char(b));
    } while (v);
    QByteArray out;
    out.append(char(0x94)).append(char(0xc3)).append(char(body.size() >> 8)).append(char(body.size() & 0xff));
    return out + body;
}

std::vector<QByteArray> extract_frames(QByteArray& buffer) {
    std::vector<QByteArray> out;
    for (;;) {
        const int start = buffer.indexOf(char(0x94));
        if (start < 0) { buffer.clear(); break; }
        buffer.remove(0, start);
        if (buffer.size() < 4) break;
        if (uchar(buffer[1]) != 0xc3) { buffer.remove(0, 1); continue; }
        const int len = (uchar(buffer[2]) << 8) | uchar(buffer[3]);
        if (len > kMaxFrame) { buffer.remove(0, 1); continue; }
        if (buffer.size() < 4 + len) break;
        out.push_back(buffer.mid(4, len));
        buffer.remove(0, 4 + len);
    }
    return out;
}

std::optional<PositionUpdate> parse_position_packet(const QByteArray& payload) {
    Reader r = reader(payload);
    while (!r.done()) {
        const quint64 tag = r.varint();
        if (!r.ok) break;
        if ((tag >> 3) != 2 || (tag & 7) != 2) { r.skip(int(tag & 7)); continue; } // FromRadio.packet
        const QByteArray packet = r.bytes();
        if (!r.ok) return std::nullopt;
        PositionUpdate u;
        QByteArray data;
        Reader pk = reader(packet);
        while (!pk.done()) {
            const quint64 t = pk.varint();
            if (!pk.ok) break;
            const quint64 field = t >> 3;
            const int wire = int(t & 7);
            if (field == 1 && wire == 0) u.num = quint32(pk.varint());           // MeshPacket.from
            else if (field == 4 && wire == 2) data = pk.bytes();                 // MeshPacket.decoded
            else if (field == 7 && wire == 5) u.rx_time = pk.fixed32();          // MeshPacket.rx_time
            else pk.skip(wire);
        }
        if (!pk.ok || u.num == 0 || data.isEmpty()) return std::nullopt;
        quint64 port = 0;
        QByteArray body;
        Reader d = reader(data);
        while (!d.done()) {
            const quint64 t = d.varint();
            if (!d.ok) break;
            if ((t >> 3) == 1 && (t & 7) == 0) port = d.varint();                // Data.portnum
            else if ((t >> 3) == 2 && (t & 7) == 2) body = d.bytes();            // Data.payload
            else d.skip(int(t & 7));
        }
        if (!d.ok || port != 3) return std::nullopt;                             // POSITION_APP
        qint32 lat_i = 0, lon_i = 0;
        bool has = false;
        Reader pos = reader(body);
        while (!pos.done()) {
            const quint64 pt = pos.varint();
            if (!pos.ok) break;
            if ((pt & 7) == 5 && (pt >> 3) == 1) { lat_i = qint32(pos.fixed32()); has = true; }
            else if ((pt & 7) == 5 && (pt >> 3) == 2) { lon_i = qint32(pos.fixed32()); has = true; }
            else pos.skip(int(pt & 7));
        }
        if (!pos.ok || !has || (lat_i == 0 && lon_i == 0)) return std::nullopt;
        u.latitude = lat_i * 1e-7;
        u.longitude = lon_i * 1e-7;
        if (std::fabs(u.latitude) > 90 || std::fabs(u.longitude) > 180) return std::nullopt;
        return u;
    }
    return std::nullopt;
}

std::optional<NameUpdate> parse_nodeinfo_packet(const QByteArray& payload) {
    Reader r = reader(payload);
    while (!r.done()) {
        const quint64 tag = r.varint();
        if (!r.ok) break;
        if ((tag >> 3) != 2 || (tag & 7) != 2) { r.skip(int(tag & 7)); continue; } // FromRadio.packet
        const QByteArray packet = r.bytes();
        if (!r.ok) return std::nullopt;
        NameUpdate u;
        QByteArray data;
        Reader pk = reader(packet);
        while (!pk.done()) {
            const quint64 t = pk.varint();
            if (!pk.ok) break;
            if ((t >> 3) == 1 && (t & 7) == 0) u.num = quint32(pk.varint());     // MeshPacket.from
            else if ((t >> 3) == 4 && (t & 7) == 2) data = pk.bytes();           // MeshPacket.decoded
            else pk.skip(int(t & 7));
        }
        if (!pk.ok || u.num == 0 || data.isEmpty()) return std::nullopt;
        quint64 port = 0;
        QByteArray body;
        Reader d = reader(data);
        while (!d.done()) {
            const quint64 t = d.varint();
            if (!d.ok) break;
            if ((t >> 3) == 1 && (t & 7) == 0) port = d.varint();                // Data.portnum
            else if ((t >> 3) == 2 && (t & 7) == 2) body = d.bytes();            // Data.payload
            else d.skip(int(t & 7));
        }
        if (!d.ok || port != 4) return std::nullopt;                             // NODEINFO_APP
        Reader ur = reader(body);
        while (!ur.done()) {
            const quint64 t = ur.varint();
            if (!ur.ok) break;
            const int w = int(t & 7);
            if (w == 2 && (t >> 3) >= 1 && (t >> 3) <= 3) {
                const QString s = QString::fromUtf8(ur.bytes()).left(40);
                if ((t >> 3) == 1) u.id = s; else if ((t >> 3) == 2) u.long_name = s; else u.short_name = s;
            } else ur.skip(w);
        }
        if (!ur.ok || (u.long_name.isEmpty() && u.short_name.isEmpty())) return std::nullopt;
        return u;
    }
    return std::nullopt;
}

std::optional<Node> parse_from_radio(const QByteArray& payload) {
    Reader r = reader(payload);
    while (!r.done()) {
        const quint64 tag = r.varint();
        if (!r.ok) break;
        if ((tag >> 3) != 4 || (tag & 7) != 2) { r.skip(int(tag & 7)); continue; } // FromRadio.node_info
        const QByteArray info = r.bytes();
        if (!r.ok) return std::nullopt;
        Node n;
        bool has_pos = false;
        qint32 lat_i = 0, lon_i = 0;
        Reader ni = reader(info);
        while (!ni.done()) {
            const quint64 t = ni.varint();
            if (!ni.ok) break;
            const quint64 field = t >> 3;
            const int wire = int(t & 7);
            if (field == 1 && wire == 0) n.num = quint32(ni.varint());
            else if (field == 5 && wire == 5) n.last_heard = ni.fixed32();
            else if (field == 2 && wire == 2) {
                const QByteArray user = ni.bytes();
                Reader u = reader(user);
                while (!u.done()) {
                    const quint64 ut = u.varint();
                    if (!u.ok) break;
                    const int uw = int(ut & 7);
                    if (uw == 2 && (ut >> 3) >= 1 && (ut >> 3) <= 3) {
                        const QString s = QString::fromUtf8(u.bytes()).left(40);
                        if ((ut >> 3) == 1) n.id = s; else if ((ut >> 3) == 2) n.long_name = s; else n.short_name = s;
                    } else u.skip(uw);
                }
            } else if (field == 3 && wire == 2) {
                const QByteArray position = ni.bytes();
                Reader pos = reader(position);
                while (!pos.done()) {
                    const quint64 pt = pos.varint();
                    if (!pos.ok) break;
                    if ((pt & 7) == 5 && (pt >> 3) == 1) { lat_i = qint32(pos.fixed32()); has_pos = true; }
                    else if ((pt & 7) == 5 && (pt >> 3) == 2) { lon_i = qint32(pos.fixed32()); has_pos = true; }
                    else pos.skip(int(pt & 7));
                }
            } else ni.skip(wire);
        }
        if (!ni.ok || !has_pos || (lat_i == 0 && lon_i == 0)) return std::nullopt;
        n.latitude = lat_i * 1e-7;
        n.longitude = lon_i * 1e-7;
        if (std::fabs(n.latitude) > 90 || std::fabs(n.longitude) > 180 || n.num == 0) return std::nullopt;
        if (n.id.isEmpty()) n.id = QString("!%1").arg(n.num, 8, 16, QLatin1Char('0'));
        return n;
    }
    return std::nullopt;
}

} // namespace hamclock::mesh
