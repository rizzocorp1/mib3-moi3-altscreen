# MU0841 v16: circular-gauge safe area and private capture recovery

Your intended layout is the CIRCULAR tachometer and speedometer. V15's960px
safe width let the card extend behind those circles. V16 uses a centered320px
safe width: x480,y64,width320,height320 (right800,bottom384), retaining the
1280x480 canvas and topaligned maneuver request. Approximate measurements of
the supplied photos put the narrowest gap near336–346canvaspx. These are
camera-based calibration estimates, not exact panel metrology; check the
arrow and full maneuver text in the circular-gauge view. The map background
can remain behind the gauges. No whole-video scaling or cropping is added.

One cluster-wide preference applies to all map providers. Apps may respond
differently. Main CarPlay dimensions/input and factory cluster overlays remain
native. A wider full-map-specific profile or automatic gauge-mode switching is
not implemented; this candidate deliberately targets your intended circles.

V15 logged a completed84-frame private recording but no video file arrived on
the returned USB. It could not be decoded offline. V16 first attempts to recover
the saved v15 recording during the updater, before arming the new test. A native
helper validates completion, format, sizes, frame count and relative timestamps,
copies to USB, and compares every byte before removing unit-side source files.
It needs no optional head/stat/cp utility flags. Failure is reported and evidence
is retained where possible. Existing differing USB output is never overwritten.

This test also collects one fresh private sample, bounded3seconds/90frames/2MiB,
at a substantial authenticated IDR after handoff. It contains map imagery and
relative PTS, no pairing/control records, keys, audio or absolute phone time.
Keep map-v15-video.bin and map-v16-video.bin PRIVATE; do not upload them to GitHub.
The collector also tries old/new recovery. Capture/export on this unit remains
to be verified. No claim is made that v16 fixes the two-second updates or colored
video artifacts. Those need the actual recording for decoder analysis.

## One parked test

1. Use stable update power, iPhone disconnected and this USB connected.
2. Select ONLY MIB3OI_POST_SCRIPT at E9400L/Y940. Stop before starting if other
   components or verification errors appear. Complete the update and Resume
   with USB connected. Do not interrupt an active update.
3. Confirm native VW map, then connect wired CarPlay through the other data port.
   Start with APPLE MAPS, set a route while parked and use CIRCULAR GAUGES.
4. Check the full maneuver arrow/text fits between the circles near the top,
   and whether the position marker remains above the footer. A full-cluster
   photo with both dials is useful. Allow at least20seconds for capture/export.
5. Compare Waze and Google Maps separately with routes. Two-second updates are
   still expected. Note clipped text, wrong app, corruption or missing map.
   There is no need for another four-minute duration-only check.
6. Fully disconnect CarPlay and confirm native map returns. Wired unplugging
   may leave wireless CarPlay active. Reconnect once if desired. Leave USB
   connected during testing so private recordings/reports can export.
7. After the updater is complete, finish with normal reboot and confirm stock
   functions. At about ten minutes the experiment expires and attempts native
   handback. Return the USB, including all private video files and reports.

Known signed-trigger speech resets remain. This is a one-shot runtime test with
verified startup restoration and narrow recovery, not a full factory backup.
No firewall, SMACK, coding or persistent factory-executable changes. Explicit
startup recovery: add empty RESTORE_FACTORY_STARTUP.txt on this same stick
(no COLLECT_ONLY.txt), run the same single-component update, finish/Resume,
then reboot normally. That does not undo earlier speech-data resets.

Private report files: altscreen-setup.txt, boot-status.txt, altscreen-handler.log,
map-v16-*.plist, packet-audit.txt, output-diagnostics.txt, network-status.txt and
map-v15-video.bin/map-v16-video.bin. Export failures now include native errors;
the absence of a recording will not be reported as successful decoding.
