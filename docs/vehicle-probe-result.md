# First vehicle USB execution result — 2026-11-06

The user supplied moi3-usb-probe.txt after running the engineering-container probe on the Atlas. It reports Effective UID: 0 and RESULT: PROBE COMPLETED. This proves the USB script entry point executes with root privileges on this unit. It does not prove map display, backup or rollback.

The reported SHA-256 values exactly match the files extracted from the downloaded MU0841 archive:

- CarPlay executable: d9c06c9f0aff2a098ec9da09a63d4339de40e4f57e7f46a173706ccd04d15e2b
- carplay.sh: aca72269589a14c38ceef28bd8d1a316964b7baf3490365ea2b85ea2528c7325
- os-release: 97d0b16193a2ef43a0e10138127e05376644ec36be4d199b9c059032eee89284

Available tools: /bin/tar, /usr/bin/sha256sum and /usr/bin/chsmack. readelf, getfattr and getfacl are absent from PATH. The image's /bin/tar is a symlink to the BusyBox applet. The current stock-snapshot.sh requires GNU tar with xattr/ACL preservation and is therefore not ready for this unit. Do not weaken its metadata requirements or substitute a content-only backup.

The old head -2 invocation is unsupported by this BusyBox 1.24.1 and obscured the tar version output. It has been changed to head -n 2; the snapshot helper's analogous head -1 has also been corrected. The probe completed despite this display-only error.

All listed payloads were absent, as intended. No AltScreen installer, launcher or snapshot tool was run. The report cannot confirm the user-visible effects of the engineering container's earlier speech-file deletions.

Next implementation work: provide a target-compatible backup mechanism preserving ownership, modes and security xattrs; validate its restore behavior before changing startup files. The one-shot launcher and native cluster-channel handoff also remain unfinished. Do not request another identical entry-point probe: that question is now answered.
