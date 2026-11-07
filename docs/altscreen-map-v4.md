# MU0841 HomeKit-capable AltScreen map attempt v4

This attempts an actual CarPlay cluster map. The preceding observation-only test confirmed HomeKit session security and ChaCha screen encryption during WIRED CarPlay. V4 derives the auxiliary stream's key from that session and its stream connection ID, authenticates the entire header and encrypted frame, and rejects failed authentication. No keys or frame contents are logged. It preserves factory feature-list entries and primary-only SETUP responses.

This package is still experimental. Host tests and a matching factory-library build cannot establish success on the car. The confirmed diagnostic applied to wired CarPlay; start with wired CarPlay again. Wireless remains unverified.

Only for verified 3G5035842B / H56 / 0841. The signed component repeats the four previously accepted speech resets. Startup restores the exact original startup bytes/metadata BEFORE one-shot activation. The original executable is unchanged on disk; the temporary copy is bind-mounted from RAM. The native multi-display video encoder is stopped for this session, without disabling/editing its service. Factory navigation video may be unavailable until reboot. This is a narrow startup recovery mechanism, not a full vehicle backup.

## One parked map attempt

1. Park with stable update power; VW specifies a charger supplying at least 70 A for MIB3 update operations. Disconnect CarPlay before the update. Keep the test USB connected through the updater and test.
2. Use the same Engineering update process. Verify **E9400L / Y940** and **only MIB3OI_POST_SCRIPT**. If CPU/HMI/MCU, another component or verification errors appear, stop before starting.
3. Start that single-component update. Let it finish, select **Resume**, and allow normal mode to load. Never interrupt an active update or remove its USB.
4. Without an extra reboot, connect the iPhone by cable using the other data port while leaving this USB connected. Select the cluster navigation/map view and start an **Apple Maps** route while parked. Allow up to two minutes.
5. Note whether wired CarPlay connects and whether any map, image, or error appears in the cluster. Do not repeat installation if it fails. The logger runs about five minutes after activation.
6. Once the updater is fully finished and the test is complete, reboot infotainment normally to end this one-session experiment. Check factory CarPlay and turn-by-turn again. Reboot returns to the factory executable/startup/encoder; it does not rearm the test.
7. Return the USB to the computer with **altscreen-setup.txt**, **boot-status.txt**, **altscreen-handler.log** and **carplay-journal.txt** if available. Keep logs private. The factory journal was unavailable in the diagnostic; the separate file log is the primary path.

**MAP TEST ARMED** proves setup only. **MAP TEST ACTIVE** proves the test copy was mounted and the native encoder stopped. Handler **AltScreen security ready: HomeKit ChaCha20-Poly1305** proves key derivation/init succeeded. A first-frame message and transmission counters establish subsequent receiver stages, not visible rendering. **ABORT**, authentication failure or a blank map is a reason to bring back reports, not rerun the update.

## Explicit recovery

Create an empty **RESTORE_FACTORY_STARTUP.txt** on this same v4 USB, with no COLLECT_ONLY.txt, and run the same single-component update. Let it finish, Resume and restart normally. This restores the v4 startup backup and removes its ticket/recognized test mount. It repeats the accepted speech resets and retains backup assets. Do not mix v1/v2/v3/v4 files or use R9900 recovery packages.

Keep the archive private: it contains a firmware-derived executable copy. No full CPU/HMI firmware flash is involved.
