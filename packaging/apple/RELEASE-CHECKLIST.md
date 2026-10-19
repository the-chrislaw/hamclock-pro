# Apple release checklist (manual checks)

## iPad hardware keyboard (#513)
CI cannot type on a device, so before a TestFlight build is promoted:
1. On an iPad with a Magic Keyboard or Smart Keyboard: open Settings, tap Callsign, type letters and digits. They must appear.
2. Disconnect and reconnect the keyboard; typing still works.
3. If keys do not appear, do not ship: file the Qt bug (QIOSTextInputResponder / `pressesBegan`) and note the Qt version.

## iPad app on Apple Silicon Macs (owner action in App Store Connect)
The iPad build ignores the hardware keyboard when run on a Mac, and Mac users get the native macOS app instead. Once the macOS app is distributed:
1. App Store Connect > the app > Pricing and Availability.
2. Under "iPhone and iPad Apps on Apple Silicon Macs", untick availability.
3. Until then the app shows a notice under text fields when it detects it is running on a Mac (`NSProcessInfo.isiOSAppOnMac`).
