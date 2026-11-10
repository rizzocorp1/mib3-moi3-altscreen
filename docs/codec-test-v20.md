# V20: parked-only synthetic codec comparison

This is NOT a map improvement release. It temporarily replaces the cluster's
VW map video with four moving counter patterns to investigate slow refreshing.
Do not run it while driving. It does not need a CarPlay connection.

1. While parked, disconnect wired CarPlay and prevent wireless CarPlay from
   connecting (disable the phone's Wi-Fi/Bluetooth). Select the cluster's full
   built-in VW map view, the same view that normally shows a moving factory map.
2. Insert this USB. In the updater select ONLY MIB3OI_POST_SCRIPT at E9400L/Y940.
   Stop if other components or verification errors appear. Complete the updater
   and Resume with USB CONNECTED. Never interrupt an active update.
3. Once the updater has COMPLETED and you are back in normal mode, confirm/select
   the full VW cluster map again. Remove the USB, wait at least FIVE seconds,
   then reinsert it while remaining parked. Do this within about two minutes of
   startup. This removal is only a post-update test trigger, never a mid-update
   instruction. If the monitor times out, factory video stays unchanged.
4. Watch four phases, eight seconds each (32 seconds total). A number near the
   top identifies phase 1, 2, 3 or 4. The middle counter goes 0 through 79 and a
   lower bar moves. Note which phases update regularly versus freezing/jumping.
   Phase 1: Baseline, one reference frame, intermediate frames.
   Phase 2: High profile, one reference frame, intermediate frames.
   Phase 3: High profile, four reference frames, intermediate frames.
   Phase 4: High profile, every picture independently decodable.
5. Keep the stick connected. The factory sender should return automatically
   after playback. If it does not, use a normal infotainment reboot once the
   updater is complete. Report any persistent loss of the factory map.
6. Return the USB with boot-status.txt and codec-player.log. Report what each
   phase looked like; a passenger-free parked observation is sufficient. A
   stationary recording is optional. Do not drive for this test.

The patterns are synthetic and contain no vehicle, map, location or phone data.
They use one fixed transport and pace. Codec cases change reference/entropy
coding together with profile; all-intra also differs in bitrate and SPS flags.
The comparison narrows causes but does not by itself prove cluster capability.
Offline checks are not on-car validation, and no smooth-map fix is claimed.

Startup restores before the bounded monitor starts. The CarPlay executable is
not patched or bind-mounted. The monitor verifies clip hashes, waits for the
USB trigger, confirms the factory sender stopped, and requires its source port
exclusively. Playback is bounded and interrupted on pacing/send failure.
Normal exit, signal handling and a systemd post-stop action attempt factory
restoration after the player releases the port. A normal reboot remains the
fallback if ownership cannot be released or the factory service fails to start.

No new video recording or old-stage cleanup occurs. Only the small player,
validated synthetic clips, controller, metadata helper and factory startup
metadata backup are staged. Existing USB evidence/backup folders are preserved.
Free-space preflight requires 32 MiB on relevant filesystems.

Known signed-trigger speech-file resets remain and are not reversed by reboot
or startup recovery. This is not complete vehicle rollback. Explicit narrow
startup recovery uses empty RESTORE_FACTORY_STARTUP.txt on this same stick,
then the same single-component update, complete/Resume and reboot.
Do not publish the carrier ZIP, vehicle reports, prior videos or firmware-derived
backup folders. Only original source and synthetic tests/docs go to the fork.
