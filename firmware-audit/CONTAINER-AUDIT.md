# Engineering container inspection — 2026-11-05

Obtained the 5,858-byte archive linked by James11123 in the original [MOI3 CarPlay article](https://www.drive2.ru/l/745725070915144961/): [Google Drive source](https://drive.google.com/file/d/1bbDFspN-k0PpT4wa_p5h8Re-ewM-q7Hd/view).

Archive SHA-256: `763a85698771ef2bb69fb29eb6a1e8b0a32b063aee1910194865f70ea6c1005d`.

## Confirmed static contents

- `MOI3_EU_SE_E9400L`, package `DeviceRelease_940.MAIN`, MUVersion `Y940`, version `1.940.2`.
- Variant string `FM3-*-*-*-*-*-LG`; this is an allowlist pattern, not proof of Atlas MU0841 compatibility.
- Only included update component: `MIB3OI_POST_SCRIPT`. No CPU/HMI/MCU image is present in this ZIP.
- Hardware indexes 21 and 31 point to the same script-installer descriptor.
- Three manifest hashes plus the installer-descriptor hash match when JSON is serialized without whitespace. The post-script's raw SHA-256 matches its descriptor. Initial raw-file manifest mismatches were serialization differences, not evidence of corruption.
- Signature file names `main.mnf.cks` and contains a 2023-09-25 date plus 24 hexadecimal signature chunks. Signature authenticity has NOT been verified against a trusted unit key.
- `tnrref.csv` contains shell calls to factory `disp_sysinfo.sh` and `get_version_info.sh` redirecting output to USB port 1. Their call order and execution on MU0841 are not established by static inspection.
- Root `fix_hw_partnumber.sh` contains `ksend` with a binary message. Its effect is unverified; our experimental probe replaces this entire file and does not execute that command.

## Side effects before the custom entry point

The original signed post-script deletes these four paths and calls sync:

```
/lge/speech/db/asr/common/dynvocab
/data/lge/speech/tts_lang_info.cfg
/data/lge/speech/asr_lang_info.cfg
/data/lge/audiomgnt/sse/TEMPLATE_MQB37W_VOCON_WUW.scd
```

Therefore this update-container route is NOT read-only even when our custom script is read-only. Voice personalization/language/wake-word data may need reinitialization. The exact user-visible effect on Atlas MU0841 is not verified. Our script runs too late to back up those paths before deletion. The scoped AltScreen backup tool does not cover that earlier change, and must not be represented as a complete stock recovery mechanism.

## First on-unit experiment

The local experimental ZIP keeps Data and Meta byte-identical to the source and replaces only the root script with the tested non-installing probe. No hook library, launcher, CarPlay binary or upstream installer is included. This tests the entry mechanism and gathers identification/tool information; it does not prove AltScreen functionality. The vehicle updater may reject the package; do not disable verification, change allowlists, or force an update to overcome rejection.

This is a research candidate, not a factory-approved update or hardware-validated product. No on-unit installation or signature verification has occurred in this session. Keep the proprietary archive local; publish only the builder, inspection evidence and our scripts.
