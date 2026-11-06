# Atlas MU0841 experimental test status
> RETIRED AFTER VEHICLE FAILURE (2026-11-06): The v1 package below did not produce cluster maps and temporarily disrupted CarPlay. Explicit startup recovery completed; factory turn-by-turn verification is pending. Do not deploy this ZIP again. Host tests below did not establish successful vehicle operation. Replacement logging and feature-preservation changes are in development.


The first engineering-container vehicle probe completed as root. The installed CarPlay executable, startup script and OS-release hashes exactly match the extracted MU0841 baseline. The user accepted the known four speech-data resets. No repeated entry-point-only probe is required.

A local-only one-session AltScreen test package is built: moi3-altscreen-once-0841-EXPERIMENTAL.zip, SHA-256 b95dc78e908a893d724008adf320c4982700229223294196694adb73d5c5d6cd. It includes a firmware-derived CarPlay copy and must not be published. Its Data/Meta bytes match the audited engineering container, it contains only MIB3OI_POST_SCRIPT, all seven declared payload hashes verify, and its generated scripts match the current templates.

The handler is built against the matching factory glibc 2.23 libraries and uses incoming screen NTP intervals with monotonic fallback. A static AArch64 helper preserves Linux ownership, modes, ACLs and extended attributes for the narrow startup-file backup and atomic replacement. This avoids requiring GNU tar for the new installer; the older whole-scope snapshot prototype still requires GNU tar and is not used here.

The test copies the executable, adds an explicit handler dependency without instruction patches, and prepares a RAM copy for a one-session bind mount. The original executable is never overwritten on disk. The bootstrap restores the original startup script before consuming the activation ticket or mounting the test copy. It stops the native multi-display encoder for the session without disabling its service. Reboot removes the mount and returns to the factory path. Explicit startup restoration and log-only modes are included; backup/test assets are retained for review.

GitHub Actions run 42261364228 passed at source commit 8a18ef16c743c38b3fa1148c4776539124bde683. It covers handler sanitizers, malformed AVC/crypto/receiver tests, relative frame timing, all 30 frames of a moving H.264 mux/decode fixture, real file copy/restore with ACLs and xattrs, explicit executable dependency loading without LD_PRELOAD, recovery-first boot ordering/ticket consumption, mount/service failure paths, setup refusal, arming and explicit restoration. Service and mount operations are mocked in host integration checks; no vehicle activation is claimed.

Remaining hardware questions: SMACK execution/mapping permissions, normal-boot mount behavior, iPhone negotiation and encrypted frame reception, actual cluster reception and view geometry. The session-level encoder stop is not a production navigation-focus handoff implementation. The test may fail or reboot infotainment; automatic recovery is designed and host-tested, not hardware-proven. It is a narrow startup recovery mechanism, not a complete partition or vehicle backup, and does not undo the earlier speech resets.

See altscreen-test.md for the parked test and recovery instructions. Do not run old upstream precompiled payloads or install the R9900 recovery packages.
