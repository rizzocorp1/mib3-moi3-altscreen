# V22: continuous Baseline timing diagnostic

V21 sent all 320 pictures without reported UDP errors and restored the native
service, but phase4 was not visibly displayed. V22 removes codec/profile changes:
it plays ONLY the known-visible Baseline/ref1 clip for 32 seconds at 10 fps.
The top number stays 1. The central counter repeats 0-79 four times. These repeats
are intentional; there are no four different codec phases in this test.

Four bounded log summaries measure actual scheduling lateness, start-to-start
intervals, mux/send duration, and PTS minus PCR. These are sender measurements,
not proof of the cluster's reception or presentation time. Transport timestamps,
packetization, codec data and pacing policy are unchanged from V21. Only the
initial takeover has a source-discontinuity flag; loop boundaries keep the same
codec configuration and continuous timestamps. This is a diagnostic, not a
claimed smooth-map repair.

## Parked test

1. Stay parked. Disconnect wired CarPlay and prevent wireless CarPlay from
   connecting (disable phone Wi-Fi/Bluetooth). Select the full factory VW map
   in the instrument cluster. No phone video is needed for this comparison.
2. Insert the USB and update ONLY MIB3OI_POST_SCRIPT at E9400L/Y940. Stop if other
   components or verification errors appear. Keep USB connected through the
   updater's reboot and complete Resume/return to normal mode. Never interrupt
   an active update.
3. AFTER the update is completely finished, confirm/select the full VW map.
   Remove the USB for at least FIVE seconds and reinsert it while parked.
   Do not start a second update. This is only the playback trigger. Do it within
   about two minutes of startup; a timeout leaves the factory sender untouched.
4. Watch the counter and moving bar for 32 seconds. Simply report whether motion
   is roughly regular, irregular, or repeatedly freezes. No counting is needed.
   Counter 0-79 repeats four times; the top number stays 1 throughout.
5. Keep USB connected. The factory sender should return after the 32-second
   sequence. If it does not, use a normal infotainment reboot AFTER the updater
   is complete. Report a persistent failure of the factory map.
6. Bring the USB back with boot-status.txt and codec-player.log even if no pattern
   appears. No route, driving or OBDeleven coding is needed. A parked recording
   is optional.

No driving, route or OBDeleven coding is needed. A parked video is optional.
If nothing appears, return the USB logs without repeating the update.

## Scope and recovery

Startup restores before the monitor starts. The CarPlay executable is unchanged
and is not bind-mounted. A verified runtime unit is created only if the named
file does not already exist; unknown existing units are preserved. /run is
volatile and the unit is not enabled for later boots. Controller cleanup and
ExecStopPost attempt factory restoration after completion, interruption, main
process death or runtime timeout, once the custom port is released. A normal
reboot remains the fallback if release or the factory service restart fails.

No new recordings, firmware flash, coding or old-stage cleanup. The small test
assets and a metadata-preserving startup backup are staged with a 32 MiB storage
preflight. Existing reports, private videos and USB backup folders are preserved.
Known carrier speech-file resets remain; startup recovery is not complete stock
rollback. Narrow startup recovery: empty RESTORE_FACTORY_STARTUP.txt on this
stick, same single-component update, complete/Resume and reboot.

Keep all vehicle reports, prior recordings, firmware-derived USB backups and
built carrier packages private. Only original source/tests/docs go to the fork.
