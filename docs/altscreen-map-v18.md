# MU0841 V18: local enqueue and packetization timing

Stock turn-by-turn has been confirmed working after reboot. This test leaves
factory guidance and its settings alone.

V17 did not fix smooth refreshing. V18 changes only the video clock policy:
presentation time is stamped when an authenticated frame enters output, and PCR
is read again at packetization, both from the same local monotonic epoch with
millisecond quantization. This follows the factory transport's queue/send clock
separation. Phone timing remains diagnostic; no phone absolute time is recorded.
There is no invented presentation lead or change to encoded picture data.
This is a compatibility experiment, not a confirmed smooth-video fix.

The working circular-gauge layout and V17 discontinuity/IDR markers remain.
The two-second keyframe fallback is unchanged, allowing a direct comparison of
whether dependent pictures begin displaying. New raw video capture remains off.

## Parked test

1. Disconnect the iPhone. Insert USB and select ONLY MIB3OI_POST_SCRIPT at
   E9400L/Y940. Stop if other components or verification errors appear.
2. Complete the update and Resume with USB connected; never interrupt it.
3. Confirm the full VW map, then connect wired CarPlay through the other data
   port. Use Apple Maps first with circular gauges. Change route previews while
   parked so picture changes are observable. Driving is not required for this.
4. Compare Waze and Google Maps. Note smooth versus two-second updates and
   colored/block artifacts. Keep USB connected for reports.
5. Disconnect CarPlay completely (wireless can remain connected after unplugging
   the cable), check the factory map returns, and optionally reconnect once.
6. After the update is complete, finish with a normal reboot. Confirm stock
   CarPlay and turn-by-turn, then return the USB. The lease is about ten minutes.

## Storage and recovery

Before staging, at least 32 MiB must be available on the relevant filesystems.
The verified transient preflight helper is removed after setup completes.
Housekeeping records free space and inventories our stages. It backs up and
verifies only exact-hash old program copies (V6-V17) before removing them, capped
at 24 copies or 32 MiB per update. Active stages, factory metadata backups,
reports, unknown/changed files and directories are retained. This is not a
complete uninstaller. No new raw video recording occurs. Previously completed
bounded recordings can be exported and verified before their unit-side removal.
Existing USB recordings are preserved.

The USB obsolete-program backup folder contains private firmware-derived files.
Do not publish it, the built package, photos, video or vehicle reports.

Startup restores itself before RAM activation; a reboot returns stock operation.
Explicit narrow startup recovery: add empty RESTORE_FACTORY_STARTUP.txt on this
stick (no COLLECT_ONLY.txt), run the same single-component update, finish/Resume
and reboot. Factory executable storage is unchanged. Known signed-trigger speech
resets still occur and are not reversed by this mechanism. It is not complete
vehicle rollback or a full firmware flash.
