---
title: HamClock Pro privacy policy
permalink: /privacy/
---

# HamClock Pro privacy policy

_Last updated: 27 October 2026_

HamClock Pro is a free, open-source amateur-radio dashboard for Raspberry Pi, macOS, iPhone, iPad and Apple TV. It is developed independently by Christopher Law (KM7IBM). This policy covers all versions of the app.

## The short version

- **HamClock Pro does not collect, sell or share your personal data.** There are no accounts with us, no analytics, no advertising and no tracking.
- Everything you enter (callsign, location, settings) **stays on your device**.
- The app fetches public data (space weather, satellites, spots and more) **directly from each data provider**. We run no server of our own, so nothing passes through us.

## What stays on your device

- **Station settings:** callsign, home and DX location, time zone, display choices. Stored only on your device (and, on Apple TV, synced between your own devices through your iCloud account if you enable it).
- **Account details for optional services** (for example QRZ.com, HamQTH, HamAlert or hams.at): stored in the system's secure storage (Keychain on Apple devices, a file readable only by you on Raspberry Pi) and sent **only** to the service they belong to.
- **Location:** if you use "Use my location" or the satellite finder, your position is read from the device and used on the device only, for calculations such as pass times and pointing directions. It is never sent to us.
- **Camera and motion sensors:** used only while the satellite finder is open, on the device, to show where a satellite is. Nothing is recorded or uploaded.

## Who the app talks to, and what they see

To show live information, the app connects directly to public data providers. Like any internet request, each provider sees your device's IP address and the app's name and version (in the "User-Agent"). The providers include, depending on which panes you use:

- NOAA Space Weather Prediction Center, NOAA National Hurricane Center, the US National Weather Service, USGS (earthquakes), NASA (solar images, fire and event data)
- Open-Meteo (weather), OpenStreetMap Nominatim (only when you press **Find** to look up an address)
- CelesTrak and SatNOGS (satellite orbits and frequencies), Launch Library (launches), SondeHub (weather balloons)
- Amateur-radio services: PSKReporter, the Reverse Beacon Network, POTA, SOTA, WWFF, hams.at, HamQSL, the DX cluster node you choose, and the callsign lookup service you choose

Some of these connections, by their nature, include your callsign: for example, logging in to a DX cluster or the Reverse Beacon Network, or asking PSKReporter who has heard you. These are features you turn on, and the data goes only to that service.

Each provider has its own privacy policy. HamClock Pro sends them only what is needed for the feature you are using.

## Notifications

Satellite pass reminders are scheduled **on your device**. No notification server is involved.

## Children

HamClock Pro is suitable for all ages and does not knowingly collect any information from anyone.

## Changes

If this policy changes, the new version will be published at this address with a new date.

## Contact

Questions about privacy: open an issue at <https://github.com/the-chrislaw/hamclock-pro/issues> (please don't post private information there).
