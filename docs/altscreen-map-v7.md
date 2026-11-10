# MU0841 map attempt v7 — auxiliary endpoint correction

V6 restored wired CarPlay and reached the auxiliary stream request and HomeKit key derivation, but the phone never connected to its TCP listener. No video was authenticated, no handoff occurred, and the native encoder remained running. The teardown accept error was normal shutdown, not proof of a failed accept during the connection.

V7 returns a minimal auxiliary stream response containing only type and the actual dataPort. It does not echo request-only fields or add an invented streamID. Existing duplicate auxiliary replies are replaced while primary replies are retained. It asks the OS for an available listener port and advertises the actual assigned port. Logs now include factory/final stream types and ports, listener family, and bounded read-only network snapshots. This is a candidate endpoint correction, not a proven vehicle fix.

Initial SETUP acknowledgement, HomeKit frame authentication and deferred encoder handoff are retained. Native encoding stays running until authenticated IDR video passes validation. Test output waits for a metadata-preserving grant and an IDR; failed handoff restarts native encoding. Reboot ends the one-session test. No native service is disabled/edited and no firewall rule is changed.

Only for verified 3G5035842B / H56 / 0841. The signed trigger repeats the four accepted speech resets. Startup restores original bytes/metadata before mounting the RAM test copy; the original executable is unchanged on disk. This is a narrow recovery mechanism, not a full vehicle backup. Hardware success remains unproven.

## One parked wired test

1. Park with stable update power using prior precautions (VW specifies at least a 70 A charger for MIB3 updates). Disconnect CarPlay first. Leave this USB connected through the update and test.
2. Use the same Engineering updater. Verify **E9400L / Y940** and **only MIB3OI_POST_SCRIPT**. Stop before starting if other components or verification errors appear.
3. Let the update finish, choose **Resume**, and allow normal mode to load. Never interrupt an active update or remove its USB.
4. Without another reboot, connect **wired CarPlay** through the other data port. Select the cluster navigation/map view and start an **Apple Maps** route while parked. Allow two minutes.
5. Note CarPlay connectivity, factory turn-by-turn and any cluster image. If it fails, do not repeat installation. After the updater is complete and the test ends, reboot normally and check factory functions.
6. Return the USB with **altscreen-setup.txt**, **boot-status.txt**, **altscreen-handler.log**, **network-status.txt**, and any **map-v7-*.plist** files. Keep everything private: addresses and device identifiers can appear. The logger runs about five minutes; factory journal output was unavailable in previous tests.

**Auxiliary response committed** means the endpoint was placed in the response, not that the phone connected. The next required event is an accepted auxiliary TCP connection. Subsequent config/authenticated-video/IDR/handoff/transmission events locate later stages. Only seeing a map proves cluster rendering.

For explicit recovery, add empty RESTORE_FACTORY_STARTUP.txt on this same v7 stick (no COLLECT_ONLY.txt), run the same single-component update, finish/Resume and reboot normally. It repeats the speech resets and retains assets. Do not mix versions or use R9900 packages. The handoff marker protocol remains the tested v6 protocol and refuses stale markers.

Keep the ZIP private because it includes a firmware-derived executable copy. No complete CPU/HMI firmware flash is included.
