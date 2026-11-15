# Installing on a Raspberry Pi

The flashable image is built by `.github/workflows/image.yml` (pi-gen, Raspberry Pi OS Lite arm64,
Trixie). **Not yet validated on a device**: build it from the Actions tab (manual run) and report
what happens on your Pi.

1. Download the `image_<date>-hamclock-pro.img.xz` image (about 700 MB) and `SHA256SUMS-image` from the release; check with
   `sha256sum -c SHA256SUMS-image`.
2. In Raspberry Pi Imager choose *Use custom* and the `.img.xz`. In the settings dialog set Wi-Fi
   (and, only if you want it, enable SSH with a key or password you choose; SSH is off by default).
3. Boot. HamClock Pro starts full screen in a kiosk session (cage). With no saved station it opens
   Settings first: enter your callsign, use *Find location* or a grid, and pick a time zone.
4. There are no default credentials: the `hamclock` user's password is locked.
5. Automatic updates are off by default. To opt in run `sudo systemctl enable --now hamclock-pro-update.timer`;
   it checks hourly for the newest *stable* release (never the rolling pre-release) and installs its `.deb` only
   if the release's `SHA256SUMS` carries a valid signature from the release key baked into the image, the `.deb`
   matches its signed checksum, and it is the `hamclock-pro` package for this architecture with the tag's version
   and newer than what is installed. If the install fails, or the kiosk does not come back up, the previous
   package is reinstalled. Logs: `journalctl -u hamclock-pro-update`.
   **Until the owner creates the signing key (below), the updater refuses every update.** Update by hand meanwhile:
   download the `.deb` and `SHA256SUMS`, check with `sha256sum -c --ignore-missing SHA256SUMS`, then `sudo apt install ./hamclock-pro_*.deb`.

Files: `packaging/pi/` (pi-gen config, stage, kiosk and updater units).

## Signed releases (owner, one time)

The updater verifies an Ed25519 signature (OpenSSL, `SHA256SUMS.sig` over `SHA256SUMS`) against
`/usr/local/share/hamclock-pro-update/release-signing.pub`, which the image copies from
`packaging/pi/stage-hamclock/00-install/files/release-signing.pub`. That file is a placeholder now, so the
updater fails closed. To turn signing on, on a trusted machine with OpenSSL 3 (`brew install openssl@3`):

1. `openssl genpkey -algorithm ed25519 -out release-signing.key`
2. `openssl pkey -in release-signing.key -pubout -out packaging/pi/stage-hamclock/00-install/files/release-signing.pub`
3. `gh secret set RELEASE_SIGNING_KEY --repo the-chrislaw/hamclock-pro < release-signing.key`
4. Commit the public key (`git add packaging/pi/stage-hamclock/00-install/files/release-signing.pub`) and open a PR.
5. Store `release-signing.key` offline (password manager or encrypted backup), then delete the local copy.
   If it is lost or leaks, make a new key with these steps and re-flash every Pi, since the old key is baked in.

From then on `release.yml` signs every release (and fails if the secret does not match the committed key), and
`image.yml` checks the signature of the `.deb` it bakes in. Releases published before step 3, and images built
before step 4, stay unverifiable: re-flash Pis from an image built after step 4.
