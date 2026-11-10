# Atlas MU0841 AltScreen v2 — parked map-display experiment

Only for the verified 3G5035842B / H56 / 0841 head unit. Wired CarPlay and factory Apple Maps turn-by-turn were confirmed working after v1 recovery. V1 is retired.

This test attempts an actual CarPlay map on the cluster. It is not yet proven on the car. V2 preserves the factory enabled-feature list, leaves primary-only SETUP responses unchanged, and requires an independent logger to report ready before mounting the test executable or stopping the native cluster video encoder. Handler messages also go to the CarPlay journal. Logs distinguish handler loading, display advertisement, stream negotiation, receipt of the first video frame, and transmission; none of those alone proves the cluster displayed the map.

The signed engineering component still deletes the four previously accepted speech-data paths. This is not a stock firmware flash or complete backup. The original executable remains unchanged on disk. The temporary startup restores the verified original startup before activating a one-session RAM bind mount. Reboot removes that mount and starts the native encoder normally. V1 assets are retained; available v1 boot and handler logs are copied to USB before v2 arms.

## Run once

1. Park the vehicle and use the same update power precautions as before. VW specifies a charger supplying at least 70 A for MIB3 update operations. Keep the test USB connected throughout the updater and test.
2. Enter Engineering using the previously successful menu route. Choose **Update**, select this USB, and verify **E9400L / Y940** with **only MIB3OI_POST_SCRIPT**. If CPU, HMI, MCU, other components, or a verification error appear, stop before starting.
3. Start that single-component update. Let it finish and choose **Resume** when offered. Never remove the USB or restart in the middle of an update. Allow normal mode to load.
4. Without an extra reboot, select the cluster navigation/map view. Connect CarPlay wirelessly, or use the other data port for wired CarPlay while this USB remains connected. Start an **Apple Maps** route while parked. Wait up to two minutes. The logger runs for about five minutes after activation.
5. Note whether CarPlay connects, whether a map appears, and what the cluster shows. A blank view or failed connection is a failed experiment; do not repeat the updater. Native navigation video may be absent because its encoder is stopped during the session.
6. After the updater is fully finished, end the experiment with a normal infotainment power-button reboot. Check wired CarPlay and factory turn-by-turn again. Reboot ends the one-session test; it is not a way to retry it.
7. Once the updater is inactive and the test has ended, reconnect the USB to the computer. Keep **altscreen-setup.txt**, **boot-status.txt**, **altscreen-handler.log** (if present), **carplay-journal.txt**, and any **previous-v1-*** reports. Do not publish them.

**ARMED** in setup means installation only. **LOGGER: ... ready** proves the independent logger handshake. **ACTIVE** proves the test bind mount and encoder-stop path ran. **ABORT** means the map experiment did not activate successfully; bring back the reports rather than rerunning it.

## Explicit recovery if required

On this same v2 USB, create an empty **RESTORE_FACTORY_STARTUP.txt**, with no **COLLECT_ONLY.txt**. Run the same single-component updater, finish it normally, Resume, then restart normally. This restores the v2 startup backup and removes its ticket and any recognized v2 test mount. It repeats the known speech resets. Retain all backup assets. The old v1 recovery USB is archived on the computer and must not be mixed into this package.

No R9900 recovery files or original FAZIT/ksend script are used. Do not manually mix files from v1 and v2.
