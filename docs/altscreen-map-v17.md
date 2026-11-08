# MU0841 v17: transport source signaling and verified housekeeping

Before installing: with USB UNPLUGGED, check factory turn-by-turn on a fresh
Google Maps route (or whichever normally appears while parked) and the cluster
Navigation view. There is no need to drive solely for this check. If you confirm
factory guidance is still missing when it normally works, do not install this
test yet; report that first. Reboot/startup and native video handback in the logs
do not independently verify the separate factory route-guidance overlay.

## What this tests

The actual v15/v16 phone recordings decoded84/84 and83/83pictures without errors.
Replaying those same recordings through our production MPEG-TS/RTP functions
produced pixel-identical output. This narrows the remaining issue to behavior
not reproduced by that offline test, including the cluster's decoder and live
transport timing. It does not prove that every on-car frame was displayed.

V17 marks the first video/PCR packet after source takeover as discontinuous and
marks IDR access units as random-access points. Ordinary dependent pictures are
not marked random-access. Markers reset on disconnect/reconnect, not every
configuration or keyframe. Encoded pictures, PTS/PCR values, resolution and
two-second fallback rate are unchanged. This is a targeted compatibility test,
not a confirmed smooth-video fix. Reference for transport marker semantics:
https://ffmpeg.org/ffmpeg-formats.html

The now-working circular-gauge safe rectangle remains x480,y64,width320,height320.
The canvas stays1280x480. There is no font-size change, map zoom, factory-overlay
suppression, coding change or unverified per-app layout preference in this test.
A separate factory turn-by-turn off switch has not been confirmed for this car.

## Storage and privacy

New video recording is DISABLED: the two existing samples are sufficient for
current offline checks. Previously saved bounded captures can still be recovered
privately, with native record validation and byte-readback verification.

Before staging, the updater requires at least32MiB available on the relevant
app_rw/data filesystems; a failed check aborts without arming a new test. This
is a test-footprint reserve, not a claim about the unit's total resources.
The updater records filesystem space and inventories our named test stages in
housekeeping-v17.txt. It can remove ONLY known old v6–v16 program-copy files whose
exact hashes match our allowlist, whose factory baseline/metadata verify, and
whose USB backups verify first. It skips active test loggers, changed/unknown
files, symlinks, conflicting backup output and failed copies. No recursive
directory deletion occurs. Work is capped at24 copies/32MiB per update; remaining
copies are retained for a later verified pass. Old factory metadata backups and diagnostic reports
remain; this is not a complete uninstaller. The current recovery baseline remains
on the unit with its Linux metadata. USB backup copies cannot preserve all Linux
security metadata and are not represented as complete factory recovery.

moi3-obsolete-programs-v17 on USB contains private firmware-derived test binaries:
never publish that folder. Vehicle reports, recordings and photos remain private.
Available bytes before/after cleanup are measured on the actual unit, not inferred
from the firmware image. Cleanup may be refused safely without preventing the
otherwise valid one-shot test; review its report after returning the stick.

## Parked test

1. Complete the stock-guidance check above first. Use stable update power.
2. Disconnect the iPhone, insert this USB and select ONLY MIB3OI_POST_SCRIPT at
   E9400L/Y940. Stop before starting if other components or verification errors
   appear. Complete/Resume with USB connected; never interrupt an active update.
3. Confirm the full built-in VW map before connecting CarPlay, then connect wired
   CarPlay through the other data port. Start with Apple Maps and circular gauges.
4. While parked, change a route preview/route selection to make map changes
   observable. Note whether updates are smoother than two-second steps or unchanged.
   Compare Waze and Google Maps separately and note any colored/block artifacts.
5. Leave USB connected. Disconnect CarPlay completely and verify VW map returns;
   wired unplugging can leave wireless CarPlay active. Reconnect once if desired.
6. After the updater is complete, finish with normal reboot. Check factory
   CarPlay and turn-by-turn again. Return USB including housekeeping-v17.txt, resource-preflight-v17.txt and
   the private obsolete-program backup folder. The lease remains about ten minutes.

Known signed-trigger speech resets remain. No firewall, SMACK, coding, permanent
factory executable changes or full firmware flash. Startup restores before RAM
activation. Explicit startup recovery: add empty RESTORE_FACTORY_STARTUP.txt on
this same stick (no COLLECT_ONLY.txt), run the same single-component update,
finish/Resume and reboot. That is narrow recovery, not reversal of speech resets.
