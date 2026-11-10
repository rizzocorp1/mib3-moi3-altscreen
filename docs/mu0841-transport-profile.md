# Matching MU0841 transport profile

Read-only analysis used the privately held MU0841 multi-screen-server, not a different firmware's constants. Firmware binaries and disassembly dumps are not included here. These are native sender characteristics; they do not establish that every field is a strict decoder requirement.

| Field | Native observation | V9 behavior |
|---|---|---|
| PAT transport-stream ID | 100, getProgramAssociationTable at file VMA 0x1dec0 | 100 |
| PAT program / PMT PID | program 1 maps to PID 1000 | same |
| PMT program / H264 / PCR PID | program 1, H264 stream type 0x1b, video/PCR PID 17, getProgramMapTable at 0x1e038 | same |
| Table versions | PAT 0xdb and PMT 0xc3 version/current bytes | same, regenerated MPEG CRCs |
| RTP identifier | constructor stores 100000 at object offset 8; process serializes it big endian | 100000 |
| RTP timestamp | process at 0x1e950 converts steady_clock nanoseconds to milliseconds using signed high multiply and shift; PES/PCR use a separate clock | monotonic milliseconds in RTP; 90 kHz PES/PCR retained |
| RTP payload/type | type 33 without per-access-unit marker; six 188-byte slots after a 12-byte header; send length 0x474 | 1140 bytes, unused slots padded with valid null TS packets |

Native constructor VMA 0x1d918, stream-packet builder 0x1e330. These addresses apply only to the privately inspected firmware copy. Runtime diagnostics record the installed sender hash and read-only displaymanagement resources so differences can be detected. No factory sender code is executed or distributed by these tests.

The native sender's policy and content path remain distinct from transport syntax. Receiving a valid phone keyframe, creating a grant or emitting UDP does not prove that the instrument cluster selected and decoded this source. Displaymanagement source selection, runtime grant readability, multicast delivery and H264 hardware compatibility still require vehicle evidence.

The original research remains useful background: [lipedesign cluster-map](https://github.com/lipedesign/mib3-moi3-lg-research/blob/main/docs/cluster-map.md) and [pipeline analysis](https://github.com/lipedesign/moi3-cluster-pipeline-re/blob/main/docs/shareserver-pipeline.md). The upstream AltScreen repository remains explicitly unconfirmed on a real vehicle; its numeric assumptions are checked against our matching binary rather than treated as an on-car success report.
