# Hardware validation log

Real-device runs only. Record board, OS, display, build, results and problems.

## 2026-10-13: Pi 4 Model B (4 GB), generic 7-inch HDMI touchscreen

- **OS:** Raspberry Pi OS (Legacy, 64-bit) Bookworm, desktop (Wayland, labwc/wayfire), fresh image.
- **Display:** reports as "Daewoo HDMI", 1024×600 @ 59.8 Hz (preferred); USB touch.
- **Build:** `hamclock-pro_0.1.0~git20261013.d02d23c_arm64.deb` from Releases → latest.
- **Install:** `sudo apt install ./hamclock-pro_*.deb` succeeded; Qt runtime pulled automatically. `hamclock-pro-cli --self-test` passed.
- **Bug found:** dashboard failed to start: `Could not find the Qt platform plugin "wayland"`. Qt platform plugins are runtime-loaded, so `dpkg-shlibdeps` did not add `qt6-wayland`. Installing `qt6-wayland` fixed it; the package now depends on it.
- **Running:** window opened on the desktop; clock live, DE/DX grids, distance/bearing, great-circle path, night shading and subsolar point correct for 15:02 UTC (subsolar over Brazil, night over Asia/Pacific).
- **Resources (idle, steady state):** 0.6 % CPU (15 s average), 84 MB RSS, SoC 44.8 °C, `get_throttled=0x0`, load average 0.5.
- **Layout issues:**
  - Window opens at its default size instead of filling the 1024×600 screen; the desktop panel and wallpaper remain visible. Needs a fullscreen/kiosk start (roadmap M3).
  - Map is letterboxed inside its card with large unused margins at this size; it should use the available width.
- **Not yet checked:** touch interaction, Big Clock/Stopwatch/Countdown dialogs on device, TV over HDMI, kiosk mode without desktop, Pi 3 / Zero 2 W.

Screenshot taken with `grim` over SSH (kept out of the repo).

## 2026-10-13 (later): Qt Quick app on the same Pi 4 + 7-inch 1024×600

- **Build:** `hamclock-pro_0.1.0~git20261013.937eb49_bookworm_arm64.deb` (first release with the Qt Quick app). `apt install` pulled the QML modules automatically.
- **Running:** `hamclock-pro` started fullscreen on the Wayland desktop, followed the system dark theme, and showed **live** NOAA data (Space weather, Solar wind, Solar trend) plus Sun & Moon and the station card. Steady state **0.8 % CPU** (20 s), **141 MB RSS**, SoC 52.5 °C.
- **Bugs found and fixed in the follow-up PR:**
  - Same-day `.deb` builds were treated as downgrades (version used date + commit hash; hashes sort arbitrarily). Version now includes the time.
  - Default settings folder was `~/.config/hamclock-pro/hamclock-pro/` (organization and app name both set), so an existing station file was not found and the app opened in Settings. Now `~/.config/hamclock-pro/`.
- **Layout issues on the real screen (area A):** large gap under Space weather (cards of different heights in one row); Kp bars clipped at the bottom; world map below the fold at 1024×600, reachable only by scrolling.
- **Not yet checked:** touch scrolling feel, Settings by touch, TV.
