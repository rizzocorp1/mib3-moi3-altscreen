# V24: CarPlay map test with factory-style video PES length

The direct V23 capture found stable custom 100ms PES cadence, no RTP sequence
gaps and zero packet-capture drops. Both factory and custom PTS/PCR lead were
near zero; those results do not support a guessed timestamp offset. The factory
reference was stationary; the user's smooth zoom observation occurred afterward
and was not captured. PES starts are not automatically a count of displayed
pictures.

Matching firmware getStreamPacket writes video PES length bytes as zero. V24
aligns those two header bytes with the factory. Encoded picture bodies, clocks,
TS/RTP packetization, keyframe fallback and the centered 352x320 safe area remain
unchanged from the working V19 map path. This is a single transport hypothesis,
not proof of the slow-refresh cause or a guarantee of smooth video.

## Installation and test

1. Park, disconnect CarPlay, insert USB and update ONLY MIB3OI_POST_SCRIPT at
   E9400L/Y940. Keep USB connected through reboot and complete Resume/normal
   mode. Never interrupt an active update.
2. Select the full factory VW map, then connect wired CarPlay through the other
   data port. Start Apple Maps and a route. V24 is a MAP test, not the numbered
   counter diagnostic. There is no USB unplug/reinsert trigger.
3. While parked, confirm normal CarPlay, a visible cluster map and normal gauges.
   Note whether updates look smoother. If CarPlay or the cluster misbehaves,
   finish with a normal reboot after the update is complete rather than drive.
4. If the parked check works, a short road test can compare movement and turns.
   Set the route before moving; make observations without operating test controls
   while driving. No counting, filming or app switching is required. The existing
   session lease is about 10minutes from startup, so factory restoration near
   that limit is expected and this is not a permanent installation.
5. Park before changing apps, disconnecting or rebooting. Keep USB connected for
   the report export. A normal infotainment reboot restores the stock startup
   path and removes the temporary executable bind mount.
6. Return the USB and report the map refresh behavior and any CarPlay issues.

New raw video capture stays disabled. Prior private evidence is preserved.
Staging requires 32MiB free space. Existing exact-hash housekeeping/recovery
behavior is retained; original factory executable bytes are not overwritten.
This does not reverse the accepted signed-carrier speech-file resets and is not
complete vehicle rollback. Narrow startup recovery uses the empty
RESTORE_FACTORY_STARTUP.txt marker and the same single-component completed
update/reboot sequence if necessary.

All vehicle reports, videos, firmware-derived binaries, packages and backups
remain private. Only reviewed original source/tests/docs are published.
