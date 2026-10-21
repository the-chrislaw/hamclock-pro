# Installing on a Raspberry Pi

The flashable image is built by `.github/workflows/image.yml` (pi-gen, Raspberry Pi OS Lite arm64,
Bookworm). **Not yet validated on a device**: build it from the Actions tab (manual run) and report
what happens on your Pi.

1. Download `hamclock-pro-*.img.xz` and `SHA256SUMS`; check with `sha256sum -c SHA256SUMS`.
2. In Raspberry Pi Imager choose *Use custom* and the `.img.xz`. In the settings dialog set Wi-Fi
   (and, only if you want it, enable SSH with a key or password you choose; SSH is off by default).
3. Boot. HamClock Pro starts full screen in a kiosk session (cage). With no saved station it opens
   Settings first: enter your callsign, use *Find location* or a grid, and pick a time zone.
4. There are no default credentials: the `hamclock` user's password is locked.
5. An hourly timer (`hamclock-pro-update.timer`) installs the newest release `.deb` from GitHub
   Releases; run `sudo systemctl disable --now hamclock-pro-update.timer` to stop it.

Files: `packaging/pi/` (pi-gen config, stage, kiosk and updater units).
