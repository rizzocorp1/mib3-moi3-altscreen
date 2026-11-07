# MU0841 gated map experiment v6

The v5 stock-info diagnostic succeeded with one parsed response. Up to three files were possible; one call/one snapshot is normal. The captured factory response has no enabledFeatures entry and no display viewAreas entry. The preserved-viewAreas conflict hypothesis is absent in this sample. Raw snapshots stay private.

V4 failed before any auxiliary request or decryption; it remains retired. A comparison with the v1 trace that reached stream111 identifies a changed initial handshake step. V6 restores AltScreen acknowledgement during initial/no-stream SETUP while leaving primary media/error/NULL requests unchanged. This is a candidate correction, not a proven vehicle root cause.

V6 removes the startup encoder stop. Only an authenticated HomeKit IDR that passes H264 validation can request handoff. The independent root collector stops native encoding and clones a metadata-preserving grant; the output gate requires that grant and an IDR before forwarding. Failed handoff restarts native encoding and withholds output. A stream ending before a grant cancels its request. Stale markers refuse installation/activation. The unit is never disabled or edited.

Both stock and advertised capability snapshots are collected using the verified binary writer ABI. The native encoder remains available when no valid auxiliary video arrives. If handoff activates, reboot remains the end of this experimental session; production focus handoff is not established.

Validation source 2ba11674bfe94c0a4f5583d78d149ac12cb75e78 passed all four jobs in Actions run 42422810468. Tests cover initial acknowledgement, media/error/NULL pass-through, actual C output gate plus collector/metadata helper with mocked services, early end/no-video, successful grant, stop/still-active/clone failure rollback, stale markers, and existing authenticated receiver/recovery/systemd/H264 checks. The target build binds the stat check to verified AArch64 glibc2.23 __fxstat version0 with layout assertions. Host tests do not execute factory AArch64 crypto or prove rendering.

Local private archive moi3-altscreen-gated-0841-v6.zip is 1,385,800 bytes, SHA256 3750d8b5176efb88e005fd3a2e27b9de8f7e0482c57fc41e856d9663965417d2. Signed Data/Meta unchanged, only MIB3OI_POST_SCRIPT; eight payload hashes and generated templates verify. Recovery-first startup restores original bytes/metadata before one-shot RAM mounting. Known speech resets remain; this is not a full vehicle backup.

No V6 vehicle success is established. Use one parked WIRED attempt matching the confirmed security path; see altscreen-map-v6.md. Archives, firmware-derived binaries, raw reports and capability snapshots remain local/private. Do not mix versions or repeat failed activation.
