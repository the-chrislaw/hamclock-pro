#pragma once
// Public DX cluster nodes offered as one-tap choices in Settings. The app never
// connects to one on its own: the nodes' banners say they are for licensed amateurs
// using their real callsign, so the user picks one after setting a callsign.
// Each was checked on 2026-10-29 to answer with a login prompt (no login was sent).
#include <array>

namespace hamclock::dxcluster {

struct ClusterNode {
    const char* label;  // shown on the button
    const char* host;
    int port;
    const char* note;   // software and area, shown under the buttons
};

inline constexpr std::array<ClusterNode, 4> kSuggestedNodes{{
    {"NC7J", "dxc.nc7j.com", 7373, "AR-Cluster, USA west"},
    {"K1TTT", "k1ttt.net", 7373, "AR-Cluster, USA north-east"},
    {"W3LPL", "w3lpl.net", 7373, "DX Spider, USA east"},
    {"DX Spider UK", "dxspider.co.uk", 7300, "DX Spider, UK and Europe"},
}};

} // namespace hamclock::dxcluster
