# MU0841 security diagnostic v3 — one session

This USB identifies the security path used during a normal CarPlay connection. It will NOT display a map. It does not advertise AltScreen, start video receivers or sockets, capture keys, or stop/disable the native cluster encoder. A separate five-function observation library forwards the factory calls and logs path names and results only. Loading an experimental library is still unproven on the vehicle.

Only for the confirmed 3G5035842B / H56 / 0841 unit. The signed trigger repeats the four previously accepted speech-data resets. The installer does not overwrite the factory executable on disk. Its temporary startup restores the original startup bytes and metadata before loading the one-session RAM copy. An independent bounded logger must report ready before the test copy is mounted. V1/v2 assets remain on the unit, with a separate v3 staging path.

## One parked diagnostic check

1. Park and use the previously discussed update power precautions. VW specifies a charger supplying at least 70 A for MIB3 update operations. Disconnect CarPlay first; temporarily turn off iPhone Wi-Fi/Bluetooth so it cannot reconnect during the updater. Do not forget/delete the pairing.
2. Connect this USB and enter the same Engineering update process. Verify **E9400L / Y940** and **only MIB3OI_POST_SCRIPT**. If CPU/HMI/MCU or other components appear, or verification fails, stop before starting.
3. Start the single-component update. Keep USB connected, let the update finish, choose **Resume**, and allow normal mode to load. Never remove USB or restart during an active update.
4. Without an extra infotainment reboot, enable the phone's Wi-Fi/Bluetooth and connect wireless CarPlay. Open Apple Maps and start a route while parked. Leave it connected for about one minute. If convenient, also test wired CarPlay through the other data port for one minute while leaving this USB connected. No new cluster map is expected.
5. Keep USB connected for another 15 seconds for logs. After the updater is fully finished, perform a normal infotainment reboot to end the diagnostic session. Check normal CarPlay and turn-by-turn.
6. Once no updater is active, reconnect the stick to the computer. Keep **altscreen-setup.txt**, **boot-status.txt**, **security-probe.txt** if present, and **carplay-journal.txt**. Tell me whether you tested wireless, wired, or both. The logger runs for about five minutes.

Setup **DIAGNOSTIC ARMED** means installed, not running. Boot **DIAGNOSTIC ACTIVE** confirms the observation copy was mounted; the native encoder is unchanged. Probe **LOADED_observation_only** confirms the library loaded. **session_HomeKit**, **screen_ChaCha**, **session_AES**, **screen_AES**, and **CBC_decrypt** identify observed calls; a present context alone does not prove that every stream uses that path. **ABORT** means bring back the logs; do not repeat installation.

## Recovery

The startup restores itself before the diagnostic mount, and reboot removes that mount. For explicit startup recovery, create an empty **RESTORE_FACTORY_STARTUP.txt** on this same v3 USB, with no COLLECT_ONLY.txt, and use the same single-component update. Finish normally, Resume and restart normally. This repeats the known speech resets and retains backup assets. Do not mix v1, v2 or v3 files or use R9900 recovery packages.

Keep this ZIP and all logs private: the archive contains a firmware-derived executable copy. This is a narrow startup recovery mechanism, not a complete vehicle backup.
