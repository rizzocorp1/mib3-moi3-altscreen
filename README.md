# MIB3 MOI3 LG — CarPlay AltScreen → Instrument Cluster (AID)

> **Status:** Active research / alpha — not yet confirmed working on real hardware  
> **Hardware:** VW MIB3 MOI3 LG (`3G5035832C`, SW MU0803/MU0883, Qualcomm Snapdragon AArch64, Yocto Linux, glibc 2.17)  
> **Goal:** Display the CarPlay navigation map (AltScreen stream type 111, 1280×480 H.264) on the VW Active Info Display (digital instrument cluster) via MPEG-TS/RTP multicast

---

## What this project does

VW MQB vehicles with a digital instrument cluster (AID) can display the native navigation map on the cluster. The cluster hardware decoder accepts an MPEG-TS stream on a fixed multicast address. This project intercepts the CarPlay AltScreen H.264 stream (rendered by the iPhone at 1280×480) and forwards it to the cluster decoder — replacing the native nav source with the phone's map.

```
iPhone (Maps/Google Maps/Waze)
  └─ CarPlay AltScreen type 111 (H.264, 1280×480, TCP port 6031)
       └─ altscreen_handler.so  [LD_PRELOAD hook in carplay process]
            ├─ AES key capture
            ├─ AVCC → Annex B
            └─ MPEG-TS mux (PAT/PMT/Video PIDs 0/1000/17)
                 └─ RTP/UDP multicast → [ff14::2:4]:42800 (VLAN 5 / eth0.5)
                      └─ AID cluster hardware decoder → display
```

## Platform details

| Field | Value |
|---|---|
| Unit | VW MIB3 MOI3 LG (`3G5035832C`) |
| Firmware | MU0803 (bench RE on MU0883 dump) |
| SoC | Qualcomm Snapdragon (AArch64) |
| OS | Yocto Linux |
| glibc | 2.17 |
| CarPlay binary | `/lge/app_rw/smartlink/carplay/carplay` |
| SMACK label | `smartlink::app` |
| Cluster multicast | `[ff14::2:4]:42800` via `eth0.5` (VLAN 5) |
| Cluster stream format | MPEG-TS, Program 100, PMT PID 1000, Video PID 17 |

## Repository structure

```
mib3-sandbox/
├── deploy-usb/          USB one-shot deploy for real hardware tests
│   ├── payload/         C source of the LD_PRELOAD hook (altscreen_handler.c)
│   └── fix_hw_partnumber.sh   Deploy script (one-shot, auto-disarms)
├── driver/              HMI simulation stubs (C, Python, Java, shell)
│   ├── *.c              Subsystem stubs (vehicle, audio, media, diag, …)
│   ├── *.sh             Session automation scripts
│   └── *.py / *.java    Analysis and schema tools
├── camada3/             RSI redirect layer (Python, Dockerfiles)
├── glforward-moi3/      OpenGL forwarder bridge (C, Swift helpers)
├── janela/              macOS viewer for the cluster surface (Swift)
├── stubs/               Per-subsystem stubs (coding, media, radio, …)
├── scratch/             MPEG-TS mux prototype (C)
├── ao-vivo/             Live HMI streaming viewer (HTML)
└── *.md                 RE notes and architecture docs
```

## Key technical findings

### CarPlay AltScreen schema (stream type 111)

The iPhone opens stream type 111 when:
1. `features` root dict has bit 26 set (`1ULL << 26`)
2. `enabledFeatures` contains `"altScreen"`
3. The `/info` RTSP response includes a display dict with `type: 111`, `widthPixels: 1280`, `heightPixels: 480`, no `primaryInputDevice`

