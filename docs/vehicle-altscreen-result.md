# Vehicle AltScreen test and recovery — 2026-11-06

The first one-session test did not achieve a cluster map. The user reported wired CarPlay connection failures, wireless connecting after retries, and absent factory Apple Maps turn-by-turn. Wired CarPlay later worked normally before the explicit recovery update, after the car had been shut off. This does not establish which boot path ran.

Only altscreen-setup.txt was present on USB; it reported ARMED/SETUP COMPLETE. No boot-status, handler or journal files were present. Neither successful activation nor ticket consumption can be inferred from that alone.

The explicit recovery update subsequently reported RESULT: FACTORY STARTUP RESTORED and SETUP COMPLETE, with no errors. That branch verifies backup/startup/executable hashes and restores startup metadata, removes a recognized experimental bind mount if present, and removes the activation ticket. Factory turn-by-turn has not yet been functionally checked after recovery. Keep the test stick out of the car during stock verification.

## Instrumentation design under investigation

The boot script starts a background collector in ExecStartPre. The systemd v232 documentation warns that these children are killed before the main process. However, a real current Ubuntu runner allowed the child to survive even when its write was gated on main-service startup. Therefore collector termination is a hypothesis for this vehicle, not a confirmed explanation. The replacement collector has a separate managed service and readiness handshake; the integration test must verify it exports evidence created after main startup. Vehicle logs are still needed to establish what ran.

Primary reference: https://github.com/systemd/systemd/blob/v232/man/systemd.service.xml

The existing activation ZIP is retired from further testing. Do not ask the user to repeat activation. Before another experiment: verify stock functions, collect retained on-unit boot/handler logs without arming a test, and replace the collector with a separately managed, bounded service whose lifecycle is actually tested. Do not loosen factory metadata, signature or SMACK checks to obtain a successful activation.
