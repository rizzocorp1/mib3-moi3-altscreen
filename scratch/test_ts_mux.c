#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* CRC32 for MPEG-TS */
static uint32_t ts_crc32(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint32_t)data[i] << 24;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ 0x04C11DB7;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

static uint8_t s_pat_cc = 0;
static uint8_t s_pmt_cc = 0;
static uint8_t s_video_cc = 0;

static void build_pat_packet(uint8_t *pkt) {
    memset(pkt, 0xFF, 188);
    pkt[0] = 0x47;
    pkt[1] = 0x40; // PUSI=1, PID=0x0000
    pkt[2] = 0x00;
    pkt[3] = 0x10 | (s_pat_cc++ & 0x0F);
    pkt[4] = 0x00; // pointer field

    pkt[5] = 0x00; // table_id = 0x00
    pkt[6] = 0xB0;
    pkt[7] = 0x0D; // section_length = 13
    pkt[8] = 0x00; // transport_stream_id = 1
    pkt[9] = 0x01;
    pkt[10] = 0xC1;
    pkt[11] = 0x00;
    pkt[12] = 0x00;
    pkt[13] = 0x00; // program_number = 1
    pkt[14] = 0x01;
    pkt[15] = 0xE1; // PMT PID = 0x0100
    pkt[16] = 0x00;

    uint32_t crc = ts_crc32(&pkt[5], 12);
    pkt[17] = (crc >> 24) & 0xFF;
    pkt[18] = (crc >> 16) & 0xFF;
    pkt[19] = (crc >> 8) & 0xFF;
    pkt[20] = crc & 0xFF;
}

static void build_pmt_packet(uint8_t *pkt) {
    memset(pkt, 0xFF, 188);
    pkt[0] = 0x47;
    pkt[1] = 0x41; // PUSI=1, PID=0x0100
    pkt[2] = 0x00;
    pkt[3] = 0x10 | (s_pmt_cc++ & 0x0F);
    pkt[4] = 0x00; // pointer field

    pkt[5] = 0x02; // table_id = 0x02 (PMT)
    pkt[6] = 0xB0;
    pkt[7] = 0x12; // section_length = 18
    pkt[8] = 0x00; // program_number = 1
    pkt[9] = 0x01;
    pkt[10] = 0xC1;
    pkt[11] = 0x00;
    pkt[12] = 0x00;
    pkt[13] = 0xE1; // PCR PID = 0x0101
    pkt[14] = 0x01;
    pkt[15] = 0xF0;
    pkt[16] = 0x00;

    pkt[17] = 0x1B; // stream_type = 0x1B (H.264)
    pkt[18] = 0xE1; // Video PID = 0x0101
    pkt[19] = 0x01;
    pkt[20] = 0xF0;
    pkt[21] = 0x00;

    uint32_t crc = ts_crc32(&pkt[5], 17);
    pkt[22] = (crc >> 24) & 0xFF;
    pkt[23] = (crc >> 16) & 0xFF;
    pkt[24] = (crc >> 8) & 0xFF;
    pkt[25] = crc & 0xFF;
}

/* Encapsulates a frame (Annex B) into MPEG-TS packets */
typedef void (*ts_output_fn)(const uint8_t *ts_pkt, void *user_data);

