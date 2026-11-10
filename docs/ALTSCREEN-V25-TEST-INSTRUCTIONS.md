# V25: stop forced keyframe requests during active output

Recovered V24 logs show 9663 received access units,all authenticated and muxed,
zero invalid frames and zero reported UDP send errors across two phone sessions.
The 390configuration records have identical logged SPS/PPS metadata; 198 native
forceKeyFrame submission messages occurred on an approximately 2-second schedule.
This is a correlation,not proof that requests caused the cluster loading screen.

V25 no longer queues periodic forceKeyFrame commands while authenticated output
is active. The one-time top-aligned layout command remains. A configuration
message clears the handoff flag,so the existing 2-second rate-limited recovery
path remains while waiting for an IDR. A command queued before output becomes
active may still complete. No reconnect/crypto policy change is included.

Video PES length remains factory-zero,clocks/keyframe handling and 352x320 safe
area remain unchanged. No smooth-video fix is claimed before hardware testing.
The report collector also writes a bounded compressed ASCII bundle with source
hashes,zero counts,fsync and readback,using the method that recovered V24. This is
reporting only and does not change video bytes or save raw frames.

## Installation and test

1. Park, disconnect CarPlay, insert USB and update ONLY MIB3OI_POST_SCRIPT at
   E9400L/Y940. Keep USB connected through reboot and complete Resume/normal
   mode. Never interrupt an active update.
2. Select the full factory VW map, then connect wired CarPlay through the other
   data port. Start Apple Maps and a route. V25 is a MAP test, not the numbered
   counter diagnostic. There is no USB unplug/reinsert trigger.
3. While parked, confirm normal CarPlay, a visible cluster map and normal gauges.
   Note whether loading-screen interruptions decrease and whether updates look smoother. If CarPlay or the cluster misbehaves,
   finish with a normal reboot after the update is complete rather than drive.
4. If the parked check works, a short road test can compare movement and turns.
   Set the route before moving; make observations without operating test controls
   while driving. No counting, filming or app switching is required. The existing
   session lease is about 10minutes from startup, so factory restoration near
   that limit is expected and this is not a permanent installation.
5. Park before changing apps, disconnecting or rebooting. Keep USB connected for
   the report export. A normal infotainment reboot restores the stock startup
   path and removes the temporary executable bind mount.
6. Return the USB with map-v25-reports.json and report loading-screen frequency, refresh behavior and any CarPlay issues. No manual counting is needed.

New raw video capture stays disabled. Prior private evidence is preserved.
Staging requires 32MiB free space. Existing exact-hash housekeeping/recovery
behavior is retained; original factory executable bytes are not overwritten.
This does not reverse the accepted signed-carrier speech-file resets and is not
complete vehicle rollback. Narrow startup recovery uses the empty
RESTORE_FACTORY_STARTUP.txt marker and the same single-component completed
update/reboot sequence if necessary.

All vehicle reports, videos, firmware-derived binaries, packages and backups
remain private. Only reviewed original source/tests/docs are published.
