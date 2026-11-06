# Handler engineering status

Corrected source is in PR #4, branch fix/handler-offline. This is not an installable vehicle payload. Pre-existing .so files in the upstream repository have not been rebuilt and do not correspond to our edits.

## Completed validation

GitHub Actions run 42157534258 compiled the entire changed handler as a Linux host shared library and compiled the test harness with AddressSanitizer and UndefinedBehaviorSanitizer. Packet tests covered PCR encoding at multiple PES/TS boundary sizes, payload reconstruction and clock wrap. AVC tests covered zero-length/truncated NALs, one-byte NAL lengths, malformed config rejection and IDR parameter-set insertion after an AUD. Receiver tests covered missing/failed crypto prerequisites, blocked accept shutdown, connected receive shutdown, failed decryption and 25 start/stop cycles. A generated 1280x480 H.264 fixture passed through the actual muxer and FFmpeg decoded the resulting MPEG-TS with one counted frame.

These checks establish host behavior only. They do not validate AArch64 factory ABI compatibility or cluster reception.

## Factory inspection and target build

The matching 0841 archive has downloaded successfully and passed its integrity test. Its manifest includes US variants. See factory-0841-audit.md for its hash, extracted runtime and hook evidence, successful factory-linked AArch64 candidate, signature checks and rejected recovery carriers.

GitHub Actions run 42168729717 passed sanitizer checks and a 30-frame moving H.264 mux/decode test after the factory-based derivation return-type correction.

tools/build-target-windows.ps1 records compiler, source, factory-library and output hashes for the local AArch64 build. The candidate imports only GLIBC_2.17 symbols from the factory glibc 2.23 runtime. It has not been executed on the vehicle.

The remaining gaps are the one-shot launch/restore mechanism, native cluster-channel ownership and hand-back, incoming frame timing, concurrent-session behavior and on-car validation. The engineering USB trigger deletes four speech paths before our backup entry point. The two R9900 recovery packages contain bootloader writes and are unsuitable substitutes. No claim of complete stock rollback is made.

The container-free summary-hook probe avoids the engineering package and contains no firmware or deletion script. It is an execution/menu-path experiment only; no general no-update route to the summary screen is established. See summary-probe.md. The user explicitly declined the speech-reset route.