static void mux_h264_frame(const uint8_t *frame_data, size_t frame_len, uint64_t pts_90k, ts_output_fn out_cb, void *user_data) {
    // 1. Build PES packet
    // PES Header: 14 bytes (with PTS)
    size_t pes_header_len = 14;
    size_t total_pes_len = pes_header_len + frame_len;
    uint8_t *pes_buf = (uint8_t *)malloc(total_pes_len);
    if (!pes_buf) return;

    pes_buf[0] = 0x00;
    pes_buf[1] = 0x00;
    pes_buf[2] = 0x01;
    pes_buf[3] = 0xE0; // Stream ID 0xE0 (Video)

    // PES packet length: 0 for unbounded video in TS (ISO 13818-1)
    pes_buf[4] = 0x00;
    pes_buf[5] = 0x00;

    pes_buf[6] = 0x80; // Marker bits (10)
    pes_buf[7] = 0x80; // PTS present (flag 0x80)
    pes_buf[8] = 0x05; // Header data length (5 bytes for PTS)

    // 5-byte PTS encoding
    pes_buf[9]  = 0x21 | (((pts_90k >> 30) & 0x07) << 1);
    pes_buf[10] = (pts_90k >> 22) & 0xFF;
    pes_buf[11] = 0x01 | (((pts_90k >> 15) & 0x7F) << 1);
    pes_buf[12] = (pts_90k >> 7) & 0xFF;
    pes_buf[13] = 0x01 | ((pts_90k & 0x7F) << 1);

    memcpy(pes_buf + 14, frame_data, frame_len);

    // 2. Slice PES into 188-byte TS packets
    size_t offset = 0;
    int is_first = 1;

    while (offset < total_pes_len) {
        uint8_t ts_pkt[188];
        ts_pkt[0] = 0x47; // Sync byte
        ts_pkt[1] = (is_first ? 0x40 : 0x00) | 0x01; // PUSI, PID 0x0101 (high: 0x01)
        ts_pkt[2] = 0x01; // PID 0x0101 (low: 0x01)

        size_t remaining = total_pes_len - offset;

        if (is_first) {
            // First packet: include PCR in adaptation field (8 bytes: 1 af_len + 1 af_flags + 6 pcr)
            // Header 4 + AF 8 = 12 bytes header, 176 bytes payload
            uint8_t af_len = 7; // flags (1) + PCR (6)
            ts_pkt[3] = 0x30 | (s_video_cc++ & 0x0F); // afc=3 (adaptation + payload)
            ts_pkt[4] = af_len;
            ts_pkt[5] = 0x10; // PCR flag = 1

            // 6-byte PCR (33-bit base + 6 reserved + 9-bit ext)
            uint64_t pcr_base = pts_90k;
            ts_pkt[6]  = (pcr_base >> 25) & 0xFF;
            ts_pkt[7]  = (pcr_base >> 17) & 0xFF;
            ts_pkt[8]  = (pcr_base >> 9) & 0xFF;
            ts_pkt[9]  = (pcr_base >> 1) & 0xFF;
            ts_pkt[10] = ((pcr_base & 1) << 7) | 0x7E;
            ts_pkt[11] = 0x00;

            size_t payload_space = 188 - 12; // 176
            size_t copy_bytes = (remaining < payload_space) ? remaining : payload_space;
            if (remaining < payload_space) {
                // Need stuffing inside adaptation field
                size_t stuff_len = payload_space - remaining;
                ts_pkt[4] = af_len + stuff_len;
                memset(&ts_pkt[12], 0xFF, stuff_len);
                memcpy(&ts_pkt[12 + stuff_len], pes_buf + offset, copy_bytes);
            } else {
                memcpy(&ts_pkt[12], pes_buf + offset, copy_bytes);
            }
            offset += copy_bytes;
            is_first = 0;
        } else {
            // Continuation packets
            size_t payload_space = 188 - 4; // 184
            if (remaining >= payload_space) {
                // Full packet (afc = 1, payload only)
                ts_pkt[3] = 0x10 | (s_video_cc++ & 0x0F);
                memcpy(&ts_pkt[4], pes_buf + offset, payload_space);
                offset += payload_space;
            } else {
                // Last packet with stuffing (afc = 3)
                size_t stuff_len = payload_space - remaining;
                ts_pkt[3] = 0x30 | (s_video_cc++ & 0x0F);
                if (stuff_len == 1) {
                    ts_pkt[4] = 0; // af_len = 0 (only af_len byte)
                    memcpy(&ts_pkt[5], pes_buf + offset, remaining);
                } else {
                    ts_pkt[4] = (uint8_t)(stuff_len - 1);
                    ts_pkt[5] = 0x00; // flags = 0
                    if (stuff_len > 2) {
                        memset(&ts_pkt[6], 0xFF, stuff_len - 2);
                    }
                    memcpy(&ts_pkt[4 + stuff_len], pes_buf + offset, remaining);
                }
                offset += remaining;
            }
        }

        out_cb(ts_pkt, user_data);
    }

    free(pes_buf);
}

static void file_ts_out(const uint8_t *ts_pkt, void *user_data) {
    FILE *fp = (FILE *)user_data;
    fwrite(ts_pkt, 1, 188, fp);
}

int main(void) {
    FILE *fp = fopen("/scratch/test_full.ts", "wb");
    if (!fp) return 1;

    uint8_t pat[188], pmt[188];
    build_pat_packet(pat);
    build_pmt_packet(pmt);
    fwrite(pat, 1, 188, fp);
    fwrite(pmt, 1, 188, fp);

    // Dummy SPS + PPS + IDR frame in Annex B
    // SPS: 00 00 00 01 67 42 C0 1E D9 00 A0 47 B8
    uint8_t dummy_frame[] = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x1E, 0xD9, 0x00, 0xA0, 0x47, 0xB8, // SPS
        0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x38, 0x80,                               // PPS
        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x00, 0x10, 0x20, 0x30, 0x40        // IDR slice
    };

    mux_h264_frame(dummy_frame, sizeof(dummy_frame), 0, file_ts_out, fp);
    fclose(fp);

    printf("Full MPEG-TS with PAT/PMT/PES written to /scratch/test_full.ts\n");
    return 0;
}
