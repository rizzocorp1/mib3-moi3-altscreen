# MU0841 v13: session-bound refresh and native-map restoration

V12 displayed all three map apps and updated about every two seconds. Its90-request budget was exhausted after roughly three minutes while P-frame traffic continued. V13 removes that fixed request cutoff. Requests remain auxiliary-UUID scoped, coalesced and limited to one per two seconds while the authenticated session/test lease is active. This remains a keyframe-based workaround, not smooth interframe decoding.

On auxiliary disconnect the writer closes its UDP socket BEFORE withdrawing its ownership request. The root monitor then revokes its grant, verifies UDP42989 is free using a real exclusive bind probe, and starts the factory encoder. Quick reconnects can request a fresh grant. A still-busy port prevents native restart, avoiding competing senders. Failed native starts are logged rather than retried repeatedly.

The experiment is bounded to ABOUT TEN MINUTES from startup. At expiry or normal monitor termination, a readable expiry token stops further custom output/refresh, the grant is withdrawn, and native restart waits for port release. This replaces the silent three-minute refresh freeze with explicit test expiry and attempted restoration. On-car disconnect/reconnect/expiry behavior remains to be validated. If restoration cannot be confirmed, use normal reboot.

The video format,1280x480 canvas and current layout remain unchanged. Your photos are private. Safe-area clipping will be addressed separately; this version does not fix it. No firewall, SMACK, coding, factory executable or native service definition is modified. Startup restores before one-shot RAM activation. Separate v13 assets/markers preserve old evidence. Known signed-trigger speech resets remain; recovery is narrow, not a full vehicle backup.

## One parked lifecycle test

1. Use stable update power and prior parked-update precautions. Unplug the iPhone; keep this USB connected.
2. Verify E9400L/Y940 and ONLY MIB3OI_POST_SCRIPT. Stop before starting if other components or verification errors appear. Complete the update, choose Resume and allow normal mode. Never interrupt an active update or add another reboot before testing.
3. Confirm the full VW map is visible in the cluster for about30seconds with CarPlay disconnected.
4. Connect WIRED CarPlay through the other data port. Start a Google Maps route and keep it active for AT LEAST FOUR MINUTES. Confirm updates continue beyond the old three-minute cutoff. Stay parked; cancel/start another route if needed to create an observable change.
5. Then try Apple Maps and Waze separately, about30seconds each. Note which app is on the main screen and cluster and whether the cluster still changes. Two-second steps remain expected; smooth motion is not claimed.
6. Disconnect CarPlay completely and check whether the VW map returns within about10seconds. Unplugging the cable may leave wireless CarPlay active; confirm the CarPlay connection has actually ended before judging restoration. Keep the USB connected.
7. If the VW map returns, reconnect CarPlay once and confirm the cluster can display the phone map again. Finish this within the approximately ten-minute test lease. At lease expiry the monitor deliberately attempts to return to native output.
8. After the updater is complete and testing ends, reboot normally, check factory functions and return the USB. If the factory map fails to return, the UI becomes unstable or restoration errors appear, end with normal reboot rather than repeating installation.

Keep private reports:altscreen-setup.txt,boot-status.txt,altscreen-handler.log,network-status.txt,output-diagnostics.txt,packet-audit.txt,packet-audit-errors.txt,map-v13-*.plist. LEASE EXPIRED means the bounded test ended. NATIVE RESTORED confirms the service was active after a free-port probe, not that a visible map was independently verified. Port-busy/restore-failed records require normal reboot and review.

For explicit startup recovery, add empty RESTORE_FACTORY_STARTUP.txt on this same v13 stick (no COLLECT_ONLY.txt), run the same single-component update, finish/Resume and reboot normally. It repeats speech resets and retains assets. Do not mix versions or use R9900 recovery packages. Keep the install ZIP private: it includes a firmware-derived executable copy. No complete firmware flash is included.
