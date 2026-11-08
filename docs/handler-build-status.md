# V11 corrected outgoing observation — diagnostic build awaiting vehicle results

V10 authenticated 3002 video frames, validated the encoder grant, released the opening queue and reported 17682 successful UDP calls with zero errors. No cluster picture appeared. The observer initialized but recorded no flows. This does not establish packet loss: its ETH_P_IPV6 subscription missed Linux outgoing taps, while the previous loopback assertion accepted incoming copies. The user confirmed wired connection; it began about 12 seconds after handler startup, so the requested full-minute native reference was not recorded.

V11 uses ETH_P_ALL on the same bound interface, then filters IPv6/UDP42800 in userspace. It enables no promiscuous membership or interface changes. Tests now explicitly require outgoing capture, including multicast on an isolated VLAN5 fixture. Heartbeat and filter-stage counts distinguish empty capture from parser filtering. Existing phase separation and bounded RTP/TS/codec metadata remain. No picture bytes or literal network addresses are saved.

Linux's transmit tap dispatch iterates the all-protocol packet lists: [kernel v4.4 dev_queue_xmit_nit](https://github.com/torvalds/linux/blob/v4.4/net/core/dev.c#L1722). This corrects measurement; it is not evidence that transmitted vehicle video has reached the decoder.

The service index obtained in v10 advertises a separate local displaymanagement port. V11 discovers that port rather than using gateway resource URLs that returned403, and issues bounded GET-only snapshots at startup and approximately40/90seconds. The actual client is tested against discovery/error fixtures and rejects missing, ambiguous, external or invalid endpoints. No display configuration is written.

The video path remains the v10 path: source-port ownership after authenticated grant, factory transport profile, bounded RAM queue, stream-scoped teardown and recovery-first RAM copy. V11 is a diagnostic comparison, not a confirmed rendering fix. New asset/marker names preserve older evidence. The test requires a full60seconds of visible native VW cluster map with the cable unplugged before two minutes of wired Apple Maps, without app switching. See altscreen-map-v11.md.

Only reviewed source/tests/scripts/docs are published to the user's fork. Raw reports, source-tag fingerprints, unique identifiers, capability snapshots, firmware-derived binaries, install ZIPs and backups remain private. No original-author repository changes or merge. Known speech resets remain; startup recovery is narrow rather than a complete vehicle backup.
