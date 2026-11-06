# USB trigger rehearsal

The root `deploy-usb/fix_hw_partnumber.sh` is now a probe, not an AltScreen installer. The original author's exact script is preserved as `deploy-usb/upstream-reference/fix_hw_partnumber.sh.txt` at upstream commit 455658b005d1544ab1f6d6e5c42f529c0381235f. The text extension prevents accidental execution as a USB entry point. Core CarPlay code and binaries are unchanged.

## What can be tested first

1. On a computer, run `sh recovery/test-usb-entry.sh` and `sh recovery/test-snapshot.sh` against disposable mock files.
2. Prepare the probe script and a `PROBE_ONLY.txt` marker at USB root. Do not include `ENABLE_ALTSCREEN.txt` or `REVERT.txt`. The script only inventories paths/tools, hashes selected existing inputs and writes `moi3-usb-probe.txt` onto the USB. It does not install hooks, stop/start services, mount anything, or invoke the upstream script.
3. Only once a separately verified update-trigger mechanism is available for this exact unit, use it to reach the probe. A USB stick containing a shell script is not itself executable by stock MOI3. A report ending `RESULT: PROBE COMPLETED` confirms the entry point was reached. It does not validate AltScreen or the full update package.
4. Review the local report, confirm the exact unit/software identity, then validate backup metadata support before any actual installation.

## Engineering container inspection and experimental bundle

The upstream deployment README describes `LG-wipe-FAZIT.zip` / `MOI3_EU_SE_E9400L`. We obtained the 5,858-byte archive linked in James11123's original article and inspected every file. Five checksum relationships match (JSON manifests require whitespace-free serialization). Our experimental local bundle preserves Data/Meta exactly and replaces only the root ksend script with the tested probe. See `../firmware-audit/CONTAINER-AUDIT.md`, the builder and verifier. The vendor signature has not been authenticated with a trusted key, and acceptance on this Atlas MU0841 remains untested.

The original container deletes four speech-related paths BEFORE our script executes. This is not a read-only updater experiment and our scoped backup cannot restore pre-update speech data it never captured. Review the audit and USB test instructions before considering the experiment. No AltScreen installation command is enabled. The proprietary container is kept local; the PR supplies a reproducible builder/verifier rather than redistributing it. Actual snapshot creation should happen with writers stopped using a verified platform-specific procedure. The backup/restore tool is separate and is not automatically called by this probe.

Keep reports and actual snapshots offline. They can contain software fingerprints, personal pairing data and proprietary files; the public repository should contain only code/tests and generic documentation.
