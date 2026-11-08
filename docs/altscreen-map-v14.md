# MU0841 v14: video clock and reference diagnostics

This is a targeted diagnostic experiment, NOT a claimed smooth-video fix.
The working v13 video path, two-second keyframe fallback, layout and bounded
session restoration remain unchanged. V14 observes relative phone/arrival
timing, repeated timestamps, clock clamps/rebases, and bounded H.264 slice
header metadata (slice type and reference frame number). It stores no picture
payloads, timestamps from the phone's absolute clock, keys or raw frame dumps.
Reference gaps are observations, not proof of a corrupt stream. Unsupported
codec syntax disables that observation; video is still forwarded unchanged.

Why this test: synthetic Baseline and High-profile dependent pictures decode
through the actual AVC conversion, so basic software decoding alone does not
explain the cluster's two-second picture changes. Factory firmware uses local
clock timing; our live phone-clock and reference behavior needs measurement
before changing it. In particular, reordered pictures and clamped timestamps
must be distinguished from cluster decoder compatibility. No forced profile
label rewrite or faster keyframe requests are introduced.

## One parked test

1. Keep stable update power. Disconnect the iPhone and keep this USB connected.
2. Select ONLY MIB3OI_POST_SCRIPT at E9400L/Y940. Stop before starting if other
   components or verification errors appear. Complete the update and Resume
   into normal mode with USB connected; do not interrupt an active update.
3. Confirm the VW cluster map first. Connect wired CarPlay through the other
   data port. Start a Google Maps route and leave that app selected for FOUR
   MINUTES. Stay parked; changing route preview or map zoom can create visible
   content changes. Two-second updates are still expected in this version.
4. Switch once to Apple Maps with a route for about30seconds, then Waze for
   about30seconds. Note if the cluster corrupts, freezes or changes apps.
5. Fully disconnect CarPlay, check the factory map returns, then reconnect once
   and check the phone map returns. Keep the USB connected for report export.
   Unplugging wired CarPlay may leave wireless CarPlay connected.
6. The test lease is approximately ten minutes from startup. At expiry the
   monitor deliberately withdraws custom output and attempts factory restoration.
   Normal reboot remains the fallback. After the updater is complete and the
   test ends, reboot normally, check stock functions and return the USB.

No firewall, SMACK, coding or native service definition changes. Original
executable remains unchanged on persistent storage; startup restores before
RAM-only activation. The signed USB trigger retains the previously accepted
four speech-data resets; this is not a complete vehicle backup or rollback.
No full firmware flash or R9900 recovery package is involved.

Reports stay private: boot-status.txt, altscreen-handler.log, packet-audit.txt,
output-diagnostics.txt, network-status.txt and map-v14-*.plist. No vehicle logs
or images are published. NATIVE RESTORED verifies a free port and active factory
service, not an independent visual-map check.

Explicit startup recovery: add an empty RESTORE_FACTORY_STARTUP.txt on this
same stick (no COLLECT_ONLY.txt), run the same single-component update, complete
and Resume, then reboot normally. That restores startup only; it does not undo
the speech-data resets or claim a full factory reflash.
