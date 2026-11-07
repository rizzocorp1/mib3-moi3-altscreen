# MU0841 stock capability capture v5 — one session

V4 disrupted CarPlay negotiation and did not reach an AltScreen request or frame decryption. Factory CarPlay and turn-by-turn returned after reboot. V4 is retired.

This diagnostic returns the factory display-information response unchanged and saves up to three copies privately. It does not advertise AltScreen, alter feature lists, start video receivers, retain pairing contexts, decrypt video, or stop/disable the native cluster encoder. No new map is expected. The snapshot API's format/path argument order has been checked against the matching factory library; this firmware accepts the binary1 format.

The package is experimental and loading a library is still an on-car test. Only use it on the verified 3G5035842B / H56 / 0841 unit. The signed trigger repeats the four previously accepted speech resets. Startup restores its original bytes/metadata before a one-session RAM bind mount; the original executable remains unchanged on disk. Reboot ends the diagnostic. This is not a full vehicle backup.

## One parked capture

1. Park with stable update power, using the prior precautions (VW specifies a charger supplying at least 70 A for MIB3 updates). Disconnect CarPlay first. Keep this USB connected through the update and capture.
2. Use the same Engineering update process. Verify **E9400L / Y940** and **only MIB3OI_POST_SCRIPT**. Stop before starting if another component or a verification error appears.
3. Let the update finish, choose **Resume**, and allow normal mode to load. Never remove USB or reboot during an active update.
4. Without an extra reboot, connect **wired CarPlay** via the other data port. Start an Apple Maps route while parked and leave it connected for about one minute. Normal CarPlay and factory turn-by-turn are expected; this diagnostic does not output cluster video.
5. Leave USB connected for another 15 seconds. After the updater is finished, reboot normally to end the session. Confirm normal CarPlay/turn-by-turn, then return the USB to the computer.

Expected reports: **altscreen-setup.txt**, **boot-status.txt**, **info-probe.txt**, and up to three **stock-info-1.plist**, **stock-info-2.plist**, **stock-info-3.plist** snapshots. The files are binary; no need to open them. An optional **previous-modified-info.plist** may be absent because the old snapshot call was incorrect. The factory journal was unavailable in earlier tests.

**DIAGNOSTIC ACTIVE** confirms the probe was mounted with native encoder unchanged. **STOCK_INFO_SNAPSHOT ... status=0** indicates a snapshot was completed; **STOCK_INFO_PASSTHROUGH** records the original response being returned. If CarPlay fails, do not repeat installation: finish the updater, reboot normally, and bring back the USB.

Keep every snapshot and report private; capability responses may contain device identifiers. Do not publish the archive because it contains a firmware-derived executable copy.

For explicit recovery, put an empty RESTORE_FACTORY_STARTUP.txt on this same v5 USB (no COLLECT_ONLY.txt), run the same single-component updater, finish it normally, Resume and restart normally. This repeats the speech resets and retains backup assets. Do not mix package versions or use R9900 recovery files.
