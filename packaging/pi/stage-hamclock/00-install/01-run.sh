#!/bin/bash -e
# Runs on the build host; files/ holds the release .deb (hamclock-pro_*.deb) and the units below.
install -m 0755 files/hamclock-pro-update "${ROOTFS_DIR}/usr/local/bin/hamclock-pro-update"
install -m 0755 files/hamclock-kiosk "${ROOTFS_DIR}/usr/local/bin/hamclock-kiosk"
install -m 0644 files/hamclock-kiosk.service "${ROOTFS_DIR}/etc/systemd/system/hamclock-kiosk.service"
install -m 0644 files/hamclock-pro-update.service "${ROOTFS_DIR}/etc/systemd/system/hamclock-pro-update.service"
install -m 0644 files/hamclock-pro-update.timer "${ROOTFS_DIR}/etc/systemd/system/hamclock-pro-update.timer"
install -m 0644 files/hamclock-pro*.deb "${ROOTFS_DIR}/tmp/"
on_chroot << EOF
apt-get install -y /tmp/hamclock-pro*.deb
rm -f /tmp/hamclock-pro*.deb
usermod -aG video,render,input,audio hamclock
passwd -l hamclock
systemctl enable hamclock-kiosk.service hamclock-pro-update.timer
systemctl set-default graphical.target
EOF
