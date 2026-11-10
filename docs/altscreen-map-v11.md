# MU0841 v11: corrected outgoing capture and advertised display endpoint

V10 authenticated3002 video frames and reported17682 successful UDP calls with no send errors, but no map. The packet observer opened successfully yet recorded no flows. Its protocol-specific ETH_P_IPV6 subscription missed outgoing taps; the previous loopback test accepted incoming copies. V11 uses ETH_P_ALL on the same interface and still filters to IPv6 UDP42800 metadata in userspace. Tests now require outgoing packets, including real multicast over an isolated VLAN5 fixture. Heartbeat/filter-stage counts will identify empty or filtered runs.

The v10 video path is retained. This is a corrected diagnostic comparison, not a confirmed map-display fix. Display snapshots discover the local displaymanagement service port from the root index instead of using forbidden gateway resource URLs. Snapshots occur at startup and approximately40/90seconds. Only GET requests are issued; no display configuration is changed.

Observer logs remain bounded, without promiscuous mode, frame/picture payloads or literal network addresses. It records RTP/TS identifiers, sequence/timestamp fields, source-port/tag, codec profile/level, counters and before/after-grant phases. Reports and derived source tags stay private. Capture must initialize before activation. Original executable remains unchanged on persistent storage; startup restores before RAM activation; no firewall, SMACK or coding changes. Separate v11 markers preserve older evidence.

Only for verified3G5035842B/H56/0841. The signed trigger repeats the four previously accepted speech resets. Recovery is narrow, not a complete vehicle backup.

## One parked diagnostic comparison

1. Use stable update power and the previous parked-update precautions. UNPLUG the iPhone. Keep this USB connected throughout the update and experiment.
2. Verify E9400L/Y940 and ONLY MIB3OI_POST_SCRIPT. Stop before starting if other components or verification errors appear. Let the update finish, choose Resume and allow normal mode to load. Never interrupt an active update or add another reboot before testing.
3. KEEP THE IPHONE UNPLUGGED. Show the built-in VW navigation map in the cluster for a FULL60SECONDS after normal mode returns. Confirm that the full VW map is visible. Do not plug in CarPlay after just a few seconds; this minute is the reference capture. If the stock map cannot be shown, end with a normal reboot and return the reports.
4. AFTER that full minute, connect WIRED CarPlay through the other data port. Select the cluster navigation view and keep an Apple Maps route active for TWO MINUTES without switching apps. A CarPlay map may remain absent; the primary purpose is to obtain a valid factory/custom comparison.
5. After updater completion and testing, reboot normally, check stock functions and return the USB. Complete steps3–4 promptly: the observer/logger lasts about five minutes from startup.

Expected private reports: altscreen-setup.txt, boot-status.txt, altscreen-handler.log, network-status.txt, output-diagnostics.txt, packet-audit.txt, packet-audit-errors.txt and map-v11-*.plist. Keep everything private. If capture cannot initialize, the bootstrap retains stock behavior and reports the error.

For explicit startup recovery, add empty RESTORE_FACTORY_STARTUP.txt on this same v11 stick (no COLLECT_ONLY.txt), run the same single-component update, finish/Resume and reboot normally. It repeats speech resets and retains assets. Do not mix versions or use R9900 recovery packages.

Keep the install ZIP private because it includes a firmware-derived executable copy. No full CPU/HMI firmware flash is included. V11 has not been validated in the vehicle.
