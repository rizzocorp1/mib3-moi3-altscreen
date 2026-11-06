# USB trigger rehearsal

The root `deploy-usb/fix_hw_partnumber.sh` is now a probe, not an AltScreen installer. The original author's exact script is preserved as `deploy-usb/upstream-reference/fix_hw_partnumber.sh.txt` at upstream commit 455658b005d1544ab1f6d6e5c42f529c0381235f. The text extension prevents accidental execution as a USB entry point. Core CarPlay code and binaries are unchanged.

## What can be tested first

1. On a computer, run `sh recovery/test-usb-entry.sh` and `sh recovery/test-snapshot.sh` against disposable mock files.
2. Prepare the probe script and a `PROBE_ONLY.txt` marker at USB root. Do not include `ENABLE_ALTSCREEN.txt` or `REVERT.txt`. The script only inventories paths/tools, hashes selected existing inputs and writes `moi3-usb-probe.txt` onto the USB. It does not install hooks, stop/start services, mount anything, or invoke the upstream script.
3. Only once a separately verified update-trigger mechanism is available for this exact unit, use it to reach the probe. A USB stick containing a shell script is not itself executable by stock MOI3. A report ending `RESULT: PROBE COMPLETED` confirms the entry point was reached. It does not validate AltScreen or the full update package.
4. Review the local report, confirm the exact unit/software identity, then validate backup metadata support before any actual installation.

## Current blocker to testing on the car

The upstream deployment README describes an engineering update container named `LG-wipe-FAZIT.zip` / `MOI3_EU_SE_E9400L`. We have not obtained, authenticated, or inspected that container, and have not established its compatibility or side effects on this Atlas MU0841. The updater/container runs BEFORE our script and can make changes of its own. The probe's read-only behavior does not make that surrounding update process read-only. Do not substitute a random engineering firmware archive or the full EU 0841 update.

No complete on-car USB package is supplied in this PR. No installation command is enabled yet. Actual snapshot creation should happen with writers stopped using a verified platform-specific procedure; taking a live changing backup may fail or capture inconsistent data. The backup/restore tool is available separately, not automatically called by this probe.

Keep reports and actual snapshots offline. They can contain software fingerprints, personal pairing data and proprietary files; the public repository should contain only code/tests and generic documentation.
