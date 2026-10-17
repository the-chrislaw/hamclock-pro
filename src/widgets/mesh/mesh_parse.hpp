#pragma once
// Meshtastic client API over TCP (a node's port 4403): frames are 0x94 0xC3 <len16 big-endian> <protobuf>.
// Only the small subset needed to list nodes with positions is decoded (FromRadio.node_info); every other
// message and unknown field is skipped. The only thing ever written is ToRadio.want_config_id, which asks the
// node to send its node database; nothing is transmitted over the mesh.
#include <QByteArray>
#include <QString>
#include <optional>
#include <vector>

namespace hamclock::mesh {

struct Node {
    quint32 num = 0;
    QString id;         // "!a1b2c3d4"
    QString long_name, short_name;
    double latitude = 0, longitude = 0;
    qint64 last_heard = 0; // unix seconds, 0 = never
};

inline constexpr int kMaxFrame = 512;

QByteArray want_config_frame(quint32 config_id);
// Removes complete frames from `buffer` and returns their payloads; resynchronises past garbage.
std::vector<QByteArray> extract_frames(QByteArray& buffer);
// A node with a valid position; anything else (no position, out of range, other message types) is nullopt.
std::optional<Node> parse_from_radio(const QByteArray& payload);

} // namespace hamclock::mesh
