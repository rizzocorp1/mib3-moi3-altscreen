# MU0841 map attempt v6 — initial acknowledgement and deferred handoff

The stock v5 capture succeeded. Factory /info contains no enabledFeatures or viewAreas entries, so the proposed preserved-viewAreas conflict was not present. V6 restores AltScreen acknowledgement during initial SETUP, based on the v1 trace that reached an auxiliary request. This is a candidate negotiation correction, not a proven vehicle fix. It keeps primary media SETUP responses unchanged and retains the HomeKit receiver.

The native encoder stays running during startup/negotiation. After a HomeKit-authenticated frame passes H.264 validation and is an IDR/keyframe, the receiver requests an encoder handoff. A separate root collector stops the native encoder and grants output only if that stop succeeds. Test output waits for a valid grant and an IDR. Failed handoff attempts restart native encoding and withhold test output. An auxiliary stream ending before the grant cancels its request. No frames are transmitted to the cluster before the grant.

This still is an experimental map test. Host tests cannot establish on-car success. Only use on verified 3G5035842B / H56 / 0841. The signed trigger repeats the four accepted speech resets. Startup restores exact original bytes/metadata before activation; the original executable is unchanged on disk. A RAM copy is bind-mounted for one session. Reboot ends the experiment. The recovery mechanism is narrow, not a full vehicle backup.

## One parked wired map test

1. Park with stable update power, using prior precautions (VW specifies at least a 70 A charger for MIB3 update operations). Disconnect CarPlay first. Leave this USB connected through the update/test.
2. Use the same Engineering updater. Verify **E9400L / Y940** and **only MIB3OI_POST_SCRIPT**. Stop before starting if other components or verification errors appear.
3. Let the update finish, choose **Resume**, and allow normal mode to load. Do not remove USB or reboot during an active update.
4. Without an extra reboot, connect **wired CarPlay** through the other data port. Select the cluster navigation/map view and start an **Apple Maps** route while parked. Allow up to two minutes.
5. Note CarPlay connectivity, factory turn-by-turn and whether any map/image appears. If it fails, do not repeat installation. The logger runs about five minutes.
6. After the updater is fully finished and the test ends, reboot normally to remove the test copy. Confirm factory CarPlay and turn-by-turn, then return USB to the computer.

Expected reports: **altscreen-setup.txt**, **boot-status.txt**, **altscreen-handler.log**, plus **map-v6-stock-info.plist** and **map-v6-advertised-info.plist** if captured. The snapshots are binary and may contain device identifiers; keep everything private. The factory journal was unavailable in earlier tests.

**MAP TEST ACTIVE** means the test copy loaded with native encoder initially retained. **AltScreen acknowledgement: initial=1** identifies the negotiation change. **HANDOFF ACTIVE** means the native encoder stopped after authenticated video became available. Authentication, IDR and transmission logs show receiver progress; only observing the cluster establishes visible success. No handoff should occur when the peer never requests or sends valid auxiliary video.

For explicit recovery, add an empty RESTORE_FACTORY_STARTUP.txt to this same v6 USB (no COLLECT_ONLY.txt), run the same single-component update, let it finish, Resume and reboot normally. This repeats the speech resets and retains backup/test assets. Do not mix versions or use R9900 files. No complete CPU/HMI firmware flash is included.
