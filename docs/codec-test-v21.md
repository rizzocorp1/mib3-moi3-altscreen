# V21: corrected launcher for the parked codec comparison

V20 did not play: its startup report showed systemd-run rejected
TimeoutStopSec=10 before the monitor could start. No codec result was obtained.
The factory startup and executable hashes were confirmed original afterward.

V21 uses a temporary service file under /run/systemd/system. The firmware's
systemd 232 unit-file parser supports the needed timeout and post-stop settings;
its narrower systemd-run command-line property parser is no longer used.
Early bootstrap errors are now copied directly to USB. The player and four
validated synthetic clips are unchanged. This is a diagnostic, not a map fix.

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
4. Watch four phases, eight seconds each. The top number identifies phase 1-4;
   the central counter goes 0-79 and a lower bar moves. Report which phases
   update regularly and which freeze, jump or fail to appear:
   1: Baseline/ref1 with intermediate pictures.
   2: High/ref1 with intermediate pictures.
   3: High/ref4 with intermediate pictures, closer to the phone's stream.
   4: High profile with every picture independently decodable.
5. Keep USB connected. The factory sender should return after the 32-second
   sequence. If it does not, use a normal infotainment reboot AFTER the updater
   is complete. Report a persistent failure of the factory map.
6. Bring the USB back with boot-status.txt and codec-player.log even if no pattern
   appears. No route, driving or OBDeleven coding is needed. A parked recording
   is optional.

The comparison narrows codec versus live-delivery hypotheses. Codec cases also
differ in entropy coding; all-intra differs in bitrate and SPS flags. It does not
by itself prove a smooth-map fix or reveal commercial implementations.

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
