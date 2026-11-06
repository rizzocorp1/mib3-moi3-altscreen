# Atlas MU0841 factory evidence

## Archive obtained and inspected

MIB Vault file: `MOI3_EU_VW_R8410L(0841)_3G0919360TH.7z`, 6,246,676,571 bytes.

SHA-256: `7d19bcd1f412dd31ada3f6828ca02bab62f15550221b227a0974d69555409dc8`.

7-Zip integrity test passed. Manifest release MOI3_EU_VW_R8410L, MUVersion 0841, package 1.841.3. Its eight variants include both FM3-SM-*-US-VW-MQB-LG and FM3-S-*-US-VW-MQB-LG, alongside EU/RW/W2 variants. This supports the media-number match to VW bulletin 91-23-03 for the 2021 Atlas Cross Sport. It is not a reading of the actual vehicle's installed binary hashes.

The CPU OTA includes a 3,069,050,880-byte system.new.dat. Its transfer list contains one contiguous new range, blocks 0 through 749279, so the file is directly readable as ext4. The filesystem has a 23,986,176-byte tail; it was inspected as a file, never mounted or modified. OS release is AGL 3.0.0+snapshot-20230314; factory libc is 2.23.

## Hook and crypto evidence

All ten symbols inventoried by tools/check-sysroot.mjs are present in AArch64 factory libraries. Library hashes:

- libAirPlay.so: f41be90d491d22350663995f50f992fc5bbb97fa546c67b4b32e43edb4b2f2fd
- libAirPlaySupport.so: da25d0566cf5a06e664dce9afa99b4691f70daf8385c0bf0e58cacc939297df0
- libCoreUtils.so: 82984d03af478261f5cc16e30f7566b091d47c26a9d64972d5b444fb5f44cf44

Static disassembly confirms the register argument order for AES_CTR_Init, AES_CTR_Update and AES_CBCFrame_Init. CTR initialization/update access through offset 0x11f; the handler's aligned 512-byte storage covers these observed accesses. This establishes a sufficient observed buffer bound, not a published SDK structure definition.

AirPlay_DeriveAESKeySHA512ForScreen takes five arguments. The factory setup call at 0x1c104 does not consume a status result; the routine does not explicitly return one. The handler declaration was changed to void and its test now exercises AES initialization failure instead of a fabricated derivation status. The factory's security setter passes a 16-byte CBC key and stores that same key for later screen derivation. Other possible CBC users and concurrent session behavior remain unverified.

The incoming screen-frame path uses a 128-byte header, opcode at offset 4 and a time-synchronizer input at offset 8. The test receiver now uses relative NTP timing from that input with monotonic fallback; host tests cover intervals and backwards-clock rebasing. On-car clock behavior remains unverified.

## Target build

tools/build-target-windows.ps1 builds locally with portable Zig 0.17.1, AArch64 glibc 2.23 headers and the extracted factory libc/libdl/libpthread/loader, using -nostdlib and -z defs. The resulting shared library is AArch64 and needs libc.so.6, libdl.so.2 and libpthread.so.0, with only GLIBC_2.17 symbol requirements. Input and output SHA-256 hashes are recorded beside the local candidate. No proprietary library is uploaded for building.

This is a target-linked engineering candidate, not a USB installer or an on-car validated payload. The repo's old precompiled binaries remain separate from this build.

## Startup and installer constraints

Factory carplay.service runs as sint_sec_carplay_display with executable SMACK label carplay::app. It uses the factory carplay.sh as a privileged ExecStartPre and specifies FailureAction=reboot-immediate. The existing upstream installer uses inferred labels and reconstructs a factory script rather than preserving its exact bytes. Do not treat its rollback claims as validated.

The one-session test bootstrap stops the native multi-display encoder before factory CarPlay starts with the experimental handler. It does not disable the service; reboot is the hand-back mechanism. This is not a production navigation-focus controller and has not been validated on the vehicle.

Both the stock checksum-file signature and the engineering-container checksum-file signature verify as RSA-PSS/SHA-256 against the public key extracted from this factory image. Public-key file SHA-256: 98a7ff2306f05c734655f91da41b14b605fafad988c19cb700ef60ab8b92cc82. This verifies signatures against that supplied key; an independent trust root and acceptance by the actual unit have not been established.

The engineering post-script still removes four speech paths before our custom entry point. Signature verification does not remove those side effects or make a later backup cover them.

## Container-free maintenance hook candidate

The hardware-part-number script is invoked by the updater itself, not by the signed 206-byte speech-reset script. In this MU0841 binary, SummaryScreen::initScreen (0x3619c8) constructs the port-1 script path at 0x361d54/0x361d60, checks it with access at 0x361eb0, and calls UpdateHandler::run_cmd at 0x361ed0. The helper (0x20aba8) uses posix_spawn for /bin/sh -c and waitpid. A port-2 fallback is also present.

The screen-factory switch identifies SummaryScreen as screen 27. Normal logging uses LoggingScreen, not SummaryScreen. Engineering-menu startup initially creates other screens. Reaching the real summary screen from a normal no-update menu state has NOT been established. Existing update-summary state might permit a test, but no general no-update navigation sequence is claimed.

The container-free summary probe contains only a reporting script, a marker and instructions. It has no Data/Meta folders or firmware payload, refuses mixed update/activation sticks, and does not use the speech-reset or recovery packages. A host fixture verified shell syntax, report generation, unchanged factory input and refusal when Data is present. A vehicle menu-path check is still required; do not start or deliberately fail an update to reach the hook.

## Recovery packages rejected as test carriers

MOI3_RECOV_R9900L_5FA919360AM.7z and MOI3_RECOV_R9900L_5FA919360AS.zip were downloaded and inspected offline. Both include conditional raw partition writes for boot components, plus Ethernet, navigation, speech and updater repair operations. Their generic wildcard manifest does not make them harmless. Neither package is used in our USB test bundle; none of their scripts or binaries has been executed.

Only inspection tools and these findings belong in the public repository. Keep firmware images, factory libraries and vehicle snapshots local.

