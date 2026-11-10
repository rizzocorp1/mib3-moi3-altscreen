# MU0841 map attempt v8 — permitted TCP endpoint

V8 preserved working CarPlay and factory turn-by-turn, but the phone never connected to the auxiliary TCP listener. No video authentication or encoder handoff occurred. The read-only IPv6 firewall snapshot permits TCP 49200..49499 in both directions; it does not permit the v8 ephemeral endpoint or the earlier 6031 endpoint through its generic CarPlay rules. Packet arrival was not captured, so firewall rejection is a supported hypothesis rather than an observed dropped SYN.

V8 selects an unused TCP port within that existing permitted range, skips ports already owned by factory services, and fails the auxiliary setup if the range is exhausted. It still advertises the actual bound port. It changes no firewall or SMACK rules. Endpoint schema, security and video handoff behavior otherwise remain the same. This is a transport experiment, not a confirmed map solution.

Initial SETUP acknowledgement, HomeKit frame authentication and deferred encoder handoff are retained. Native encoding stays running until authenticated IDR video passes validation. Test output waits for a metadata-preserving grant and an IDR; failed handoff restarts native encoding. Reboot ends the one-session test. No native service is disabled/edited and no firewall rule is changed.

Only for verified 3G5035842B / H56 / 0841. The signed trigger repeats the four accepted speech resets. Startup restores original bytes/metadata before mounting the RAM test copy; the original executable is unchanged on disk. This is a narrow recovery mechanism, not a full vehicle backup. Hardware success remains unproven.

## One parked wired test

1. Park with stable update power using prior precautions (VW specifies at least a 70 A charger for MIB3 updates). Disconnect CarPlay first. Leave this USB connected through the update and test.
2. Use the same Engineering updater. Verify **E9400L / Y940** and **only MIB3OI_POST_SCRIPT**. Stop before starting if other components or verification errors appear.
3. Let the update finish, choose **Resume**, and allow normal mode to load. Never interrupt an active update or remove its USB.
4. Without another reboot, connect **wired CarPlay** through the other data port. Select the cluster navigation/map view and start an **Apple Maps** route while parked. Allow two minutes.
5. Note CarPlay connectivity, factory turn-by-turn and any cluster image. If it fails, do not repeat installation. After the updater is complete and the test ends, reboot normally and check factory functions.
6. Return the USB with **altscreen-setup.txt**, **boot-status.txt**, **altscreen-handler.log**, **network-status.txt**, and any **map-v8-*.plist** files. Keep everything private: addresses and device identifiers can appear. The logger runs about five minutes; factory journal output was unavailable in previous tests.

**Auxiliary response committed** means the endpoint was placed in the response, not that the phone connected. The next required event is an accepted auxiliary TCP connection. Subsequent config/authenticated-video/IDR/handoff/transmission events locate later stages. Only seeing a map proves cluster rendering.

For explicit recovery, add empty RESTORE_FACTORY_STARTUP.txt on this same v8 stick (no COLLECT_ONLY.txt), run the same single-component update, finish/Resume and reboot normally. It repeats the speech resets and retains assets. Do not mix versions or use R9900 packages. The handoff marker protocol remains the tested v6 protocol and refuses stale markers.

Keep the ZIP private because it includes a firmware-derived executable copy. No complete CPU/HMI firmware flash is included.
