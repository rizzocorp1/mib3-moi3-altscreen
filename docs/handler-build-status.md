# V8 permitted-port experiment — awaiting vehicle validation

V7 preserved normal CarPlay and factory navigation instructions, but no auxiliary TCP connection or video reached the handler. Its read-only IPv6 firewall snapshot shows a default-drop input policy with explicit TCP 49200..49499 accepts in both directions. The last ephemeral listener and the earlier auxiliary port were outside those permits. No packet capture establishes that the phone sent a SYN; firewall filtering is the next evidence-supported transport hypothesis.

V8 binds an unused port within the existing permitted range, skips occupied ports without taking them over, advertises the actual bound port and fails auxiliary setup if the range is exhausted. It makes no firewall, SMACK or native-service configuration changes. Primary CarPlay, minimal SETUP response, HomeKit framing, authenticated-IDR handoff and recovery-first startup remain as before. A new stage/unit separates this experiment. Tests cover occupied-port fallback, range exhaustion and IPv4/IPv6 receiver connections in addition to existing security and recovery checks.

This is not a confirmed map solution. Host tests cannot establish on-car packet delivery, native cipher ABI behavior or cluster rendering. See altscreen-map-v8.md. Known speech resets remain and recovery is not a complete vehicle backup.

Only source/tests/scripts/docs are published to the user's fork. Raw vehicle reports, addresses, capability snapshots, firmware-derived binaries, archives and backups remain private. No change is submitted to the upstream author.
