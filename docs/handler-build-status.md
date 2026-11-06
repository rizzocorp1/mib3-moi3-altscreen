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

## Firmware retrieval

MIB Vault lists MOI3_EU_VW_R8410L_MU0841_3G0919360TH.7z (5.8 GB). The browser download attempts have not produced a confirmed local archive. The site now explicitly requires sign-in because its guest daily allowance is exhausted. No account, paid plan or credentials were created or purchased. Continue with the user's authenticated session; do not circumvent the quota or publish proprietary firmware to the public repository.
