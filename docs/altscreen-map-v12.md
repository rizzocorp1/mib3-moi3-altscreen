# MU0841 v12: bounded auxiliary keyframe recovery experiment

V11 produced the first user-confirmed Google Maps image in the cluster, but it remained frozen while apps/routes were changed. The sender continued:2807 authenticated access units,14923 observed outgoing UDP packets, no observed sequence gaps. Only one IDR keyframe appeared during roughly2.5minutes. Working VW video used Baseline profile66; phone video used High profile100. That difference is not proof of incompatibility, because an actual custom picture was displayed. App switching also prevents isolating stale UI from decoder/reference-picture recovery.

V12 retains the v11 video path and adds only a bounded auxiliary refresh request. After authenticated encoder handoff, it requests forceKeyFrame for the advertised auxiliary UUID at most once per two seconds, capped at90 attempts per stream. It uses the matching factory session API and public dispatch queue. Session/request objects are retained while asynchronous work is queued; stale/security/teardown jobs are canceled. It never uses the untargeted primary-screen helper. Missing API, allocation or native submission errors retain the media path. Submission success is not a phone acknowledgement: increased IDR counts and visible updates are the acceptance evidence.

This experiment does not re-encode video, change codec labels, force a navigation app, or implement app-switch control. Keep one app active so motion can be assessed independently. The observer and read-only runtime configuration snapshots are retained. No picture/frame data is saved. No firewall, SMACK or coding changes. Separate v12 stage/log/markers preserve earlier evidence. Original executable remains unchanged on persistent storage; startup restores before RAM activation; reboot ends the experiment.

Only for verified3G5035842B/H56/0841. Signed-trigger speech resets remain accepted; recovery is narrow, not a complete vehicle backup. Motion and complete map behavior remain unproven.

## One parked motion test

1. Use stable update power and prior parked-update precautions. Unplug the iPhone; keep this USB connected.
2. Verify E9400L/Y940 and ONLY MIB3OI_POST_SCRIPT. Stop before starting if other components or verification errors appear. Let the updater finish, choose Resume and return to normal mode. Do not interrupt an active update or add another reboot before testing.
3. Keep the phone cable unplugged while the VW full map is visible in the cluster for60seconds.
4. Connect WIRED CarPlay through the other data port. Open GOOGLE MAPS ONLY and keep the cluster navigation view selected. Start a route while parked. Over the next two minutes, note whether the cluster changes from its initial preview, shows the route, updates labels or recovers from a freeze. Cancel and start a different route once if needed to create an observable change. Do not switch to Apple Maps or Waze during this first test; do not drive for the test.
5. After updater completion and testing, reboot normally, check factory functions and return the USB. Perform steps3–4 promptly within the roughly five-minute logging window.

Keep all reports private: altscreen-setup.txt,boot-status.txt,altscreen-handler.log,network-status.txt,output-diagnostics.txt,packet-audit.txt,packet-audit-errors.txt,map-v12-*.plist. New AUX_REFRESH records describe native command submission, while IDR and output counters describe actual returned video. If no image/motion appears, do not repeat installation.

The brief old frame seen during reboot is consistent with the cluster retaining its last decoded picture until stock video resumes; it does not by itself establish persistent modification. For explicit startup recovery, add empty RESTORE_FACTORY_STARTUP.txt on this same v12 stick (no COLLECT_ONLY.txt), run the same single-component update, finish/Resume and reboot normally. It repeats speech resets and retains assets. Do not mix versions or use R9900 recovery packages.

Keep the install ZIP private because it includes a firmware-derived executable copy. No full CPU/HMI firmware flash is included.
