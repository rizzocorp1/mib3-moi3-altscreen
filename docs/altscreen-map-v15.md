# MU0841 v15: top-aligned maneuver request and inset safe area

V15 requests a TOP-aligned maneuver card for the auxiliary cluster display.
This is one display-wide preference, not separate Apple Maps/Google Maps/Waze
settings. Each app can interpret it differently; on-car support is unvalidated.
The URI convention is documented by the MHI2Q-CarPlay-AltScreen implementation:
https://github.com/yuedizhibo/MHI2Q-CarPlay-AltScreen/blob/main/README_EN.md
This package is an independent implementation for MOI3, not its QNX installer.

Video canvas remains1280x480. The provisional safe rectangle is x160,y64,
width960,height320 (right1120,bottom384). Important content should stay within
that region instead of clipping behind the cluster overlays. These coordinates
are an initial calibration, not a promise that all app elements will align.
The build-time ALT_SAFE_X/Y/W/H constants in layout-v15.h are configurable;
they are not arbitrary live commands or per-app switches. No misleading
boolean transition flags are added. Main-display geometry/input settings are
preserved; its complete full-screen view/safe rectangle is advertised too.

The top request is sent once per authenticated auxiliary session using a
UUID-scoped showUI command on the asynchronous session queue. Submission
success is not phone acceptance or proof of positioning. Changes remain
one-shot with normal reboot restoration and an approximately ten-minute lease.

This release does NOT fix two-second updates or colored artifacts. The v14
timing/reference diagnostics, two-second reference-picture fallback, packet
observer and tested ownership/restoration are retained. One short PRIVATE map-video capture is included for offline decoding: at most
three seconds,90 access units or2 MiB, whichever comes first. It begins at a
substantial authenticated IDR after output handoff, never contains pairing
keys/audio, and is copied to map-v15-video.bin on the USB only when complete.
After a checksum-verified USB copy the capture is removed from the unit.
It may show your location/route: keep it private and never upload it to GitHub.
Do not mistake layout improvements for a video-decoder fix.

## Parked test: begin with Apple Maps

1. Use stable update power. Keep the iPhone disconnected and this USB connected.
2. Select ONLY MIB3OI_POST_SCRIPT at E9400L/Y940. Stop before starting if other
   components or verification errors appear. Complete the update and Resume
   into normal mode with USB connected. Never interrupt an active update.
3. Confirm the factory VW map first. Connect WIRED CarPlay through the other
   data port. Begin with APPLE MAPS and start a route while parked.
4. Check whether the maneuver card is near the top center, fully visible, and
   whether the position marker/road labels clear the bottom overlay. A photo
   of the full cluster and main screen together is useful; keep it private.
5. Try Waze and Google Maps separately with a route. Note each card position,
   clipped elements, map visibility and artifacts. Different app layouts are
   possible. There is no need to wait four minutes in every app.
6. Disconnect CarPlay completely and verify the factory map returns; unplugging
   wired CarPlay can leave wireless CarPlay active. Reconnect once if desired.
   If negotiation fails, the screen is unusable or factory restoration fails,
   end with a normal reboot after the updater is complete. Do not reinstall
   repeatedly. Keep the USB connected for report export, then return it.
7. At approximately ten minutes the lease expires and attempts native handback.
   Finish testing with normal reboot and confirm stock functions.

No firewall, SMACK, coding or persistent factory-executable changes. Startup
restores before RAM-only activation. The signed trigger retains the four
previously accepted speech-data resets. Recovery is narrow, not a full vehicle
backup or reflash. Explicit startup recovery: add empty RESTORE_FACTORY_STARTUP.txt
to this same stick (no COLLECT_ONLY.txt), run the same single-component update,
complete/Resume and reboot normally. That does not undo speech-data resets.

Reports remain private: boot-status.txt, altscreen-handler.log, packet-audit.txt,
output-diagnostics.txt, network-status.txt, and map-v15-*.plist. No firmware
binary, vehicle identifiers, private reports or photos are published.
