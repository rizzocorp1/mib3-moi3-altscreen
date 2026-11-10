# Experimental Atlas MU0841 AltScreen test — one session

This is the first map-display attempt, not a proven product. Use only on the confirmed 3G5035842B / H56 / 0841 unit. The USB execution probe matched this unit's CarPlay executable, startup script and OS identification hashes to the extracted firmware.

The engineering container still deletes the four previously reviewed speech-data paths before our script runs. You accepted that tradeoff. Those earlier deletions are not covered by this recovery mechanism.

## What this test changes

The installer preserves the exact carplay.sh bytes, ownership, modes, ACLs and extended attributes in an on-unit backup, plus a byte copy on USB. It commits a temporary startup script atomically. On the next boot that script restores the factory startup file before consuming its one-use ticket or activating the experiment.

The factory CarPlay executable is never overwritten on disk. A copy with an explicit dependency on our handler is prepared in RAM and bind-mounted for this session. The native multi-display video encoder is stopped for this session so it does not compete with the test stream. Its service is not disabled; reboot starts the normal factory path. The copy and handler remain experimental and may fail or cause an infotainment reboot. The recovery-first ordering and failure paths have host tests; SMACK behavior and the full sequence have not been validated on this car.

This is a narrow startup-file backup, not a complete vehicle/partition image. Test assets and logs remain in a private staging directory for review. Phone pairing and voice data are not restored by it.

## Prepare the USB

1. Save the previous probe report to your computer. Use an empty FAT32 stick, then extract the test ZIP directly to its root.
2. Confirm Data, Meta, payload, fix_hw_partnumber.sh, ALTSCREEN_TEST_ONCE.txt, READ-ME-FIRST.txt and build-manifest.json are present. No PROBE_ONLY.txt, SUMMARY_PROBE_ONLY.txt, REVERT.txt or old payload files should remain.
3. Keep this locally built package private: its CarPlay copy is derived from proprietary firmware. Public source/build tools are in the fork's draft PR; firmware and vehicle backups are not published.

## Run the parked test

1. Park the car. Use stable power; VW's MIB3 update bulletin specifies a charger supplying at least 70 A for update operations. Do not perform this experiment while driving.
2. Enter Service Mode by holding MENU, choose Software Update/Versions, then the initial Start Update confirmation to enter Engineering.
3. Choose Update, then the USB port holding the stick. Verify E9400L/Y940 and only MIB3OI_POST_SCRIPT. If CPU, HMI, MCU or another component appears, or verification fails, stop before starting; do not force it.
4. Start the update, confirm that same single component, and allow it to finish. Do not remove the stick or interrupt an active updater. At successful completion use Resume and allow normal mode to load.
5. Leave the stick connected for logs. Select the cluster's normal navigation/map view, connect the iPhone through CarPlay (wirelessly or through another data-capable port), and start an Apple Maps route while parked. Allow about a minute for negotiation. Successful map display is not guaranteed; a blank map area is an experimental failure, not a reason to repeat installation or drive the vehicle.
6. End the test with a normal infotainment reboot, using the power-button restart until the unit restarts. The factory startup script has already been restored, the bind mount disappears with reboot, and the factory encoder starts normally. Check that normal CarPlay and the factory cluster view return.
7. Once no updater is active, bring back altscreen-setup.txt, boot-status.txt, altscreen-handler.log and carplay-journal.txt from the USB. Some files may be absent if execution stopped early. Keep the reports private. RESULT: ARMED confirms setup, while ACTIVE in the boot report confirms the session was mounted; neither alone proves maps appeared.

## If setup fails or you want explicit restoration

Do not rerun activation. Preserve the USB report and leave all backup assets intact. If the bootstrap was committed but not armed, its next execution attempts to restore the factory startup and does not mount the test executable. Failure paths return success to ExecStartPre to avoid creating a test-induced startup failure loop, but this has not been proven on hardware.

For explicit recovery, create an empty RESTORE_FACTORY_STARTUP.txt on the same test USB, with no COLLECT_ONLY.txt. Run the same single-component engineering-container process. The recovery branch verifies baseline/backup/helper hashes, restores the startup file with its metadata and removes the activation ticket. It removes a recognized test bind mount if present. Finish the updater normally and Resume/reboot. This repeats the known speech resets; it does not flash stock CPU/HMI firmware or erase test backups.

For log collection only, use an empty COLLECT_ONLY.txt instead, with no restoration marker. That branch copies available logs and never arms a test. It still uses the engineering container and therefore repeats its known speech resets. Do not use recovery packages R9900 or the original FAZIT/ksend script.
