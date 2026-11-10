/* Generated from original V19 transport functions; no vendor code. */
#define CLUSTER_MULTICAST_IP         "ff14::2:4"
#define CLUSTER_MULTICAST_PORT       42800
#define RTP_PACKET_SIZE              1140
#define TS_PACKET_SIZE               188
#define CLUSTER_PROGRAM_NUMBER       1   /* Matching MU0841 PMT program number. */
#define CLUSTER_PMT_PID              1000  /* 0x03E8 */
#define CLUSTER_VIDEO_PID            17    /* 0x0011 */
#define CLUSTER_PCR_PID              17    /* 0x0011 */
static int g_rtp_sock = -1;
static int g_rtp_owned;
static struct sockaddr_in6 g_cluster_dest_addr;
static uint16_t g_rtp_seq = 0;
static uint32_t g_rtp_ts = 0;

static uint8_t g_pat_cc = 0;
static uint8_t g_pmt_cc = 0;
static uint8_t g_video_cc = 0;
static int g_video_discontinuity=1;

static uint8_t g_rtp_buf[RTP_PACKET_SIZE];
static int g_rtp_ts_in_buf = 0;

static uint64_t g_total_frames_sent = 0;
static uint64_t g_sent_packets,g_send_errors;
static uint64_t g_total_bytes_sent = 0;

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
static int bind_cluster_source(int fd) {
    struct sockaddr_in6 source={0};source.sin6_family=AF_INET6;
    source.sin6_addr=in6addr_any;source.sin6_port=htons(42989);
    return bind(fd,(struct sockaddr *)&source,sizeof(source));
}
static void init_cluster_network(void) {
    if (g_rtp_sock >= 0) return;

    g_rtp_sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (g_rtp_sock < 0) {
        log_msg("NET_ERR", "Falha ao criar socket IPv6 RTP: %s", strerror(errno));
        return;
    }

    int hops = 5;
    setsockopt(g_rtp_sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops));

    int loop = 1;
    setsockopt(g_rtp_sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof(loop));

    int prio = 3;
    setsockopt(g_rtp_sock, SOL_SOCKET, SO_PRIORITY, &prio, sizeof(prio));

    unsigned int ifindex = if_nametoindex("eth0.5");
    if(ifindex==0 || setsockopt(g_rtp_sock,IPPROTO_IPV6,IPV6_MULTICAST_IF,&ifindex,sizeof(ifindex))!=0 || bind_cluster_source(g_rtp_sock)!=0) {
        log_msg("NET_ERR","Cluster interface/source-port setup failed: ifindex=%u errno=%d",ifindex,errno);
        close(g_rtp_sock);g_rtp_sock=-1;return;
    }
    g_rtp_owned=1;
    log_msg("NET","Cluster socket bound exclusively: sourcePort=42989 interface=eth0.5");

    memset(&g_cluster_dest_addr, 0, sizeof(g_cluster_dest_addr));
    g_cluster_dest_addr.sin6_family = AF_INET6;
    g_cluster_dest_addr.sin6_port = htons(CLUSTER_MULTICAST_PORT);
    inet_pton(AF_INET6, CLUSTER_MULTICAST_IP, &g_cluster_dest_addr.sin6_addr);

    log_msg("NET", "Socket RTP inicializado para [%s]:%d (ifidx=%u, Prio=%d, PID %d, PMT %d, Prog %d)",
            CLUSTER_MULTICAST_IP, CLUSTER_MULTICAST_PORT, ifindex, prio,
            CLUSTER_VIDEO_PID, CLUSTER_PMT_PID, CLUSTER_PROGRAM_NUMBER);
}
static void rtp_flush_buffer(int marker) {
    if (g_rtp_ts_in_buf == 0 || g_rtp_sock < 0) return;

    g_rtp_buf[0] = 0x80;
    g_rtp_buf[1] = 33; /* Matching factory profile: no per-frame RTP marker. */
    (void)marker;
    g_rtp_buf[2] = (uint8_t)(g_rtp_seq >> 8);
    g_rtp_buf[3] = (uint8_t)(g_rtp_seq & 0xFF);
    g_rtp_seq++;

    struct timespec rtp_now;
    if(clock_gettime(CLOCK_MONOTONIC,&rtp_now)==0)
        g_rtp_ts=(uint32_t)((uint64_t)rtp_now.tv_sec*1000+(uint64_t)rtp_now.tv_nsec/1000000);
    g_rtp_buf[4] = (uint8_t)((g_rtp_ts >> 24) & 0xFF);
    g_rtp_buf[5] = (uint8_t)((g_rtp_ts >> 16) & 0xFF);
    g_rtp_buf[6] = (uint8_t)((g_rtp_ts >> 8) & 0xFF);
    g_rtp_buf[7] = (uint8_t)(g_rtp_ts & 0xFF);

    /* MU0841 TransportStream constructor uses identifier 100000 (0x186a0).
     * Its RTP clock is monotonic milliseconds, separate from 90 kHz PES/PCR. */
    g_rtp_buf[8]=0x00;g_rtp_buf[9]=0x01;g_rtp_buf[10]=0x86;g_rtp_buf[11]=0xa0;

    /* Factory datagrams always contain six TS slots. Pad with proper null TS
     * packets rather than carrying uninitialized or prior frame bytes. */
    for(int i=g_rtp_ts_in_buf;i<6;i++) {
        uint8_t *null_packet=&g_rtp_buf[12+i*TS_PACKET_SIZE];
        memset(null_packet,0xff,TS_PACKET_SIZE);
        null_packet[0]=0x47;null_packet[1]=0x1f;null_packet[2]=0xff;null_packet[3]=0x10;
    }
    size_t payload_size = RTP_PACKET_SIZE;
    ssize_t sent = sendto(g_rtp_sock, g_rtp_buf, payload_size, 0,
                          (struct sockaddr *)&g_cluster_dest_addr, sizeof(g_cluster_dest_addr));
    if(sent==(ssize_t)payload_size) {
        g_total_bytes_sent+=(uint64_t)sent;g_sent_packets++;
        if(g_sent_packets==1)log_msg("RTP","First cluster UDP datagram sent: bytes=%zu",payload_size);
    } else {
        g_send_errors++;
        if(g_send_errors<=3 || g_send_errors%300==0)
            log_msg("RTP_ERR","Cluster send failed: errno=%d sent=%zd expected=%zu failures=%llu",errno,sent,payload_size,(unsigned long long)g_send_errors);
    }
    g_rtp_ts_in_buf = 0;
}
static void rtp_queue_ts_packet(const uint8_t *ts_pkt) {
    memcpy(&g_rtp_buf[12 + (g_rtp_ts_in_buf * TS_PACKET_SIZE)], ts_pkt, TS_PACKET_SIZE);
    g_rtp_ts_in_buf++;
    if (g_rtp_ts_in_buf == 6) {
        rtp_flush_buffer(0);
    }
}
static void emit_pat_packet(void) {
    uint8_t pkt[TS_PACKET_SIZE];
    memset(pkt, 0xFF, TS_PACKET_SIZE);

    pkt[0] = 0x47;
    pkt[1] = 0x40;
    pkt[2] = 0x00;
    pkt[3] = 0x10 | (g_pat_cc & 0x0F);
    g_pat_cc = (g_pat_cc + 1) & 0x0F;

    pkt[4] = 0x00;
    pkt[5] = 0x00;
    pkt[6] = 0xB0;
    pkt[7] = 0x0D;
    pkt[8] = 0x00; pkt[9] = 0x64; /* Factory transport-stream ID 100. */
    pkt[10] = 0xDB;
    pkt[11] = 0x00; pkt[12] = 0x00;

    pkt[13] = (uint8_t)((CLUSTER_PROGRAM_NUMBER >> 8) & 0xFF);
    pkt[14] = (uint8_t)(CLUSTER_PROGRAM_NUMBER & 0xFF);
    pkt[15] = 0xE0 | (uint8_t)((CLUSTER_PMT_PID >> 8) & 0x1F);
    pkt[16] = (uint8_t)(CLUSTER_PMT_PID & 0xFF);

    uint32_t crc = ts_crc32(&pkt[5], 12);
    pkt[17] = (uint8_t)((crc >> 24) & 0xFF);
    pkt[18] = (uint8_t)((crc >> 16) & 0xFF);
    pkt[19] = (uint8_t)((crc >> 8) & 0xFF);
    pkt[20] = (uint8_t)(crc & 0xFF);

    rtp_queue_ts_packet(pkt);
}
static void emit_pmt_packet(void) {
    uint8_t pkt[TS_PACKET_SIZE];
    memset(pkt, 0xFF, TS_PACKET_SIZE);

    pkt[0] = 0x47;
    pkt[1] = 0x40 | (uint8_t)((CLUSTER_PMT_PID >> 8) & 0x1F);
    pkt[2] = (uint8_t)(CLUSTER_PMT_PID & 0xFF);
    pkt[3] = 0x10 | (g_pmt_cc & 0x0F);
    g_pmt_cc = (g_pmt_cc + 1) & 0x0F;

    pkt[4] = 0x00;
    pkt[5] = 0x02;
    pkt[6] = 0xB0;
    pkt[7] = 0x12;
    pkt[8] = (uint8_t)((CLUSTER_PROGRAM_NUMBER >> 8) & 0xFF);
    pkt[9] = (uint8_t)(CLUSTER_PROGRAM_NUMBER & 0xFF);
    pkt[10] = 0xC3;
    pkt[11] = 0x00; pkt[12] = 0x00;

    pkt[13] = 0xE0 | (uint8_t)((CLUSTER_PCR_PID >> 8) & 0x1F);
    pkt[14] = (uint8_t)(CLUSTER_PCR_PID & 0xFF);
    pkt[15] = 0xF0; pkt[16] = 0x00;

    pkt[17] = 0x1B;
    pkt[18] = 0xE0 | (uint8_t)((CLUSTER_VIDEO_PID >> 8) & 0x1F);
    pkt[19] = (uint8_t)(CLUSTER_VIDEO_PID & 0xFF);
    pkt[20] = 0xF0; pkt[21] = 0x00;

    uint32_t crc = ts_crc32(&pkt[5], 17);
    pkt[22] = (uint8_t)((crc >> 24) & 0xFF);
    pkt[23] = (uint8_t)((crc >> 16) & 0xFF);
    pkt[24] = (uint8_t)((crc >> 8) & 0xFF);
    pkt[25] = (uint8_t)(crc & 0xFF);

    rtp_queue_ts_packet(pkt);
}
static void mux_and_stream_h264_frame(const uint8_t *frame_data, size_t frame_len, uint64_t pts_90k, int random_access, uint64_t pcr_90k) {
    if (!frame_data || frame_len == 0) return;
    /* RTP timestamp is assigned at datagram send time. */

    size_t pes_header_len = 14;
    size_t pes_total_len = pes_header_len + frame_len;
    uint8_t *pes_buf = (uint8_t *)malloc(pes_total_len);
    if (!pes_buf) return;

    pes_buf[0] = 0x00;
    pes_buf[1] = 0x00;
    pes_buf[2] = 0x01;
    pes_buf[3] = 0xE0;

    uint32_t pes_payload_len = (pes_total_len - 6 > 65535) ? 0 : (uint32_t)(pes_total_len - 6);
    pes_buf[4] = (uint8_t)((pes_payload_len >> 8) & 0xFF);
    pes_buf[5] = (uint8_t)(pes_payload_len & 0xFF);

    pes_buf[6] = 0x80;
    pes_buf[7] = 0x80;
    pes_buf[8] = 0x05;

    pes_buf[9]  = 0x20 | (uint8_t)(((pts_90k >> 30) & 0x07) << 1) | 0x01;
    pes_buf[10] = (uint8_t)((pts_90k >> 22) & 0xFF);
    pes_buf[11] = (uint8_t)(((pts_90k >> 15) & 0x7F) << 1) | 0x01;
    pes_buf[12] = (uint8_t)((pts_90k >> 7) & 0xFF);
    pes_buf[13] = (uint8_t)((pts_90k & 0x7F) << 1) | 0x01;

    memcpy(&pes_buf[pes_header_len], frame_data, frame_len);

    size_t offset = 0;
    int is_first_pkt = 1;

    while (offset < pes_total_len) {
        uint8_t ts_pkt[TS_PACKET_SIZE];
        memset(ts_pkt, 0xFF, TS_PACKET_SIZE);

        ts_pkt[0] = 0x47;
        ts_pkt[1] = (is_first_pkt ? 0x40 : 0x00) | (uint8_t)((CLUSTER_VIDEO_PID >> 8) & 0x1F);
        ts_pkt[2] = (uint8_t)(CLUSTER_VIDEO_PID & 0xFF);

        size_t remaining = pes_total_len - offset;

        if (is_first_pkt) {
            is_first_pkt = 0;
            ts_pkt[3] = 0x30 | (g_video_cc & 0x0F);
            g_video_cc = (g_video_cc + 1) & 0x0F;

            ts_pkt[4] = 0x07;
            ts_pkt[5] = (uint8_t)(0x10 | (random_access?0x40:0) | (g_video_discontinuity?0x80:0));
            if(g_video_discontinuity)log_msg("TS_SIGNAL","New source clock marked discontinuous; IDR randomAccess=%d",random_access!=0);
            g_video_discontinuity=0;

            uint64_t pcr_base = pcr_90k & ((1ULL << 33) - 1);
            ts_pkt[6] = (uint8_t)((pcr_base >> 25) & 0xFF);
            ts_pkt[7] = (uint8_t)((pcr_base >> 17) & 0xFF);
            ts_pkt[8] = (uint8_t)((pcr_base >> 9) & 0xFF);
            ts_pkt[9] = (uint8_t)((pcr_base >> 1) & 0xFF);
            ts_pkt[10] = (uint8_t)(((pcr_base & 0x01) << 7) | 0x7E);
            ts_pkt[11] = 0x00;

            size_t max_body = TS_PACKET_SIZE - 12;
            size_t copy_len = (remaining < max_body) ? remaining : max_body;
            memcpy(&ts_pkt[12], pes_buf + offset, copy_len);
            offset += copy_len;

            if (copy_len < max_body) {
                size_t padding = max_body - copy_len;
                ts_pkt[4] += (uint8_t)padding;
                memmove(&ts_pkt[12 + padding], &ts_pkt[12], copy_len);
                memset(&ts_pkt[12], 0xFF, padding);
            }
        } else {
            if (remaining >= TS_PACKET_SIZE - 4) {
                ts_pkt[3] = 0x10 | (g_video_cc & 0x0F);
                g_video_cc = (g_video_cc + 1) & 0x0F;
                memcpy(&ts_pkt[4], pes_buf + offset, TS_PACKET_SIZE - 4);
                offset += (TS_PACKET_SIZE - 4);
            } else {
                ts_pkt[3] = 0x30 | (g_video_cc & 0x0F);
                g_video_cc = (g_video_cc + 1) & 0x0F;
                size_t stuff_len = (TS_PACKET_SIZE - 4) - remaining;
                if (stuff_len == 1) {
                    ts_pkt[4] = 0x00;
                    memcpy(&ts_pkt[5], pes_buf + offset, remaining);
                } else {
                    ts_pkt[4] = (uint8_t)(stuff_len - 1);
                    ts_pkt[5] = 0x00;
                    if (stuff_len > 2) memset(&ts_pkt[6], 0xFF, stuff_len - 2);
                    memcpy(&ts_pkt[4 + stuff_len], pes_buf + offset, remaining);
                }
                offset += remaining;
            }
        }
        rtp_queue_ts_packet(ts_pkt);
    }

    free(pes_buf);
    /* RTP timestamp is assigned at datagram send time. */
    rtp_flush_buffer(1);
    g_total_frames_sent++;

    if (g_total_frames_sent <= 5 || g_total_frames_sent % 300 == 0) {
        log_msg("STREAM", "Stream AID: %llu frames transmitidos (%llu bytes) para [%s]:%d",
                (unsigned long long)g_total_frames_sent,
                (unsigned long long)g_total_bytes_sent,
                CLUSTER_MULTICAST_IP, CLUSTER_MULTICAST_PORT);
    }
}
