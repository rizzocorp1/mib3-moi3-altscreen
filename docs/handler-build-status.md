# MU0841 stock-info diagnostic v5

V4 is retired after failed initial CarPlay negotiation. It activated, cached HomeKit contexts, and returned modified display advertisements, followed by teardown. Returned logs have zero auxiliary requests or security-ready/first-frame events, so vehicle decryption was not exercised. The user confirmed wired/wireless CarPlay and factory turn-by-turn work normally after reboot.

The exact rejected field is unknown. Preserved feature names, inherited display/viewAreas schema, other response changes, transport differences and preemptive native encoder stop are unisolated hypotheses. No further map activation should be supplied without understanding this negotiation failure.

The inherited capability-snapshot call had an ABI error: the factory function takes plist, format-string, path and recognizes binary1. The old code passed the filename as format and NULL as path. The corrected typed API is in source. This explains missing intended snapshots, not necessarily the connection failure.

V5 returns the original factory info object, arguments, error and errno unchanged and captures at most three successful responses via an atomic binary-file snapshot. It has no feature mutation, AltScreen injection, stream receivers, pairing-reference cache, key capture or encoder stop. The native cluster encoder remains unchanged. The independent logger copies completed snapshots to USB. A prior modified snapshot is retained if available.

Source b311ce4fc55dd688b7bd0c2e39210af90b326453 passed all four jobs in Actions run 42342729776. Tests validate exact snapshot argument order, bounded binary writes, original object/argument/error/errno preservation, missing/failed writer behavior, snapshot export after main startup under real systemd, recovery/arming failures and existing handler regression checks. These tests do not execute the factory AArch64 snapshot writer.

Private local archive: moi3-stock-info-0841-v5.zip, 1,342,380 bytes, SHA-256 9183d7cb94268357f59e4eb22e3e9749ad507cdc90462e0fdcec32a9f88760e1. Signed Data/Meta unchanged, only MIB3OI_POST_SCRIPT; eight payload hashes and templates verify. See stock-info-v5.md for the one-session parked capture and recovery.

No V5 vehicle success is established. Snapshots may contain device identifiers and must stay local/private. Firmware-derived binaries, archives and raw reports are not published. The signed trigger still repeats the four authorized speech resets. This is a narrow startup recovery mechanism rather than a complete vehicle backup.
