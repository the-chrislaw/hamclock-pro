# Kiosk mode (Raspberry Pi OS Lite, no desktop)

Not yet validated on a device: these steps follow Qt's documented eglfs
platform and have only been checked for syntax in CI.

1. Create the user: `sudo adduser --disabled-password hamclock` and add it to
   the groups `video`, `input` and `render`.
2. Install the unit: `sudo cp hamclock-pro-kiosk.service /etc/systemd/system/`,
   then `sudo systemctl enable --now hamclock-pro-kiosk`.
3. The unit sets `QT_QPA_PLATFORM=eglfs` and starts `hamclock-pro --fullscreen`.
   Small screens (up to 1280x800) start fullscreen on their own; use
   `--windowed` to override. If eglfs fails on your display, try
   `QT_QPA_PLATFORM=linuxfb` (slower, no GPU).
4. Wrong screen size or rotation: set `QT_QPA_EGLFS_KMS_CONFIG` to a JSON file
   (see the Qt eglfs documentation) or use `video=` options in `cmdline.txt`.
5. Touch calibration on generic panels: set `LIBINPUT_CALIBRATION_MATRIX`
   through a udev rule; the app has no calibration screen.
6. TV overscan: use Settings > TV overscan rather than firmware options.
7. Logs: `journalctl -u hamclock-pro-kiosk`. Stop with
   `sudo systemctl stop hamclock-pro-kiosk`.

On-screen keyboard: decision pending (Qt Virtual Keyboard needs a separate
module); until then, Settings text fields need a physical keyboard in kiosk mode.
