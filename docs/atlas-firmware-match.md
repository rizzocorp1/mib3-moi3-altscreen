# Atlas Cross Sport firmware selection

Target provided by the user: 2021 Atlas Cross Sport, head-unit part 3G5035842B, H56, software 0841.

## Selected research archive

`MOI3_EU_VW_R8410L_MU0841_3G0919360TH.7z`

MIB Vault directory: https://mibvault.com/files/MOI3/LG

An alternate entry is named `MOI3_EU_VW_R8410L(0841)_3G0919360TH.7z`. Both entries advertise 5.8 GB. Their byte-for-byte equivalence is unknown until hashes are available.

## Why this is relevant to this car

[Volkswagen bulletin 91-23-03 / 2070941](https://static.nhtsa.gov/odi/tsbs/2023/MC-10239652-0001.pdf) explicitly includes the 2021 Atlas Cross Sport, specifies software 0841, and specifies media 3G0.919.360.TH. The archive filename matches that media reference. The fact that the filename includes EU is not by itself sufficient to disqualify it, since this same media number is specified by Volkswagen Group of America.

This is a shared head-unit update reference, not necessarily an archive named after each vehicle model. The head-unit part number and update-media number are different identifiers. MIB Vault's exact search for 3G5035842B returned zero matching filenames. Its Atlas search exposed datasets such as parking-assistance and amplifier files; those are not substitutes for the infotainment firmware.

## What remains to verify

The manifest/variant list and component selection must still be inspected after download. Matching filename/media number is strong selection evidence, not a verified factory binary match. Obtain a hash and inspect the actual archive; do not flash it to prove compatibility. The purpose here is offline extraction of CarPlay libraries and target runtime files.

The vehicle already reports 0841. VW's bulletin says its update procedure does not apply when software is already 0841 or newer. There is no reason to reinstall stock firmware merely to develop the AltScreen handler.

## Download and manifest inspection completed

The alternate (0841) archive completed at 6,246,676,571 bytes and passed its integrity test. Its manifest includes US variants. See factory-0841-audit.md for its hash and factory analysis. The unit's actual installed binary hashes are still unmeasured.
