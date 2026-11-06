# Atlas MU0841 AltScreen v2 experiment

> V2 VEHICLE RESULT: This package is retired. Its boot script restored factory startup but aborted before mounting the test copy or stopping the encoder: factory systemd-run rejected TimeoutStopSec=5. CarPlay and factory turn-by-turn remained functional. Retained v1 logs prove v1 activated and the iPhone requested stream 111, but the receiver refused crypto prerequisites. No video reception or cluster map is established. The source removes the unsupported option and adds non-secret security-path diagnostics; no replacement USB is ready. The source now differs from the archived v2 ZIP below.


The user confirmed wired CarPlay and factory Apple Maps turn-by-turn working after explicit v1 recovery. This confirms the reported affected functions recovered, not a complete vehicle audit.

V1 is retired. Its vehicle test did not show maps and temporarily disrupted CarPlay; only its setup report returned. Whether the test bootstrap actually activated is unresolved. An ExecStartPre collector lifecycle problem was suspected, but a current systemd 255 runner allowed that background child to survive, so termination on the factory manager is not established.

V2 attempts actual cluster maps. It preserves the original enabledFeatures list, deduplicates additions and leaves the stock list unchanged on allocation failure. Primary-only SETUP responses are unchanged. It uses a separate v2 stage and collects available retained v1 logs before arming. A bounded independent systemd logger must report ready before the RAM bind mount or native encoder stop. Handler messages also go to standard error for CarPlay journal capture. Collector startup failure and readiness timeout leave the original executable and encoder in place. Old backups are not deleted.

The executable remains unchanged on disk. Startup restores its exact baseline bytes and metadata before consuming the one-shot ticket. The native encoder is stopped only for the test session; reboot removes the bind mount and returns to the normal factory startup path. This still lacks production navigation-focus handoff and is not a complete backup or undo for speech resets.

Source commit f67d51ad2a6238afdbced2953b8b6c4faf96c4ee passed Actions run 42271028401: handler sanitizers and feature semantics, metadata/ACL/xattr recovery, ELF dependency loading, setup/restoration, bootstrap failures including unavailable/timed-out logger, real systemd collection after main-service startup, and 30-frame H.264 mux/decode. Mount and native service control are mocked in the bootstrap tests; the standalone logger lifecycle test uses a real service manager. No v2 vehicle success is claimed.

Local archive: moi3-altscreen-once-0841-v2-EXPERIMENTAL.zip (1,371,766 bytes), SHA-256 7a341e6aeb35cc7293d4151394ab689719878472684cf7b16250d13e9a63e66c. Verification confirms unchanged signed Data/Meta, only MIB3OI_POST_SCRIPT, all eight payload hashes, and matching current script templates. Firmware-derived binaries and this archive remain private/local.

Remaining vehicle questions include SMACK permissions, bootstrap execution, dynamic loading, iPhone negotiation/encrypted video and cluster reception. The v2 test is designed to distinguish those stages. See altscreen-test-v2.md for one parked attempt and recovery. Do not rerun activation after a failure or mix v1/v2 files.