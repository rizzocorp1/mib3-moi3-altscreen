# Stock snapshot and rollback (experimental, installation disabled)

This tool implements a scoped filesystem backup/restore mechanism. It is NOT a full flash image, bootloader recovery system, automatic watchdog or a guarantee of a complete stock vehicle. Nothing here has been tested on MU0841 hardware.

The original upstream installer must not be used for this branch. Its generated factory script is not an authentic backup. Our entry point refuses deployment until an on-device backup, verified execution method and recovery procedure are established.

## Coverage

Snapshots preserve the existing CarPlay directory, smartlink persistent data (including pairing secrets), the added hook library path and the reboot helper. GNU tar captures contents, links, permissions, numeric owners, ACLs and all extended attributes, including SMACK labels when supported by the actual filesystem and tool. Absence is recorded so added files can be removed during restore. SHA-256 checks detect corruption, not malicious replacement; store backups on trusted offline media and keep a second copy.

Firmware flashing, engineering-package side effects, other RSI configuration, coding/adaptations, certificates outside these directories, MICOM, EEPROM/NVRAM, boot partitions and storage failure are outside coverage. Stock status must be established BEFORE taking the baseline. A backup of an already-modified unit is not stock. Backing up files does not give us a way to execute recovery if the unit cannot boot.

## Preconditions before vehicle use

- Validate GNU tar, SHA-256, numeric ownership, symlinks, ACL/xattr and SMACK round trips on the exact target. No unsupported-option fallback is permitted.
- Confirm the unit identity and exact software train. The tool checks machine-id and os-release, which do NOT independently establish exact infotainment firmware compatibility.
- Stop writers, CarPlay services and competing native-video producer through a verified platform procedure. Detach experimental bind mounts. Keep the head unit powered and retain independent access.
- Provide trusted offline storage with enough free space. Never commit backups or diagnostic identities to GitHub, including private GitHub.

## Interface (not yet an approved on-car procedure)

`stock-snapshot.sh backup ROOT ABSOLUTE_BACKUP_DIRECTORY` creates a new baseline and refuses to overwrite it. Failed backups never receive a COMPLETE marker. `verify` checks integrity and unit/OS identity without changing the unit. `restore` verifies before mutations, stages the originals, moves altered scopes into adjacent quarantines, restores the recorded scopes and compares metadata/content. It requires explicit service-stop and firmware-match assertions. It records progress and refuses to retry an interrupted restore blindly. It never restarts services or reboots the unit.

Restoring multiple directories is not one atomic transaction. An interruption requires a technician/developer to inspect the journal and retained original/modified trees. After verification, quarantines and experiment-only state still require an audited cleanup procedure; therefore the tool does not print a claim of complete factory restoration.

## Next engineering step

Prefer a RAM-only experiment with no replacement of factory startup scripts: loss of power removes the experimental bind mounts. That still requires verified code execution, service handoff and on-device metadata validation. A functional monitor can request rollback only after these primitives are proven; a userspace monitor cannot rescue a head unit that cannot boot.

## Validation performed

`test-snapshot.sh` passes on a temporary Windows/Git Bash mock root in explicit content-only mode. It checks original contents, removal of added files, preservation of modified trees in quarantine, refusal to overwrite a snapshot, restore preconditions and detection of archive corruption. This does not validate Linux ownership, ACLs, SMACK, interrupted on-device recovery, or the real unit's command availability. Metadata warnings are fatal in production mode.
