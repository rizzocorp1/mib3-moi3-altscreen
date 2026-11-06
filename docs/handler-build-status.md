# Handler engineering status

Corrected source is in PR #4, branch fix/handler-offline. This is not an installable vehicle payload. Pre-existing .so files in the upstream repository have not been rebuilt and do not correspond to our edits.

## Completed validation

GitHub Actions run 42157534258 compiled the entire changed handler as a Linux host shared library and compiled the test harness with AddressSanitizer and UndefinedBehaviorSanitizer. Packet tests covered PCR encoding at multiple PES/TS boundary sizes, payload reconstruction and clock wrap. AVC tests covered zero-length/truncated NALs, one-byte NAL lengths, malformed config rejection and IDR parameter-set insertion after an AUD. Receiver tests covered missing/failed crypto prerequisites, blocked accept shutdown, connected receive shutdown, failed decryption and 25 start/stop cycles. A generated 1280x480 H.264 fixture passed through the actual muxer and FFmpeg decoded the resulting MPEG-TS with one counted frame.

These checks establish host behavior only. They do not validate AArch64 factory ABI compatibility or cluster reception.

## Required factory evidence

- Exact 0841 package manifest and variant list, plus a local archive hash.
- Factory CarPlay executable, AirPlay/CoreUtils/Support libraries and exported symbols.
- Factory glibc, libdl, libpthread and dynamic loader; verify actual versions rather than assuming the public README's 2.17.
- AES functions' exact signatures and context size/layout. Current crypto code retains upstream ABI assumptions.
- Factory service units, startup scripts and SMACK labels.
- DisplayManagement configuration and how multi-screen-server/native navigation yields and regains the cluster channel.

build-target.sh is a fail-closed target-linking recipe, not a validated build: it requires extracted AArch64 factory libraries and refuses ambiguous or missing inputs. The exact sysroot layout may require adjustment after inspection. A successful target link still does not prove the dynamically intercepted factory function ABIs.

## Factory inspection and target build

The matching 0841 archive has downloaded successfully and passed its integrity test. Its manifest includes US variants. See factory-0841-audit.md for its hash, extracted runtime and hook evidence, successful factory-linked AArch64 candidate, signature checks and rejected recovery carriers.

GitHub Actions run 42166913526 passed sanitizer checks and a 30-frame moving H.264 mux/decode test. The subsequent factory-based derivation return-type correction requires a new CI run.

tools/build-target-windows.ps1 records compiler, source, factory-library and output hashes for the local AArch64 build. The candidate imports only GLIBC_2.17 symbols from the factory glibc 2.23 runtime. It has not been executed on the vehicle.

The remaining gaps are the one-shot launch/restore mechanism, native cluster-channel ownership and hand-back, incoming frame timing, concurrent-session behavior and on-car validation. The engineering USB trigger deletes four speech paths before our backup entry point. The two R9900 recovery packages contain bootloader writes and are unsuitable substitutes. No claim of complete stock rollback is made.
