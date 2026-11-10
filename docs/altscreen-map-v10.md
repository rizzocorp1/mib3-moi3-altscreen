# MU0841 v10: working VW map versus CarPlay transport comparison

V9 authenticated2108 video frames and reported12607 successful UDP sends with no errors, but no cluster image. Grant validation, opening-GOP buffering and partial teardown worked. A successful UDP call is not proof of Ethernet delivery or decoder acceptance. You confirmed the native VW full map works in the cluster.

V10 binds the factory UDP source port42989 exclusively after authenticated encoder ownership is granted. It retains the v9 transport, security and buffering fixes. It requires a read-only packet observer on eth0.5 to be ready before activation. The observer records bounded RTP/TS metadata and counters, including source port, identifier, packet sizes and PAT/PMT fields. It records no map/frame bytes or literal IP addresses and does not enable promiscuous mode or change interface/firewall/SMACK settings. The comparison will establish whether native and custom packets reach the same outgoing interface and how their transport metadata differs.

Only for verified3G5035842B/H56/0841. Original executable remains unchanged on persistent storage, startup restores before RAM activation, and reboot ends the experiment. This is narrow recovery, not a full vehicle backup. The signed trigger repeats the four previously accepted speech resets. Separate v10 stage/logs/markers preserve older evidence. No vehicle map success is claimed.

## One parked comparison

1. Use stable update power and the previous parked-update precautions. Disconnect the iPhone. Keep the USB connected throughout the update and experiment.
2. Verify E9400L/Y940 and ONLY MIB3OI_POST_SCRIPT. Stop before starting if other components or verification errors appear. Let the updater finish, choose Resume and allow normal mode to load. Never interrupt an active update.
3. WITHOUT connecting the phone or adding another reboot, display the built-in VW navigation map in the instrument cluster for ONE MINUTE. Confirm it is visible. This supplies the known-working factory packet reference.
4. Then connect WIRED CarPlay through the other data port. Select the cluster navigation view and keep an Apple Maps route active for TWO MINUTES, without switching apps. Note any map image, updating picture or interruption. Do not repeat the installation after failure.
5. After updater completion and the test, reboot normally, check stock functions and return the USB. The observer/logger lasts approximately five minutes from startup, so perform steps3–4 promptly.

Expected private reports: altscreen-setup.txt, boot-status.txt, altscreen-handler.log, network-status.txt, output-diagnostics.txt, packet-audit.txt, packet-audit-errors.txt and map-v10-*.plist. Keep them private. If capture cannot initialize, the bootstrap must retain stock behavior and report the failure rather than activate another unobserved map test.

For explicit startup recovery, add empty RESTORE_FACTORY_STARTUP.txt to this same v10 stick (no COLLECT_ONLY.txt), run the same single-component update, finish/Resume and reboot normally. It repeats speech resets and retains assets. Do not mix versions or use R9900 recovery packages.

Keep the install ZIP private: it contains a firmware-derived executable copy. No full CPU/HMI firmware flash is included.
