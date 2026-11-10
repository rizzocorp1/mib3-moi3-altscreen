# V24 retained-report recovery only

The returned map handler/audit/plist files contained only zero bytes while all
installed USB payload hashes remained correct. This does not establish failed
USB hardware or prove that the on-unit source is zero-filled. Recover retained
source reports before another map experiment. This collector independently reads
bounded original files, counts zero bytes, hashes them, compresses/encodes them
as ASCII, fsyncs the USB bundle and verifies byte readback. No executable or
service is launched/stopped, no startup/configuration file is edited, and no
source report is deleted. New image capture is not enabled.

Park, insert this USB and update ONLY MIB3OI_POST_SCRIPT at E9400L/Y940. Keep USB
connected through the updater reboot and complete Resume/normal mode. After the
update is fully finished, leave it connected for ten more seconds, then bring it
back. No CarPlay connection, drive, counter pattern, unplug/reinsert trigger or
extra manual reboot is needed. Do not repeat the update if a report is absent.

Outputs: v24-recovery-intake.txt and v24-recovered-reports.json. Preserve both.
The JSON is a private log bundle; do not publish it. The same signed carrier has
its previously accepted speech-file resets; collection is not complete rollback.
