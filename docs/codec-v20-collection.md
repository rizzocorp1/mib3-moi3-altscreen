# Retrieve V20's missing reports

This stick collects the retained V20 boot/playback reports and service status.
It does NOT arm or launch another playback test, change coding, or install a
new startup script. It removes the old playback trigger from the USB.

While parked, insert the stick and run ONLY MIB3OI_POST_SCRIPT at E9400L/Y940.
Complete the updater and Resume with USB connected. There is no remove/reinsert
trigger and no need to connect CarPlay for this collection. Once the updater
is finished, return the USB to the PC.

Expected output: codec-v20-intake.txt and, if present on the unit,
boot-status-v20.txt, codec-player-v20.log and codec-v20-journal.txt. These will
show whether startup ran, whether its one-shot ticket was consumed, and whether
systemd rejected the monitor or the USB trigger was missed. No codec conclusion
can be drawn from seeing only the factory map in the previous test.

The collection script reads unit-side files/status only. The carrier's previously
accepted signed-trigger speech resets still apply; this is not a full rollback
mechanism. No further playback test is requested by this package. Keep the
reports private; they are not uploaded or published automatically.
