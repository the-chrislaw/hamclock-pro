# Find-in-the-sky: manual device check (#573 item 7)

CI cannot prove compass, motion or camera accuracy. Run this on a real iPhone or iPad from TestFlight. Nothing here has been validated on a device yet.

## Setup
1. Settings > Station: set your location (or tap "Use my location" and allow it when asked).
2. Let the app run for a minute so planets and satellites load.
3. Go outside, away from steel and magnets. Remove a magnetic case if you have one.

## Heading and tilt
1. Open the finder. If it shows a calibration hint, move the device in a figure of eight until it clears.
2. Point the top edge at a known landmark. Compare the shown heading with a phone compass or a map; expect within about 5 degrees once calibrated.
3. Tilt the device to the horizon, then to straight up. Elevation should read about 0 and about 90.

## Point at a known object
1. Open the Planets pane and pick a planet or the Moon that is above the horizon now. Note its azimuth and elevation.
2. Follow the arrow guidance until the haptic tick fires (within about 3 degrees).
3. Look along the device's top edge. The object should be close to the line (a few degrees, not tens).
4. Record: device model, iOS version, object, shown az/el, estimated offset in degrees.

## Pass reminder
1. Star a pass that starts at least 15 minutes from now (once the pass list supports starring).
2. Confirm the notification permission prompt appears only at that point.
3. Confirm a notification arrives about the chosen lead time before AOS.

## Camera mode (only builds with HAMCLOCK_SKY_FINDER)
Camera is off by default. If enabled, check the permission prompt appears only when you turn it on, and that the marker lines up with the object within the field of view.

Report results on #573.
