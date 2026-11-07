# MU0841 auxiliary endpoint experiment v7

V6 connected primary CarPlay, acknowledged AltScreen, received an auxiliary request and derived HomeKit screen security, but no auxiliary TCP connection arrived. No video authentication or encoder handoff occurred. The final accept error was normal teardown. The cause of the missing connection remains unproven.

V7 returns a minimal type/dataPort screen descriptor rather than echoing the request and inventing streamID=111. It preserves primary replies, replaces duplicate auxiliary descriptors, validates actual fields before reporting success and closes the listener if response construction fails. The OS allocates the listener port, which is read back and advertised. Listener family, actual factory/final response ports and bounded read-only network snapshots aid further diagnosis. Firewall and label rules are unchanged.

Initial SETUP acknowledgement, HomeKit security, deferred authenticated-IDR handoff, recovery-first startup and one-session RAM copy remain. No native encoder stop occurs before valid auxiliary video. Failed handoff restores native encoding. A new stage/unit separates this attempt; the tested handoff protocol remains unchanged. Reboot ends the experiment.

Source 5a231a50ea008ffb848e442be53abebf47eac435 passed all four jobs in Actions run42425963519: IPv4/IPv6 receiver paths, minimal response schema and failure handling, authenticated framing and TCP/RTP rejection, metadata/ELF/recovery, deferred handoff, real systemd logging and H264 decode. Matching factory-library target build and private archive/USB verification passed. Host tests do not establish on-car packet delivery or rendering.

Private archive moi3-altscreen-endpoint-0841-v7.zip is 1,389,224 bytes, SHA256 db57b6086631e37b1395ec5dbc78bab2e023cf4cf7e83997bf888b1969f13c55. Signed Data/Meta unchanged, only MIB3OI_POST_SCRIPT; eight payload hashes and source templates verify. See altscreen-map-v7.md for one parked wired test and recovery. No V7 map success is claimed.

Only reviewed source/tests/scripts/docs are published, with the user's explicit authorization for this fork. Captured device identifiers, raw logs, capability snapshots, firmware-derived binaries, archives and backups remain local/private. Generic compatibility checks remain in code. The upstream author repository is unchanged. Known speech resets remain; recovery is narrow rather than a complete vehicle backup.
