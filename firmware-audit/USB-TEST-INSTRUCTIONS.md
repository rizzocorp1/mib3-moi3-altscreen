# EXPERIMENTAL: MOI3 USB trigger probe

This is the first USB execution test, NOT a CarPlay map installer. It includes the engineering container referenced by the original researcher. It has not been tested on your 3G5035842B / H56 / 0841 unit.

## Known change to the unit

Before our probe runs, the original container removes speech dynamic vocabulary, two speech-language configuration files and a wake-word data file. Our probe cannot back them up before deletion. Do not use this experiment if you require a guaranteed exact stock rollback before any modification. The original root ksend command is replaced and will not run. No AltScreen payload is installed.

## Preparing the stick

Use an empty FAT32 USB stick. Extract the ZIP contents directly to its root so `Data`, `Meta`, `fix_hw_partnumber.sh` and `PROBE_ONLY.txt` are at the top level. Do not mix it with any other update, activation files or older AltScreen payloads. Do not use ENABLE_ALTSCREEN.txt or REVERT.txt.

Before starting, record the current System Information screen and confirm it still says 3G5035842B / H56 / 0841. A different identity requires review. Keep the vehicle stationary with a stable power supply throughout the updater operation; VW's official MIB3 update bulletin specifies a 70A battery charger for updates. A completed on-device backup and recovery method have NOT yet been established for this engineering package.

## What the researcher describes

The original article uses the red engineering/service update menu to select the USB update and run MIB3OI_POST_SCRIPT, followed by Resume. This is not plug-and-play: insertion alone does not install or run the probe. We have not independently verified that menu sequence on this Atlas. Review the package identity/components on the unit before starting: the package is E9400L/Y940 with only MIB3OI_POST_SCRIPT. If CPU, HMI, MCU or another firmware component is offered, or any verification error appears, stop and do not force it.

Do not remove power or the USB during an active update. Do not interrupt a running updater to obtain logs. Follow its completion prompts only after it finishes. The probe does not request a reboot or alter startup scripts.

## Collecting results

After the updater has completed, inspect `moi3-usb-probe.txt` on the stick. `RESULT: PROBE COMPLETED` means our script executed, not that AltScreen works. Missing tools/payloads are inventory findings, not instructions to install anything. Sysinfo_SUCCESS.txt and getversion_SUCCESS.txt may also appear if the factory TNR hook runs. If there is no report, record the updater's exact summary/error rather than repeatedly retrying.

Keep all reports private. They may include unit/software details. Return the observed update result and probe report for the next compatibility step. Do not run the original FAZIT/ksend script or any unreviewed activation package.