**`viewAreas` is NOT required for stream 111.** Confirmed by external implementations (chefranov/mhi2-au37x-carplay issue #25): stream 111 opens normally with `enabledFeatures = ["altScreen"]` only. Announcing `"viewAreas"` without a complete viewAreas structure in the display dict causes `TEARDOWN reason=0` before any stream negotiation.

### MPEG-TS parameters (AID hardware decoder)

The cluster decoder is strict — it silently drops non-conforming streams:

| Field | Value |
|---|---|
| Program Number | 100 (`0x0064`) — **not** the generic 1 |
| PMT PID | 1000 (`0x03E8`) — **not** the generic 256 |
| Video PID / PCR PID | 17 (`0x0011`) — **not** the generic 257 |
| Multicast | `[ff14::2:4]:42800` |
| Interface | `eth0.5` (VLAN 5), `SO_BINDTODEVICE` with `strlen(ifname)+1` |

### Compilation (glibc 2.17 cross-compilation)

**Never compile on a modern host without the sysroot.** On Ubuntu 22.04+, `dlsym` resolves from `libc.so.6`, but glibc 2.17 only has it in `libdl.so.2`. Mismatch = relocation error = `carplay.service` crash + immediate reboot.

```bash
# Inside the moi3-bancada:rsidbg Docker container
SYSROOT=referencia/ferramentas/mib3-sandbox/sysroot/u18_glibc_aarch64

aarch64-linux-gnu-gcc -shared -fPIC -O2 -Wall -Wextra \
  altscreen_handler.c -o altscreen_handler.so \
  -nostdlib \
  -Wl,-rpath-link,$SYSROOT \
  $SYSROOT/libdl.so.2 $SYSROOT/libpthread.so.0 \
  $SYSROOT/libc.so.6 $SYSROOT/ld-linux-aarch64.so.1

# Validate
readelf -d altscreen_handler.so | grep NEEDED  # must show libdl.so.2
readelf -r altscreen_handler.so | grep -i tls  # must be EMPTY
```

### Safety constraints

- **Never `system()`** — the SMACK label `smartlink::app` causes SIGKILL → watchdog reboots the car
- **No `__thread` / TLS** — glibc 2.17 does not support `R_AARCH64_TLSDESC`
- **One-shot deploy only** — `DO_TEST_ONCE` flag: the deploy script restores factory files at boot; any reboot = clean state, no brick risk
- **Display 0 isolation** — never identify Display 1 by sequential counter; Display 0 (784×468) must stay native for Cinemo

## Current status and known issues

| Issue | Status |
|---|---|
| `TEARDOWN reason=0` loop (7 cycles in logs) | Root cause confirmed: `viewAreas` announced without valid schema in `/info`. Fix: `enabledFeatures = ["altScreen"]` only |
| Version mismatch (log says v6.0, deploy was v6.1, source is v6.2) | Not yet verified — need to recompile from v6.2 source and confirm |
| `/data/lge/smartlink/carplay/ENABLE_VIEWAREAS` persistence | Survives reboot (persistent `/data` partition). Must be cleared before test |
| AES key capture | Not yet verified on real hardware |
| MPEG-TS reception at cluster | Bench proven; real hardware not yet tested |

## Related work

- [chefranov/mhi2-au37x-carplay](https://github.com/chefranov/mhi2-au37x-carplay) — CarPlay AltScreen on a different platform; confirmed stream 111 works without viewAreas (issue #25)
- [shihabal3amri/DiPlay](https://github.com/shihabal3amri/DiPlay) — Most documented public viewAreas implementation (Android)
- [kamgurgul/mhi2_altscreen_carplay](https://github.com/kamgurgul/mhi2_altscreen_carplay) — Reference: "do not expose multi-ViewArea support unless the receiver can honor it"
- daChillout (mqbtools.nl) — Android Auto on cluster (Cupra Formentor MOI3); no code published
- James11123 — CarPlay AltScreen on MOI3 LG (Jetta MK7); second alpha; no repo

## Contributing

Contributions welcome, especially:
- Testing on other MIB3 / MOI3 variants (MU0803, MU1440, Joynext/Preh)
- AES key capture from `AirPlay_DeriveAESKeySHA512ForScreen`
- MPEG-TS reception logs from other cluster hardware
- viewAreas safe-area values for other display geometries

**Please do not open issues asking for extracted firmware or factory binaries** — those cannot be distributed.

## License

MIT — see [LICENSE](LICENSE)

> **Disclaimer:** This project is for research purposes only. Modifying vehicle software may void your warranty and affect safety systems. Use at your own risk. This project is not affiliated with Volkswagen AG, LG Electronics, Apple Inc., or Qualcomm.
