# V23: direct factory-versus-custom timing comparison

V22 showed steady sender starts at about 98-102ms while the user saw uneven
motion. V23 adds a bounded, read-only packet observer on the existing eth0.5
interface. It records PES cadence, PTS steps and PTS-minus-PCR on packets that
carry both fields, plus RTP continuity and packet-socket drop counts. It saves
metadata only: no map images, frame bodies or literal network addresses.

After the USB edge trigger, leave the full factory VW map visible for 20 seconds.
Then the unchanged V22 Baseline pattern plays for 32 seconds (counter 0-79 repeats
four times; top number 1). The observer labels factory/custom flows separately.
If capture readiness or outgoing factory PES timing is missing, playback is
refused and the factory sender stays running. No CarPlay route or driving is
needed. This comparison is intended to select a justified map timing change;
it is not itself a smooth-map release or a road-test map package.

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
4. Keep the full factory VW map visible for 20 seconds after reinserting. Do not
   connect CarPlay or change views. The test pattern should then run for 32 seconds.
   No counting or new video is required; simply note whether it appeared.
   Counter 0-79 repeats four times; the top number stays 1 throughout.
5. Keep USB connected. The factory sender should return after the 20-second reference and 32-second
   sequence. If it does not, use a normal infotainment reboot AFTER the updater
   is complete. Report a persistent failure of the factory map.
6. Bring the USB back with boot-status.txt, codec-player.log and packet-timing.log even if no pattern
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

No new image recordings, firmware flash, coding or old-stage cleanup. The small test
assets and a metadata-preserving startup backup are staged with a 32 MiB storage
preflight. Existing reports, private videos and USB backup folders are preserved. The observer runs at most70seconds; the runtime unit is bounded to240seconds.
Known carrier speech-file resets remain; startup recovery is not complete stock
rollback. Narrow startup recovery: empty RESTORE_FACTORY_STARTUP.txt on this
stick, same single-component update, complete/Resume and reboot.

Keep all vehicle reports, prior recordings, firmware-derived USB backups and
built carrier packages private. Only original source/tests/docs go to the fork.
