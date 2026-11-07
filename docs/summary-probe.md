# Container-free summary-hook probe

This is a USB execution experiment, not the map-display installer. It contains no Data or Meta directory, firmware image, signed post-install script, AltScreen binary or activation marker. Do not combine it with an update stick or either recovery package.

## Evidence and limit

In the inspected MU0841 updater, SummaryScreen::initScreen checks for /tmp/USB/PORT1/PART1/fix_hw_partnumber.sh, then port 2, and invokes it through UpdateHandler::run_cmd. That helper spawns /bin/sh -c and waits for completion. The call is independent of the engineering container's four speech deletions.

This does not prove that an update summary screen can be reached from your car's current menu state without installing anything. Ordinary update logs use a different screen and are not evidence that the maintenance hook ran. We have not established a general no-update navigation sequence. Do not start an update to reach the screen.

## Preparing and checking

1. Use an empty FAT32 USB stick. Extract moi3-summary-hook-probe.zip directly to its root. It should contain only fix_hw_partnumber.sh, SUMMARY_PROBE_ONLY.txt and README.txt.
2. Keep the car parked. Confirm System Information still reports 3G5035842B / H56 / 0841.
3. With no update in progress, insert the stick and inspect the existing red engineering/update menu. If an existing summary/result page is directly available, opening it may run the probe. Do not select Start update, recovery, emergency update, a package, a component or a verification override. Do not interrupt or cancel an active update for this test.
4. If there is no directly accessible summary/result page, stop this experiment and record the visible menu labels. No update should be started or forced. Insertion alone is not expected to run the script.
5. After normal menu activity finishes, check the USB for summary-probe-report.txt. Its result line proves the hook ran; it does not prove map display works. No report is an inconclusive menu-path result, not a reason to retry a firmware update.

The probe writes only its USB report and reads hashes, labels, tool availability, mount information and the factory service unit. The updater itself may record normal menu/state activity. No complete vehicle rollback guarantee is implied. Keep the report private; it contains device configuration details. Return the report or the observed menu labels for the next step.
