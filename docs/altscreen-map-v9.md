# MU0841 v9: authenticated video handoff and factory transport profile

V8 reached the auxiliary TCP connection, parsed video configuration and authenticated a valid H264 IDR. The root collector reported encoder grant creation. No frame-forwarding event was logged. Stream restarts may include switching map apps; the old teardown hook incorrectly stopped auxiliary reception on every session teardown call, including partial primary/audio teardown. Root grant visibility and the exact timing of app transitions remain unverified.

V9 preserves the auxiliary receiver and retained pairing during partial primary/audio teardown, and stops it for auxiliary or full session teardown. A bounded RAM queue retains an authenticated opening GOP while root creates the output grant. The receiver checks that grant every 100 ms while waiting for input, so a second keyframe is unnecessary. Authentication failures, config changes and teardown discard buffered video. Overflow discards dependent frames and waits for a new IDR. No plaintext video is written to disk.

The matching MU0841 factory TransportStream implementation uses program 1, transport-stream ID 100, PMT PID 1000, video/PCR PID 17, RTP identifier 100000, a monotonic millisecond RTP clock and six TS slots per 1140-byte datagram. V9 follows this profile, pads unused slots with null TS packets, retains 90 kHz PES/PCR timing, and reports actual send success/errors. These static matches do not prove hardware rendering.

The allowed auxiliary TCP range 49200..49499 is retained. No firewall or SMACK rule is changed. Initial acknowledgement, HomeKit security and recovery-first RAM mounting are retained. The v9 stage/log/marker namespace is separate; old v8 assets and shared v6 markers are preserved rather than deleted or trusted.

Only for verified 3G5035842B / H56 / 0841. The signed trigger repeats the four previously accepted speech resets. Original startup bytes and metadata are restored before activation; the original executable is unchanged on disk. Recovery remains narrow, not a complete vehicle backup.

## One parked test

1. Use stable update power and the prior parked-update precautions (VW specifies at least a 70 A charger for MIB3 updates). Disconnect CarPlay and leave this USB connected.
2. In Engineering update, verify E9400L / Y940 and ONLY MIB3OI_POST_SCRIPT. Stop before starting if any other component or verification error appears.
3. Let the update finish, choose Resume, and allow normal mode to load. Do not interrupt an active update or remove its USB. Do not add another reboot before testing.
4. Connect WIRED CarPlay through the other data port. Select the cluster navigation view and start an Apple Maps route. Keep this app active for the first two minutes, without switching apps, so stream lifecycle and output can be assessed consistently.
5. If an image appears, note whether it updates. Other apps can then be tested separately. Note connectivity, turn-by-turn, any image, and any visible restart. Do not repeat installation after failure.
6. After the updater is complete and testing ends, reboot normally and check factory functions. Return the USB with altscreen-setup.txt, boot-status.txt, altscreen-handler.log, network-status.txt, output-diagnostics.txt and map-v9-*.plist. The logger runs approximately five minutes. Keep all reports private; identifiers, addresses and display configuration may be present.

For explicit startup recovery, add empty RESTORE_FACTORY_STARTUP.txt to this same v9 stick (no COLLECT_ONLY.txt), run the same single-component update, finish/Resume and reboot normally. It repeats speech resets and retains assets. Do not mix versions or use R9900 recovery packages.

A TCP connection or authenticated frame is not a rendered map. Required later evidence is grant validation, buffered release, successful UDP sends and finally visible cluster output. V9 is an experiment, not a confirmed solution. Keep the install ZIP private because it contains a firmware-derived executable copy.
