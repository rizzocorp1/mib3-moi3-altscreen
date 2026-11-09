# MU0841 V19: slightly wider maneuver safe area

This is a layout test. The centered safe area changes from x480,width320 to
x464,width352: 10% wider, extending 16 canvas pixels on each side. The top
position y64, height320, full1280x480 canvas and top-aligned maneuver request
stay the same. Apps decide how their cards wrap and scale; larger fonts or a
particular layout are not guaranteed. Circular-gauge clearance needs checking.

V18 did not fix smooth refreshing. Its local enqueue/packetization timing and
the two-second keyframe fallback remain unchanged here. No smoother-video fix
is claimed for V19. Factory guidance overlays and their settings are unchanged.

## Parked test

1. Disconnect the iPhone. Insert USB and select ONLY MIB3OI_POST_SCRIPT at
   E9400L/Y940. Stop if other components or verification errors appear.
2. Complete the update and Resume with USB connected; never interrupt it.
3. Confirm the full VW map, then connect wired CarPlay through the other data
   port. Start with Apple Maps and circular gauges. Start a route/route preview
   and check whether the wider maneuver card remains clear of both gauges.
4. Switch once to Google Maps, then Waze, then Apple Maps. Note whether Google's
   Starting map display screen appears only briefly and whether it clears by
   itself. Keep USB connected. Refresh may still occur in one/two-second steps.
5. Disconnect CarPlay completely; wireless may remain connected after unplugging.
   Check the factory map returns, then optionally reconnect once.
6. After the updater is complete, finish with normal reboot and confirm stock
   CarPlay and turn-by-turn. Return USB. A short parked test is sufficient for
   the width comparison; driving is not required. The lease is about ten minutes.

The current logs cannot identify the active maps app or the cluster's exact UI
screen. The prior Starting map display observation is not treated as proof of
a factory-sender restart. Report a persistent failure if the map does not return.

## Storage and narrow recovery

New raw video capture stays off. Existing completed bounded recordings can be
exported with exact readback before unit-side removal. Existing USB videos and
private obsolete-program backup folders are preserved.

Before new staging, at least 32 MiB must be available on relevant filesystems.
The transient verified preflight helper is removed after setup. Housekeeping
backs up and verifies exact-hash obsolete V6-V18 program copies before removing
them, at most 24 copies or 32 MiB per update. It preserves active stages, factory
metadata baselines, diagnostic reports, changed/unknown files and directories.
This is not a complete uninstaller or complete vehicle rollback.

Do not publish firmware-derived USB backup folders, the built package, or vehicle
photos, reports and recordings. Only original source/tests/docs go to the fork.

Startup restores itself before RAM activation; reboot returns stock operation.
Explicit narrow startup recovery: add empty RESTORE_FACTORY_STARTUP.txt to this
stick (no COLLECT_ONLY.txt), run the same single-component update, complete/Resume
and reboot. Known signed-trigger speech resets still occur and are not reversed.
No full firmware flash, firewall change or permanent factory-executable edit.
