/*
 * altscreen_handler.c v6.1 — Genuine CarPlay AltScreen Gen-2 Cluster Streamer
 * =============================================================================
 * Arquitetura de Producao IRC-Parity (MIB3 MOI3 LG AArch64 / Linux glibc 2.17+):
 *
 * 1. Display 0 (Central Multimidia 784x468):
 *    - 100% nativo de fabrica (Cinemo / libCarPlayMediaFramework.so).
 *    - Nao sofre interferencia de AirPlayInfoArrayAddScreenDisplay nem de contadores.
 *
 * 2. Anuncio de Displays (RTSP GET /info):
 *    - Hook em AirPlayCopyServerInfo(session, properties, outErr) [Assinatura exata AArch64 de 3 parametros].
 *    - Habilita Bit 26 (AltScreen) em root 'features' (features |= 1ULL << 26).
 *    - Adiciona root 'enabledFeatures': ["altScreen", "viewAreas"].
 *    - Clona stock display[0], remove chaves de touchscreen/HID (primaryInputDevice, edid, etc.)
 *      e define:
 *        type = 111 (K_STREAM_ALT / Instrument Cluster)
 *        maxFPS = 30
 *        features = 0 (View-only, passivo MFi)
 *        widthPixels = 1280, heightPixels = 480
 *        widthPhysical = 320, heightPhysical = 120
 *        uuid = "b7e6c5a0-2222-4000-8000-000000000002"
 *        initialURL = "maps:/car/instrumentcluster"
 *        altScreenSuggestUIURLs = ["maps:/car/instrumentcluster", ...]
 *
 * 3. Captura Criptografica de Sessao MFi:
 *    - Hook em AES_CBCFrame_Init (libCoreUtils.so).
 *    - Captura a chave mestre de 128 bits negociada no handshake AirPlay.
 *
 * 4. Negociacao de Stream (RTSP SETUP):
 *    - Hook em AirPlayReceiverSessionSetup (libAirPlay.so).
 *    - Detecta solicitacao de Stream 111 (Instrument Cluster H.264).
 *    - Deriva a subchave e IV via AirPlay_DeriveAESKeySHA512ForScreen(master_key, streamConnectionID).
 *    - Inicializa AES-CTR para decriptografia dos quadros de video.
 *    - Selects an unused TCP port in the factory-permitted 49200..49499 range.
 *    - Returns only type 111 and the actual bound dataPort.
 *
 * 5. Receptor TCP & Pipeline MPEG-TS AID:
 *    - The auxiliary listener uses an available port without changing firewall rules.
 *    - Recebe pacotes de tela AirPlay (cabecalho 128B + payload).
 *    - K_SCREEN_VIDEO_CONFIG (opcode 1): extrai SPS e PPS da estrutura avcC.
 *    - K_SCREEN_VIDEO_FRAME (opcode 0): decripta com AES-CTR, converte AVCC para Annex B,
 *      empacota em MPEG-TS (PIDs 100/1000/17) e envia via UDP RTP multicast para
 *      o painel digital AID em [ff14::2:4]:42800 (eth0.5).
 * =============================================================================
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <poll.h>

#define LOG_FILE_DATA       "/data/lge/smartlink/carplay/carplay-handler-v15.log"
#define LOG_FILE_APPRW      "/lge/app_rw/smartlink/carplay/carplay-handler-v15.log"
#define LOG_FILE_FALLBACK   "/tmp/carplay-handler-v15.log"

static FILE *g_log_fp = NULL;
static int g_log_closed;
static int g_kmsg_fd = -1;
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;

static void log_msg(const char *tag, const char *fmt, ...) {
    int saved_errno=errno;
    pthread_mutex_lock(&g_log_mutex);
    if(g_log_closed){pthread_mutex_unlock(&g_log_mutex);errno=saved_errno;return;}

    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    /* journald provides a second path if SMACK denies our file locations. */
    fprintf(stderr, "[ALTSCREEN][%s] %s\n", tag, buffer);

    if (g_kmsg_fd < 0) {
        g_kmsg_fd = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
    }
    if (g_kmsg_fd >= 0) {
        char kmsg_buf[1152];
        snprintf(kmsg_buf, sizeof(kmsg_buf), "<3>[ALTSCREEN][%s] %s\n", tag, buffer);
        ssize_t wr = write(g_kmsg_fd, kmsg_buf, strlen(kmsg_buf));
        (void)wr;
    }

    if (!g_log_fp) {
        g_log_fp = fopen(LOG_FILE_DATA, "a");
        if (!g_log_fp) g_log_fp = fopen(LOG_FILE_APPRW, "a");
        if (!g_log_fp) g_log_fp = fopen(LOG_FILE_FALLBACK, "a");
    }
    if (g_log_fp) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
        fprintf(g_log_fp, "[%s] [%s] %s\n", time_str, tag, buffer);
        fflush(g_log_fp);
    }
    pthread_mutex_unlock(&g_log_mutex);
    errno=saved_errno;
}

/* =============================================================================
 * Constantes do Protocolo AltScreen & Hardware Demuxer AID
 * ============================================================================= */
#define K_STREAM_ALT                 111
#define ALT_UUID_DEFAULT             "b7e6c5a0-2222-4000-8000-000000000002"
#define ALT_URL_DEFAULT              "maps:/car/instrumentcluster"
#define ALT_URL_MAP                  "maps:/car/instrumentcluster/map"
#define ALT_URL_INSTRUCTIONCARD      "maps:/car/instrumentcluster/instructioncard"
#define AIRPLAY_FEATURE_BIT26        (1ULL << 26)

#define K_SCREEN_HDR_SIZE            128
#define K_SCREEN_VIDEO_FRAME         0
#define K_SCREEN_VIDEO_CONFIG        1
#define K_SCREEN_KEEPALIVE           2
#define MAX_SCREEN_BODY              (8u * 1024u * 1024u)

#define CLUSTER_MULTICAST_IP         "ff14::2:4"
#define CLUSTER_MULTICAST_PORT       42800
#define RTP_PACKET_SIZE              1140
#define TS_PACKET_SIZE               188
#define CLUSTER_PROGRAM_NUMBER       1   /* Matching MU0841 PMT program number. */
#define CLUSTER_PMT_PID              1000  /* 0x03E8 */
#define CLUSTER_VIDEO_PID            17    /* 0x0011 */
#define CLUSTER_PCR_PID              17    /* 0x0011 */

typedef struct {
    uint32_t bodySize;
    uint8_t opcode;
    uint8_t smallParam[3];
    uint8_t params[15 * 8];
} AirPlayScreenHeaderCompat;

/* =============================================================================
 * Simbolos Dinamicos CoreFoundation & CoreUtils
 * ============================================================================= */
typedef const void *CFTypeRef;
typedef int64_t (*CFArrayGetCount_fn)(const void *array);
typedef const void *(*CFArrayGetValueAtIndex_fn)(const void *array, int64_t idx);
typedef void *(*CFArrayCreateMutable_fn)(const void *alloc, int64_t capacity, const void *callbacks);
typedef void *(*CFArrayCreateMutableCopy_fn)(const void *alloc, int64_t capacity, const void *array);
typedef void (*CFArrayAppendValue_fn)(void *array, const void *val);
typedef int32_t (*CFArrayEnsureCreatedAndAppendCString_fn)(void **ioArray, const char *str, size_t len);

typedef const void *(*CFDictionaryGetValue_fn)(const void *dict, const void *key);
typedef void (*CFDictionarySetValue_fn)(void *dict, const void *key, const void *val);
typedef void (*CFDictionaryRemoveValue_fn)(void *dict, const void *key);
typedef void (*CFDictionarySetInt64_fn)(void *dict, const void *key, int64_t val);
typedef int64_t (*CFDictionaryGetInt64_fn)(const void *dict, const void *key, int32_t *outErr);
typedef void *(*CFDictionaryCreateMutable_fn)(const void *alloc, int64_t capacity, const void *kc, const void *vc);
typedef void *(*CFDictionaryCreateMutableCopy_fn)(const void *alloc, int64_t capacity, const void *dict);

typedef void *(*CFStringCreateWithCString_fn)(const void *alloc, const char *cStr, uint32_t encoding);
typedef int (*CFStringGetCString_fn)(const void *theString, char *buffer, int64_t bufferSize, uint32_t encoding);
typedef void (*CFRelease_fn)(CFTypeRef cf);
typedef void *(*CFRetain_fn)(CFTypeRef cf);

static CFArrayGetCount_fn s_CFArrayGetCount = NULL;
static CFArrayGetValueAtIndex_fn s_CFArrayGetValueAtIndex = NULL;
static CFArrayCreateMutable_fn s_CFArrayCreateMutable = NULL;
static CFArrayCreateMutableCopy_fn s_CFArrayCreateMutableCopy = NULL;
static CFArrayAppendValue_fn s_CFArrayAppendValue = NULL;
static CFArrayEnsureCreatedAndAppendCString_fn s_CFArrayEnsureCreatedAndAppendCString = NULL;

static CFDictionaryGetValue_fn s_CFDictionaryGetValue = NULL;
static CFDictionarySetValue_fn s_CFDictionarySetValue = NULL;
static CFDictionaryRemoveValue_fn s_CFDictionaryRemoveValue = NULL;
static CFDictionarySetInt64_fn s_CFDictionarySetInt64 = NULL;
static CFDictionaryGetInt64_fn s_CFDictionaryGetInt64 = NULL;
static CFDictionaryCreateMutable_fn s_CFDictionaryCreateMutable = NULL;
static CFDictionaryCreateMutableCopy_fn s_CFDictionaryCreateMutableCopy = NULL;

static CFStringCreateWithCString_fn s_CFStringCreateWithCString = NULL;
static CFStringGetCString_fn s_CFStringGetCString = NULL;
static CFRelease_fn s_CFRelease = NULL;
static CFRetain_fn s_CFRetain = NULL;

typedef int32_t (*CFPropertyListWriteToFilePath_fn)(const void *plist, const char *format, const char *path);
static CFPropertyListWriteToFilePath_fn s_CFPropertyListWriteToFilePath = NULL;
static pthread_mutex_t g_info_snapshot_lock=PTHREAD_MUTEX_INITIALIZER;
static unsigned g_info_saved;
static void save_info_snapshot(const void *info,const char *name,unsigned bit) {
    if(!s_CFPropertyListWriteToFilePath)return;
    int saved_errno=errno;
    pthread_mutex_lock(&g_info_snapshot_lock);
    if(!(g_info_saved & bit)) {
        char path[160],pending[176];
        snprintf(path,sizeof(path),"/data/lge/smartlink/carplay/map-v15-%s.plist",name);
        snprintf(pending,sizeof(pending),"%s.pending",path);
        int32_t result=s_CFPropertyListWriteToFilePath(info,"binary1",pending);
        if(result==0 && rename(pending,path)==0)g_info_saved|=bit;
        else if(result==0)result=-1;
        log_msg("SERVERINFO","%s snapshot status=%d",name,result);
    }
    pthread_mutex_unlock(&g_info_snapshot_lock);errno=saved_errno;
}
static const void *s_kCFLBooleanFalse = NULL;

static const void *s_kCFLArrayCallBacks = NULL;
static const void *s_kCFLDictKeyCallBacks = NULL;
static const void *s_kCFLDictValCallBacks = NULL;

static void init_cf_symbols(void) {
    if (s_CFArrayGetCount) return;
    s_CFArrayGetCount = (CFArrayGetCount_fn)dlsym(RTLD_DEFAULT, "CFArrayGetCount");
    s_CFArrayGetValueAtIndex = (CFArrayGetValueAtIndex_fn)dlsym(RTLD_DEFAULT, "CFArrayGetValueAtIndex");
    s_CFArrayCreateMutable = (CFArrayCreateMutable_fn)dlsym(RTLD_DEFAULT, "CFArrayCreateMutable");
    s_CFArrayCreateMutableCopy = (CFArrayCreateMutableCopy_fn)dlsym(RTLD_DEFAULT, "CFArrayCreateMutableCopy");
    s_CFArrayAppendValue = (CFArrayAppendValue_fn)dlsym(RTLD_DEFAULT, "CFArrayAppendValue");
    s_CFArrayEnsureCreatedAndAppendCString = (CFArrayEnsureCreatedAndAppendCString_fn)dlsym(
        RTLD_DEFAULT, "CFArrayEnsureCreatedAndAppendCString"
    );

    s_CFDictionaryGetValue = (CFDictionaryGetValue_fn)dlsym(RTLD_DEFAULT, "CFDictionaryGetValue");
    s_CFDictionarySetValue = (CFDictionarySetValue_fn)dlsym(RTLD_DEFAULT, "CFDictionarySetValue");
    s_CFDictionaryRemoveValue = (CFDictionaryRemoveValue_fn)dlsym(RTLD_DEFAULT, "CFDictionaryRemoveValue");
    s_CFDictionarySetInt64 = (CFDictionarySetInt64_fn)dlsym(RTLD_DEFAULT, "CFDictionarySetInt64");
    s_CFDictionaryGetInt64 = (CFDictionaryGetInt64_fn)dlsym(RTLD_DEFAULT, "CFDictionaryGetInt64");
    s_CFDictionaryCreateMutable = (CFDictionaryCreateMutable_fn)dlsym(RTLD_DEFAULT, "CFDictionaryCreateMutable");
    s_CFDictionaryCreateMutableCopy = (CFDictionaryCreateMutableCopy_fn)dlsym(RTLD_DEFAULT, "CFDictionaryCreateMutableCopy");

    s_CFStringCreateWithCString = (CFStringCreateWithCString_fn)dlsym(RTLD_DEFAULT, "CFStringCreateWithCString");
    s_CFStringGetCString = (CFStringGetCString_fn)dlsym(RTLD_DEFAULT, "CFStringGetCString");
    s_CFRelease = (CFRelease_fn)dlsym(RTLD_DEFAULT, "CFRelease");
    s_CFRetain = (CFRetain_fn)dlsym(RTLD_DEFAULT, "CFRetain");

    s_CFPropertyListWriteToFilePath = (CFPropertyListWriteToFilePath_fn)dlsym(
        RTLD_DEFAULT, "CFPropertyListWriteToFilePath"
    );
    void **false_slot = (void **)dlsym(RTLD_DEFAULT, "kCFLBooleanFalse");
    s_kCFLBooleanFalse = false_slot ? *false_slot : NULL;

    s_kCFLArrayCallBacks = dlsym(RTLD_DEFAULT, "kCFLArrayCallBacksCFLTypes");
    s_kCFLDictKeyCallBacks = dlsym(RTLD_DEFAULT, "kCFLDictionaryKeyCallBacksCFLTypes");
    s_kCFLDictValCallBacks = dlsym(RTLD_DEFAULT, "kCFLDictionaryValueCallBacksCFLTypes");
}

static void *s_cf(const char *s) {
    if (!s || !s_CFStringCreateWithCString) return NULL;
    return s_CFStringCreateWithCString(NULL, s, 0x08000100 /* UTF-8 */);
}

static void set_str(void *dict, const char *key, const char *val) {
    void *k = s_cf(key);
    void *v = s_cf(val);
    if (k && v && s_CFDictionarySetValue) {
        s_CFDictionarySetValue(dict, k, v);
    }
    if (v && s_CFRelease) s_CFRelease(v);
    if (k && s_CFRelease) s_CFRelease(k);
}

static void set_i64(void *dict, const char *key, int64_t val) {
    void *k = s_cf(key);
    if (k && s_CFDictionarySetInt64) {
        s_CFDictionarySetInt64(dict, k, val);
    }
    if (k && s_CFRelease) s_CFRelease(k);
}

static void remove_key(void *dict, const char *key) {
    void *k = s_cf(key);
    if (k && s_CFDictionaryRemoveValue) {
        s_CFDictionaryRemoveValue(dict, k);
    }
    if (k && s_CFRelease) s_CFRelease(k);
}

static void *dict_clone(void *dict) {
    if (!dict || !s_CFDictionaryCreateMutableCopy) return NULL;
    return s_CFDictionaryCreateMutableCopy(NULL, 0, dict);
}

static void *array_clone(void *arr) {
    if (!arr || !s_CFArrayCreateMutableCopy) return NULL;
    return s_CFArrayCreateMutableCopy(NULL, 0, arr);
}

/* =============================================================================
 * ViewAreas e enabledFeatures (AltScreen Apple CarPlay Specification)
 * ============================================================================= */
#include "layout-v15.h"

static int set_reference_enabled_features(void *dict, int enable_viewareas) {
    if (!dict || !s_CFArrayEnsureCreatedAndAppendCString || !s_CFDictionaryGetValue ||
        !s_CFDictionarySetValue || !s_CFRelease || !s_CFArrayGetCount ||
        !s_CFArrayGetValueAtIndex || !s_CFStringGetCString) return 0;
    void *k_enabled = s_cf("enabledFeatures");
    if (!k_enabled) return 0;
    void *stock = (void *)s_CFDictionaryGetValue(dict, k_enabled);
    void *enabled_features = stock ? array_clone(stock) : NULL;
    if (stock && !enabled_features) { s_CFRelease(k_enabled); return 0; }
    const char *wanted[] = {"altScreen", "viewAreas"};
    int ok = 1;
    for (int feature = 0; feature < (enable_viewareas ? 2 : 1); feature++) {
        int found = 0;
        int64_t count = enabled_features ? s_CFArrayGetCount(enabled_features) : 0;
        for (int64_t i = 0; i < count; i++) {
            char value[128];
            if (s_CFStringGetCString(s_CFArrayGetValueAtIndex(enabled_features, i), value,
                                    sizeof(value), 0x08000100) && !strcmp(value, wanted[feature])) {
                found = 1; break;
            }
        }
        if (!found && s_CFArrayEnsureCreatedAndAppendCString(&enabled_features,
                       wanted[feature], (size_t)-1) != 0) { ok = 0; break; }
    }
    /* Commit only the complete copy; allocation failure leaves stock untouched. */
    if (ok && enabled_features) s_CFDictionarySetValue(dict, k_enabled, enabled_features);
    s_CFRelease(k_enabled);
    if (enabled_features && s_CFRelease) s_CFRelease(enabled_features);
    log_msg("SERVERINFO", "enabledFeatures: factory list preserved, augmentation %s", ok ? "complete" : "aborted");
    return ok;
}

/* =============================================================================
 * Criptografia CarPlay MFi (AES-128-CTR Derivada de Sessao)
 * ============================================================================= */
typedef union {
    uint64_t align;
    unsigned char bytes[512];
} aes_ctr_storage_t;

/* MU0841 libAirPlay.so: the factory call site ignores x0 after derivation;
 * the routine has no status return. AES_CTR_Init does return an OSStatus. */
typedef void (*AirPlay_DeriveAESKeySHA512ForScreen_fn)(
    const uint8_t *master_key, size_t key_len, uint64_t connection_id, uint8_t *out_key, uint8_t *out_iv
);
typedef int32_t (*AES_CTR_Init_fn)(void *ctx, const uint8_t *key, const uint8_t *iv);
typedef int32_t (*AES_CTR_Update_fn)(void *ctx, const void *src, size_t len, void *dst);

static AirPlay_DeriveAESKeySHA512ForScreen_fn s_AirPlay_DeriveAESKeySHA512ForScreen = NULL;
static AES_CTR_Init_fn s_AES_CTR_Init = NULL;
static AES_CTR_Update_fn s_AES_CTR_Update = NULL;

static uint8_t g_master_key[16];
static volatile int g_master_valid = 0;
static void *g_master_session = NULL;
static pthread_mutex_t g_crypto_lock = PTHREAD_MUTEX_INITIALIZER;

static aes_ctr_storage_t g_alt_aes;
static volatile int g_alt_aes_valid = 0;
#include "homekit-screen-crypto.h"

static void init_crypto_symbols(void) {
    init_homekit_symbols();
    s_AirPlay_DeriveAESKeySHA512ForScreen = (AirPlay_DeriveAESKeySHA512ForScreen_fn)dlsym(
        RTLD_DEFAULT, "AirPlay_DeriveAESKeySHA512ForScreen"
    );
    s_AES_CTR_Init = (AES_CTR_Init_fn)dlsym(RTLD_DEFAULT, "AES_CTR_Init");
    s_AES_CTR_Update = (AES_CTR_Update_fn)dlsym(RTLD_DEFAULT, "AES_CTR_Update");
}

/* Hook em AES_CBCFrame_Init para interceptar a chave mestre de 128 bits no handshake AirPlay */
typedef int32_t (*AES_CBCFrame_Init_fn)(void *ctx, const uint8_t key[16], const uint8_t iv[16], int encrypt);
static AES_CBCFrame_Init_fn real_AES_CBCFrame_Init = NULL;

int32_t AES_CBCFrame_Init(void *ctx, const uint8_t key[16], const uint8_t iv[16], int encrypt) {
    if (!real_AES_CBCFrame_Init) {
        real_AES_CBCFrame_Init = (AES_CBCFrame_Init_fn)dlsym(RTLD_NEXT, "AES_CBCFrame_Init");
    }

    if (!real_AES_CBCFrame_Init) return -1;
    int32_t result = real_AES_CBCFrame_Init(ctx, key, iv, encrypt);
    /* This generic primitive is used by multiple callers. The session setter
     * below is the authoritative source for a session-bound legacy key. */
    log_msg("CRYPTO", "Native CBC initialization: encrypt=%d status=%d",encrypt!=0,result);

    return result;
}

/* Observability only: preserve native security setup and never log key bytes.
 * Signatures/return values verified against this MU0841 library's call sites. */
typedef int32_t (*session_security_fn)(void *, const uint8_t *, const uint8_t *);
static session_security_fn real_session_security = NULL;
int32_t AirPlayReceiverSessionSetSecurityInfo(void *session, const uint8_t *key, const uint8_t *iv) {
    if (!real_session_security)
        real_session_security = (session_security_fn)dlsym(RTLD_NEXT, "AirPlayReceiverSessionSetSecurityInfo");
    int32_t result = real_session_security ? real_session_security(session, key, iv) : -1;
    int native_errno=errno;
    pthread_mutex_lock(&g_crypto_lock);
    if(result==0 && session && key && iv) {
        memcpy(g_master_key,key,sizeof(g_master_key));g_master_valid=1;g_master_session=session;
    } else if(g_master_session==session) {
        secret_zero(g_master_key,sizeof(g_master_key));g_master_valid=0;g_master_session=NULL;
    }
    pthread_mutex_unlock(&g_crypto_lock);
    log_msg("SECURITY", "Native legacy session security: key_present=%d iv_present=%d status=%d",
            key != NULL, iv != NULL, result);
    errno=native_errno;
    return result;
}
typedef int32_t (*homekit_security_fn)(void *, void *);
static homekit_security_fn real_homekit_security = NULL;
int32_t AirPlayReceiverSessionSetHomeKitSecurityContext(void *session, void *context) {
    if (!real_homekit_security)
        real_homekit_security = (homekit_security_fn)dlsym(RTLD_NEXT, "AirPlayReceiverSessionSetHomeKitSecurityContext");
    int32_t result = real_homekit_security ? real_homekit_security(session, context) : -1;
    int native_errno=errno;
    if(result==0) {
        int cached=remember_pairing(session,context);
        log_msg("SECURITY", "HomeKit context cache: present=%d status=%d",context!=NULL,cached);
    }
    log_msg("SECURITY", "Native HomeKit session context: present=%d status=%d", context != NULL, result);
    errno=native_errno;
    return result;
}

/* =============================================================================
 * Camada 3: MPEG-TS & RTP Multicast Pipeline para [ff14::2:4]:42800 (eth0.5)
 * PIDs Fixos de Hardware do AID: Program 1, transport ID 100, PMT 1000, Video/PCR PID 17
 * ============================================================================= */
static int g_rtp_sock = -1;
static int g_rtp_owned;
static struct sockaddr_in6 g_cluster_dest_addr;
static uint16_t g_rtp_seq = 0;
static uint32_t g_rtp_ts = 0;

static uint8_t g_pat_cc = 0;
static uint8_t g_pmt_cc = 0;
static uint8_t g_video_cc = 0;

static uint8_t g_rtp_buf[RTP_PACKET_SIZE];
static int g_rtp_ts_in_buf = 0;

static uint64_t g_total_frames_sent = 0;
static uint64_t g_received_frames,g_authenticated_frames,g_invalid_frames,g_idr_frames,g_aud_frames;
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

/* Source port comes from the matching factory ShareServer call (0xa7ed).
 * Acquire it exclusively only after the authenticated encoder grant; do not
 * take over a live factory sender or silently choose an ephemeral source. */
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

static void mux_and_stream_h264_frame(const uint8_t *frame_data, size_t frame_len, uint64_t pts_90k) {
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
            ts_pkt[5] = 0x10;

            uint64_t pcr_base = pts_90k & ((1ULL << 33) - 1);
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

/* =============================================================================
 * Parser AVCC & Conversor Annex B
 * ============================================================================= */
static uint8_t g_sps_pps_buf[512];
static size_t g_sps_pps_len = 0;
static unsigned int g_nal_length_bytes = 4;

#include "video-observe-v14.h"

static void parse_and_store_avcc(const uint8_t *avcc, size_t len) {
    g_sps_pps_len = 0;
    if (!avcc || len < 7 || avcc[0] != 1) return;
    unsigned int length_bytes = (avcc[4] & 3) + 1;
    if (length_bytes == 3) return; /* reserved by AVCDecoderConfigurationRecord */
    uint8_t config[sizeof(g_sps_pps_buf)];

    size_t out_pos = 0;
    uint8_t num_sps = avcc[5] & 0x1F;
    size_t pos = 6;

    if (!num_sps) return;
    for (int i = 0; i < num_sps; i++) {
        if (pos + 2 > len) return;
        uint16_t sps_len = ((uint16_t)avcc[pos] << 8) | avcc[pos + 1];
        pos += 2;
        if (!sps_len || sps_len > len - pos) return;

        if (out_pos + 4 + sps_len <= sizeof(config)) {
            config[out_pos++] = 0x00;
            config[out_pos++] = 0x00;
            config[out_pos++] = 0x00;
            config[out_pos++] = 0x01;
            memcpy(&config[out_pos], &avcc[pos], sps_len);
            out_pos += sps_len;
        } else return;
        pos += sps_len;
    }

    if (pos < len) {
        uint8_t num_pps = avcc[pos++];
        if (!num_pps) return;
        for (int i = 0; i < num_pps; i++) {
            if (pos + 2 > len) return;
            uint16_t pps_len = ((uint16_t)avcc[pos] << 8) | avcc[pos + 1];
            pos += 2;
            if (!pps_len || pps_len > len - pos) return;

            if (out_pos + 4 + pps_len <= sizeof(config)) {
                config[out_pos++] = 0x00;
                config[out_pos++] = 0x00;
                config[out_pos++] = 0x00;
                config[out_pos++] = 0x01;
                memcpy(&config[out_pos], &avcc[pos], pps_len);
                out_pos += pps_len;
            } else return;
            pos += pps_len;
        }
    } else return;

    memcpy(g_sps_pps_buf, config, out_pos);
    g_nal_length_bytes = length_bytes;
    g_sps_pps_len = out_pos;
    video_observe_config(g_sps_pps_buf,g_sps_pps_len);
    log_msg("AVCC", "SPS/PPS extraidos com sucesso! Tamanho total: %zu bytes", g_sps_pps_len);
    /* Codec parameter sets describe decoder format, not picture/map pixels.
     * Bound the record so the next vehicle result can establish profile/level
     * and dimensions without capturing a video frame. */
    char codec_hex[129];size_t codec_n=g_sps_pps_len<64?g_sps_pps_len:64;
    for(size_t i=0;i<codec_n;i++)snprintf(codec_hex+i*2,3,"%02x",g_sps_pps_buf[i]);
    codec_hex[codec_n*2]=0;
    log_msg("AVCC_META","profile=%u compatibility=%u level=%u annexbParameterSets=%s",avcc[1],avcc[2],avcc[3],codec_hex);
}

#include "frame-gate-v15.h"
#include "deferred-video-v9.h"
static int validate_h264_frame(const uint8_t *data,size_t len,size_t *nals,int *is_keyframe) {
    if (!data || !len || len > MAX_SCREEN_BODY || !g_sps_pps_len) return -1;
    /* Validate the entire access unit before emitting anything. In particular,
     * zero-length NALs must not read data[len], and partial frames are rejected. */
    size_t checked = 0;*nals=0;
    *is_keyframe=0;
    while (checked < len) {
        if (len - checked < g_nal_length_bytes) return -1;
        uint32_t n = 0;
        for (unsigned int i = 0; i < g_nal_length_bytes; i++) n = (n << 8) | data[checked++];
        if (!n || n > len - checked) return -1;
        if ((data[checked] & 0x1F) == 5) *is_keyframe = 1;
        checked += n;
        (*nals)++;
    }
    return 0;
}
static int emit_h264_frame(const uint8_t *data,size_t len,uint64_t timestamp) {
    size_t nals;int is_keyframe;
    if(validate_h264_frame(data,len,&nals,&is_keyframe)!=0)return -1;
    init_cluster_network();
    if(g_rtp_sock<0)return -1;

    static int s_first_frame = 0;
    if (!s_first_frame) {
        s_first_frame = 1;
        log_msg("STREAM111", ">>> PRIMEIRO QUADRO DO CLUSTER RECEBIDO (%zu bytes, ts=%llu)! Transmitindo...",
                len, (unsigned long long)timestamp);
    }

    size_t annexb_capacity = len + nals * (4 - g_nal_length_bytes) + g_sps_pps_len;
    uint8_t *annexb_frame = (uint8_t *)malloc(annexb_capacity);
    if (!annexb_frame) return 0;

    size_t annexb_pos = 0;
    if (is_keyframe) {
        memcpy(annexb_frame, g_sps_pps_buf, g_sps_pps_len);
        annexb_pos = g_sps_pps_len;
    }

    size_t in_pos = 0;
    while (in_pos < len) {
        uint32_t nal_len = 0;
        for (unsigned int i = 0; i < g_nal_length_bytes; i++) nal_len = (nal_len << 8) | data[in_pos++];

        annexb_frame[annexb_pos++] = 0x00;
        annexb_frame[annexb_pos++] = 0x00;
        annexb_frame[annexb_pos++] = 0x00;
        annexb_frame[annexb_pos++] = 0x01;

        memcpy(&annexb_frame[annexb_pos], &data[in_pos], nal_len);
        annexb_pos += nal_len;

        in_pos += nal_len;
    }

    static int s_pat_counter = 0;
    if (is_keyframe || (++s_pat_counter % 30 == 0)) {
        emit_pat_packet();
        emit_pmt_packet();
    }

    uint64_t pts_90k = timestamp ? (timestamp * 90 / 1000) : (g_total_frames_sent * 3000);
    mux_and_stream_h264_frame(annexb_frame, annexb_pos, pts_90k);

    free(annexb_frame);
    return 0;
}

static void aux_refresh_invalidate_locked(void);
static void close_cluster_socket(void) {
    if(g_rtp_owned && g_rtp_sock>=0)close(g_rtp_sock);
    g_rtp_sock=-1;g_rtp_owned=0;
}
static void release_cluster_output_if_unleased(const char *directory) {
    char grant[512];snprintf(grant,sizeof(grant),"%s/" HANDOFF_GRANT_FILE,directory);
    if(g_rtp_owned && !handoff_file_valid(grant)) {
        close_cluster_socket();deferred_video_clear();end_frame_gate_at(directory);
        pthread_mutex_lock(&g_crypto_lock);aux_refresh_invalidate_locked();pthread_mutex_unlock(&g_crypto_lock);
        log_msg("HANDOFF","Output lease revoked; custom UDP port released");
    }
}

static int process_h264_frame(const uint8_t *data,size_t len,uint64_t timestamp) {
    size_t nals;int idr;
    if(validate_h264_frame(data,len,&nals,&idr)!=0) {
        g_invalid_frames++;
        if(g_invalid_frames<=3)log_msg("H264_ERR","Rejected malformed access unit: bytes=%zu",len);
        return -1;
    }
    video_observe_frame(data,len,g_nal_length_bytes,g_authenticated_frames);
    if(idr)g_idr_frames++;
    uint32_t nal_mask=0;size_t nal_pos=0;
    while(nal_pos<len){uint32_t length=0;for(unsigned i=0;i<g_nal_length_bytes;i++)length=(length<<8)|data[nal_pos++];nal_mask|=1u<<(data[nal_pos]&31);nal_pos+=length;}
    if(nal_mask&(1u<<9))g_aud_frames++;
    if(g_authenticated_frames<=3 || idr || g_authenticated_frames%300==0)
        log_msg("H264_META","accessUnitBytes=%zu nalCount=%zu nalTypeMask=0x%x idr=%d aud=%d",len,nals,nal_mask,idr,(nal_mask&(1u<<9))!=0);
#if !defined(ALTSCREEN_TEST) || defined(ALTSCREEN_TEST_WITH_GATE)
    return deferred_video_submit(g_frame_directory,data,len,timestamp,idr,g_alt_chacha_valid,emit_h264_frame);
#else
    return emit_h264_frame(data,len,timestamp);
#endif
}

/* =============================================================================
 * Servidor Receptor TCP Stream 111
 * ============================================================================= */
static int g_alt_listen_fd = -1;
static int g_alt_listen_port;
static int g_alt_client_fd = -1;
static pthread_t g_alt_thread;
static int g_alt_thread_running = 0; /* protected by g_crypto_lock */
static int g_alt_thread_created = 0;
static pthread_mutex_t g_receiver_lock = PTHREAD_MUTEX_INITIALIZER;

static int receiver_active(void) {
    pthread_mutex_lock(&g_crypto_lock);
    int active = g_alt_thread_running;
    pthread_mutex_unlock(&g_crypto_lock);
    return active;
}

#include "aux-refresh-v15.h"

/* MU0841's factory screen receiver reads a 64-bit timestamp at header offset 8
 * and passes it to its NTP time synchronizer. Preserve those relative intervals
 * instead of assuming every incoming access unit arrives at exactly 30 fps. */
static struct {
    int started, ntp_started;
    uint64_t samples,clamps,rebases,raw_repeats,previous_raw,last_sample,max_drift_us;
    uint64_t monotonic_start, ntp_start, ntp_offset_us, last_us;
} g_video_clock;

static uint64_t screen_timestamp_us(const AirPlayScreenHeaderCompat *header) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return g_video_clock.last_us;
    uint64_t monotonic = (uint64_t)now.tv_sec * 1000000 + (uint64_t)now.tv_nsec / 1000;
    if (!g_video_clock.started) {
        g_video_clock.started = 1;
        g_video_clock.monotonic_start = monotonic;
    }
    uint64_t elapsed = monotonic - g_video_clock.monotonic_start;
    uint64_t arrival_elapsed=elapsed;
    uint64_t ntp;
    memcpy(&ntp, header->params, sizeof(ntp));
    if(g_video_clock.samples && ntp==g_video_clock.previous_raw)g_video_clock.raw_repeats++;
    g_video_clock.previous_raw=ntp;
    if (ntp) {
        if (!g_video_clock.ntp_started) {
            g_video_clock.ntp_started = 1;
            g_video_clock.ntp_start = ntp;
            g_video_clock.ntp_offset_us = elapsed;
        }
        uint64_t delta = ntp - g_video_clock.ntp_start;
        if ((delta >> 32) > 86400) {
            /* Clock moved backwards or discontinuously: rebase on arrival time. */
            g_video_clock.ntp_start = ntp;
            g_video_clock.ntp_offset_us = elapsed;
            g_video_clock.rebases++;
            delta = 0;
        }
        elapsed = g_video_clock.ntp_offset_us + (delta >> 32) * 1000000 +
            ((delta & 0xffffffffULL) * 1000000 >> 32);
    }
    uint64_t timestamp = 1000000 + elapsed;
    if (timestamp < g_video_clock.last_us){timestamp=g_video_clock.last_us;g_video_clock.clamps++;}
    uint64_t drift=elapsed>arrival_elapsed?elapsed-arrival_elapsed:arrival_elapsed-elapsed;
    if(drift>g_video_clock.max_drift_us)g_video_clock.max_drift_us=drift;
    g_video_clock.samples++;
    if(g_video_clock.samples<=3 || g_video_clock.samples%300==0)
        log_msg("VIDEO_CLOCK","samples=%llu arrivalElapsedUs=%llu sourceElapsedUs=%llu outputStepUs=%llu maxDriftUs=%llu rawRepeats=%llu clamps=%llu rebases=%llu (relative timing only)",
            (unsigned long long)g_video_clock.samples,(unsigned long long)arrival_elapsed,(unsigned long long)elapsed,
            (unsigned long long)(timestamp-g_video_clock.last_sample),(unsigned long long)g_video_clock.max_drift_us,
            (unsigned long long)g_video_clock.raw_repeats,(unsigned long long)g_video_clock.clamps,(unsigned long long)g_video_clock.rebases);
    g_video_clock.last_sample=timestamp;
    g_video_clock.last_us = timestamp;
    return timestamp;
}

static int read_exact(int fd, void *buf, size_t len) {
    size_t total = 0;
    uint8_t *p = (uint8_t *)buf;
    while (total < len && receiver_active()) {
#if !defined(ALTSCREEN_TEST) || defined(ALTSCREEN_TEST_WITH_GATE)
        release_cluster_output_if_unleased(g_frame_directory);
#endif
#if !defined(ALTSCREEN_TEST) || defined(ALTSCREEN_TEST_WITH_REFRESH)
        aux_refresh_tick();
#endif
#if !defined(ALTSCREEN_TEST) || defined(ALTSCREEN_TEST_WITH_GATE)
        if(deferred_video_flush(g_frame_directory,emit_h264_frame)<0)
            log_msg("HANDOFF","Pending authenticated video could not be released");
#endif
        struct pollfd event={.fd=fd,.events=POLLIN};
        int ready=poll(&event,1,100);
        if(ready==0)continue;
        if(ready<0){if(errno==EINTR)continue;return -1;}
        ssize_t n = read(fd, p + total, len - total);
        if (n <= 0) {
            if (n < 0 && (errno == EINTR || errno == EAGAIN)) continue;
            return (int)n;
        }
        total += (size_t)n;
    }
    return total==len ? 1:0;
}

static void *alt_receiver_thread(void *arg) {
    int listen_fd = (int)(intptr_t)arg;
    log_msg("RECV111", "Waiting for auxiliary TCP connection on port %d",g_alt_listen_port);

    int c;
    do { c=accept(listen_fd,NULL,NULL); } while(c<0 && errno==EINTR && receiver_active());
    if (c < 0) {
        if(!receiver_active())log_msg("RECV111","Listener closed during session teardown");
        else log_msg("RECV111", "Auxiliary accept failed: %s", strerror(errno));
        return NULL;
    }

    pthread_mutex_lock(&g_crypto_lock);
    if (!g_alt_thread_running) {
        close(c);
        pthread_mutex_unlock(&g_crypto_lock);
        return NULL;
    }
    g_alt_client_fd = c;
    pthread_mutex_unlock(&g_crypto_lock);
    log_msg("RECV111", ">>> IPHONE CONECTADO NO STREAM 111 (TCP fd=%d)! Recebendo quadros H.264...", c);

    while (receiver_active()) {
        AirPlayScreenHeaderCompat h;
        uint8_t *body = NULL;

        int rr = read_exact(c, &h, sizeof(h));
        if (rr <= 0) {
            log_msg("RECV111", "Conexao TCP encerrada pelo iPhone (header rr=%d)", rr);
            break;
        }

        if (h.bodySize > MAX_SCREEN_BODY) {
            log_msg("RECV111", "bodySize invalido (%u bytes)", h.bodySize);
            break;
        }

        if (h.bodySize > 0) {
            body = (uint8_t *)malloc(h.bodySize);
            if (!body) break;
            rr = read_exact(c, body, h.bodySize);
            if (rr <= 0) {
                free(body);
                break;
            }
        }

        switch (h.opcode) {
            case K_SCREEN_VIDEO_CONFIG:
                if (body && h.bodySize > 0) {
                    log_msg("RECV111", "VideoConfig recebido: bodySize=%u bytes", h.bodySize);
                    deferred_video_clear();g_handoff_started=0;
                    parse_and_store_avcc(body, h.bodySize);
                }
                break;

            case K_SCREEN_VIDEO_FRAME:
                if (body && h.bodySize > 0) {
                    pthread_mutex_lock(&g_crypto_lock);
                    g_received_frames++;
                    size_t plaintext_size=h.bodySize;
                    int decrypt_ok;
                    if(g_alt_chacha_valid)decrypt_ok=decrypt_homekit_frame(&h,sizeof(h),body,&plaintext_size)==0;
                    else decrypt_ok = g_alt_aes_valid && s_AES_CTR_Update &&
                        s_AES_CTR_Update(&g_alt_aes, body, h.bodySize, body) == 0;
                    pthread_mutex_unlock(&g_crypto_lock);
                    if (decrypt_ok) {
                        g_authenticated_frames++;
                        if(g_authenticated_frames<=3)log_msg("CRYPTO","Authenticated video access unit: bytes=%zu",plaintext_size);
                        if(process_h264_frame(body,plaintext_size,screen_timestamp_us(&h))<0 && !frame_gate_revoked_at(g_frame_directory))
                            log_msg("VIDEO_ERR","Authenticated access unit was not accepted by output pipeline");
                    }
                    else {
                        log_msg("CRYPTO", "Frame rejected: security state, decryption or authentication failed; closing receiver");
                        free(body);body=NULL;goto receiver_done;
                    }
                }
                break;

            case K_SCREEN_KEEPALIVE:
                break;

            default:
                break;
        }

        if (body) {
            free(body);
        }
    }

receiver_done:
    pthread_mutex_lock(&g_crypto_lock);
    unsigned refresh_attempts=g_aux_attempts,refresh_results=g_aux_results,refresh_errors=g_aux_errors;
    aux_refresh_invalidate_locked();pthread_mutex_unlock(&g_crypto_lock);
    close_cluster_socket(); /* Release port BEFORE the monitor can restart native output. */
    deferred_video_clear();
#if !defined(ALTSCREEN_TEST) || defined(ALTSCREEN_TEST_WITH_GATE)
    end_frame_gate_at(g_frame_directory);
#endif
    log_msg("SUMMARY","received=%llu authenticated=%llu invalid=%llu idr=%llu aud=%llu muxed=%llu udpPackets=%llu udpErrors=%llu",
        (unsigned long long)g_received_frames,(unsigned long long)g_authenticated_frames,
        (unsigned long long)g_invalid_frames,(unsigned long long)g_idr_frames,(unsigned long long)g_aud_frames,
        (unsigned long long)g_total_frames_sent,(unsigned long long)g_sent_packets,(unsigned long long)g_send_errors);
    log_msg("AUX_REFRESH","attempts=%u submissions=%u errors=%u",refresh_attempts,refresh_results,refresh_errors);
    log_msg("RECV111", "Stream 111 desconectado.");
    pthread_mutex_lock(&g_crypto_lock);
    if (g_alt_client_fd == c) g_alt_client_fd = -1;
    close(c);
    pthread_mutex_unlock(&g_crypto_lock);
    return NULL;
}

static int bind_listener_stream111(int port) {
    int fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (fd >= 0) {
        int one = 1;
        int off = 0;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        int dual_status=setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));

        struct sockaddr_in6 sa6;
        memset(&sa6, 0, sizeof(sa6));
        sa6.sin6_family = AF_INET6;
        sa6.sin6_addr = in6addr_any;
        sa6.sin6_port = htons((uint16_t)port);

        if (dual_status==0 && bind(fd, (struct sockaddr *)&sa6, sizeof(sa6)) == 0 && listen(fd, 2) == 0) {
            log_msg("NET","Auxiliary listener family=IPv6 dual-stack requested");
            return fd;
        }
        close(fd);
    }

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) return -1;

    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in sa4;
    memset(&sa4, 0, sizeof(sa4));
    sa4.sin_family = AF_INET;
    sa4.sin_addr.s_addr = htonl(INADDR_ANY);
    sa4.sin_port = htons((uint16_t)port);

    if (bind(fd, (struct sockaddr *)&sa4, sizeof(sa4)) < 0 || listen(fd, 2) < 0) {
        close(fd);
        return -1;
    }
    log_msg("NET","Auxiliary listener family=IPv4 fallback");
    return fd;
}

/* The factory IPv6 filter permits TCP destination/source ports 49200..49499.
 * Port zero can select a filtered ephemeral port. Never take the primary screen
 * port or modify firewall policy; bind exclusively and skip occupied ports.
 * Exhaustion fails the auxiliary setup while leaving native CarPlay intact. */
#define ALT_TCP_PORT_FIRST 49200
#define ALT_TCP_PORT_LAST  49499
static int bind_permitted_listener_stream111(void) {
    for (int port=ALT_TCP_PORT_FIRST;port<=ALT_TCP_PORT_LAST;port++) {
        int fd=bind_listener_stream111(port);
        if(fd>=0)return fd;
    }
    log_msg("NET","No auxiliary listener available in permitted range %d..%d",
            ALT_TCP_PORT_FIRST,ALT_TCP_PORT_LAST);
    return -1;
}

/* Caller serializes start/stop. The worker owns the accepted fd; stop only
 * shuts it down, then joins before closing the listening fd or resetting state. */
static void stop_alt_receiver_locked(void) {
    pthread_mutex_lock(&g_crypto_lock);
    g_alt_thread_running = 0;
    aux_refresh_invalidate_locked();
    if (g_alt_client_fd >= 0) shutdown(g_alt_client_fd, SHUT_RDWR);
    if (g_alt_listen_fd >= 0) shutdown(g_alt_listen_fd, SHUT_RDWR);
    pthread_mutex_unlock(&g_crypto_lock);
    if (g_alt_thread_created) {
        pthread_join(g_alt_thread, NULL);
        g_alt_thread_created = 0;
    }
    pthread_mutex_lock(&g_crypto_lock);
    close_cluster_socket();
    if (g_alt_listen_fd >= 0) close(g_alt_listen_fd);
    g_alt_listen_fd = -1;
    g_alt_listen_port=0;
    g_alt_aes_valid = 0;
    memset(&g_alt_aes, 0, sizeof(g_alt_aes));
    reset_homekit_stream();
    g_alt_session=NULL;
    g_handoff_started=0;deferred_video_clear();
    g_received_frames=g_authenticated_frames=g_invalid_frames=g_idr_frames=g_aud_frames=0;
    g_total_frames_sent=g_total_bytes_sent=g_sent_packets=g_send_errors=0;
    g_sps_pps_len = 0;
    memset(&g_video_clock, 0, sizeof(g_video_clock));video_observe_reset();
    pthread_mutex_unlock(&g_crypto_lock);
}

static int start_alt_receiver(void *session,uint64_t connection_id) {
    pthread_mutex_lock(&g_receiver_lock);
    stop_alt_receiver_locked();
    pthread_mutex_lock(&g_crypto_lock);
    uint8_t key[16] = {0}, iv[16] = {0};
    int master_present = g_master_valid;
    int derive_present = s_AirPlay_DeriveAESKeySHA512ForScreen != NULL;
    int init_present = s_AES_CTR_Init != NULL;
    int update_present = s_AES_CTR_Update != NULL;
    int32_t init_status = -1;
    void *pairing=pairing_for_session(session);
    int ready;
    if(pairing) {
        init_status=prepare_homekit_stream(pairing,connection_id);
        ready=init_status==0;
    } else {
        ready = g_master_valid && g_master_session==session && s_AirPlay_DeriveAESKeySHA512ForScreen && s_AES_CTR_Init && s_AES_CTR_Update;
        if (ready) s_AirPlay_DeriveAESKeySHA512ForScreen(g_master_key,sizeof(g_master_key),connection_id,key,iv);
        if (ready) { init_status = s_AES_CTR_Init(&g_alt_aes,key,iv); ready = init_status == 0; }
    }
    memset(key, 0, sizeof(key));
    memset(iv, 0, sizeof(iv));
    if (!ready) {
        memset(&g_alt_aes, 0, sizeof(g_alt_aes));
        pthread_mutex_unlock(&g_crypto_lock);
        pthread_mutex_unlock(&g_receiver_lock);
        log_msg("RECV111", "Refusing stream: AES prerequisites master=%d derive=%d init=%d update=%d init_status=%d",
                master_present, derive_present, init_present, update_present, init_status);
        log_msg("RECV111", "HomeKit path: context=%d derive=%d cipher=%d status=%d",pairing!=NULL,
                s_pairing_derive!=NULL,s_chacha_init && s_chacha_aad && s_chacha_decrypt && s_chacha_verify,init_status);
        return -1;
    }
    g_alt_aes_valid = !pairing;
    g_alt_session=session;
    log_msg("CRYPTO", "AltScreen security ready: %s",pairing ? "HomeKit ChaCha20-Poly1305" : "legacy AES-CTR");
    int port = 0;
    g_alt_listen_fd = bind_permitted_listener_stream111();
    if (g_alt_listen_fd < 0) {
        g_alt_aes_valid = 0;
        reset_homekit_stream();
        g_alt_session=NULL;
        pthread_mutex_unlock(&g_crypto_lock);
        pthread_mutex_unlock(&g_receiver_lock);
        log_msg("RECV111", "Falha ao vincular porta TCP %d: %s", port, strerror(errno));
        return -1;
    }
    struct sockaddr_storage bound;socklen_t bound_len=sizeof(bound);
    if(getsockname(g_alt_listen_fd,(struct sockaddr *)&bound,&bound_len)==0) {
        if(bound.ss_family==AF_INET6)port=ntohs(((struct sockaddr_in6 *)&bound)->sin6_port);
        else if(bound.ss_family==AF_INET)port=ntohs(((struct sockaddr_in *)&bound)->sin_port);
    }
    if(port<=0 || port>65535) {
        close(g_alt_listen_fd);g_alt_listen_fd=-1;g_alt_aes_valid=0;reset_homekit_stream();g_alt_session=NULL;
        pthread_mutex_unlock(&g_crypto_lock);pthread_mutex_unlock(&g_receiver_lock);
        log_msg("RECV111","Could not establish the actual auxiliary listener port");return -1;
    }
    g_alt_listen_port=port;

    g_alt_thread_running = 1;
    if (pthread_create(&g_alt_thread, NULL, alt_receiver_thread, (void *)(intptr_t)g_alt_listen_fd) != 0) {
        close(g_alt_listen_fd);
        g_alt_listen_fd = -1;
        g_alt_thread_running = 0;
        g_alt_aes_valid = 0;
        reset_homekit_stream();
        g_alt_session=NULL;
        pthread_mutex_unlock(&g_crypto_lock);
        pthread_mutex_unlock(&g_receiver_lock);
        log_msg("RECV111", "Falha ao criar alt_receiver_thread");
        return -1;
    }

    g_alt_thread_created = 1;
    pthread_mutex_unlock(&g_crypto_lock);
    pthread_mutex_unlock(&g_receiver_lock);
    log_msg("RECV111", "Servidor TCP Stream 111 ouvindo na porta %d (cid=%llu)", port, (unsigned long long)connection_id);
    return port;
}

static void stop_alt_receiver(void) {
    pthread_mutex_lock(&g_receiver_lock);
    stop_alt_receiver_locked();
    pthread_mutex_unlock(&g_receiver_lock);
}

/* =============================================================================
 * Inspecao de SETUP do CarPlay: Detecao de Stream 111
 * ============================================================================= */
static int stream_type(const void *d) {
    if (!d || !s_CFDictionaryGetInt64) return -1;
    void *k = s_cf("type");
    if (!k) return -1;
    int32_t err = 0;
    int64_t v = s_CFDictionaryGetInt64(d, k, &err);
    if (s_CFRelease) s_CFRelease(k);
    return err ? -1 : (int)v;
}

static uint64_t stream_connection_id(void *d) {
    if (!d || !s_CFDictionaryGetInt64) return 0;
    void *k = s_cf("streamConnectionID");
    if (!k) return 0;
    int32_t err = 0;
    int64_t v = s_CFDictionaryGetInt64(d, k, &err);
    if (s_CFRelease) s_CFRelease(k);
    return err ? 0 : (uint64_t)v;
}

static void *get_streams(void *dict) {
    if (!dict || !s_CFDictionaryGetValue) return NULL;
    void *k = s_cf("streams");
    if (!k) return NULL;
    void *a = (void *)s_CFDictionaryGetValue(dict, k);
    if (s_CFRelease) s_CFRelease(k);
    return a;
}

static int contains_stream111(void *request, void **outDesc, int *outOtherCount) {
    void *a = get_streams(request);
    int found = 0, other = 0;
    if (outDesc) *outDesc = NULL;
    if (!a || !s_CFArrayGetCount) {
        if (outOtherCount) *outOtherCount = 0;
        return 0;
    }

    int64_t n = s_CFArrayGetCount(a);
    for (int64_t i = 0; i < n; i++) {
        void *sd = (void *)s_CFArrayGetValueAtIndex(a, i);
        if (stream_type(sd) == K_STREAM_ALT) {
            found = 1;
            if (outDesc && !*outDesc) *outDesc = sd;
        } else {
            other++;
        }
    }
    if (outOtherCount) *outOtherCount = other;
    return found;
}

static void log_stream_ports(const char *label,void *response) {
    if(!s_CFArrayGetValueAtIndex)return;
    void *streams=get_streams(response);
    int64_t count=streams && s_CFArrayGetCount ? s_CFArrayGetCount(streams) : 0;
    log_msg("SETUP_PORTS","%s streamCount=%lld",label,(long long)count);
    for(int64_t i=0;i<count && i<8;i++) {
        void *entry=(void *)s_CFArrayGetValueAtIndex(streams,i);void *key=s_cf("dataPort");int32_t error=-1;
        int64_t port=key && s_CFDictionaryGetInt64 ? s_CFDictionaryGetInt64(entry,key,&error) : -1;
        if(key && s_CFRelease)s_CFRelease(key);
        log_msg("SETUP_PORTS","%s index=%lld type=%d dataPort=%lld portStatus=%d",label,(long long)i,stream_type(entry),(long long)port,error);
    }
}
static int append_alt_setup_response(void *response, void *requested, int data_port) {
    if (!response || !requested || data_port<=0 || data_port>65535 || !s_CFDictionaryCreateMutable ||
        !s_CFArrayCreateMutable || !s_CFArrayGetCount || !s_CFArrayGetValueAtIndex ||
        !s_CFArrayAppendValue || !s_CFDictionarySetValue || !s_CFDictionaryGetInt64 ||
        !s_CFDictionarySetInt64 || !s_CFRelease)return 0;

    void *kstreams = s_cf("streams");
    void *old = (kstreams && s_CFDictionaryGetValue) ? (void *)s_CFDictionaryGetValue(response, kstreams) : NULL;
    void *a = s_CFArrayCreateMutable(NULL, 0, s_kCFLArrayCallBacks);
    void *sd = s_CFDictionaryCreateMutable(NULL,0,s_kCFLDictKeyCallBacks,s_kCFLDictValCallBacks);
    int success=0;

    if (a && sd) {
        set_i64(sd, "dataPort", data_port);
        set_i64(sd,"type",K_STREAM_ALT);
        void *kport=s_cf("dataPort");int32_t err=-1;
        int64_t actual=kport ? s_CFDictionaryGetInt64(sd,kport,&err) : -1;
        if(kport)s_CFRelease(kport);
        if(err==0 && actual==data_port && stream_type(sd)==K_STREAM_ALT && kstreams) {
            int64_t count=old ? s_CFArrayGetCount(old) : 0;
            for(int64_t i=0;i<count;i++) {
                void *entry=(void *)s_CFArrayGetValueAtIndex(old,i);
                if(stream_type(entry)!=K_STREAM_ALT)s_CFArrayAppendValue(a,entry);
            }
            s_CFArrayAppendValue(a,sd);
            s_CFDictionarySetValue(response,kstreams,a);success=1;
        }
    }

    if (sd && s_CFRelease) s_CFRelease(sd);
    if (a && s_CFRelease) s_CFRelease(a);
    if (kstreams && s_CFRelease) s_CFRelease(kstreams);
    return success;
}

/* =============================================================================
 * HOOK 1: AirPlayCopyServerInfo (libAirPlay.so)
 * Assinatura real AArch64: (void *session, void *properties, int32_t *outErr)
 * Injeta o Display 1 (Cluster AID 1280x480, Type 111) no GET /info do AirPlay
 * ============================================================================= */
typedef void *(*AirPlayCopyServerInfo_fn)(void *session, void *properties, int32_t *outErr);
static AirPlayCopyServerInfo_fn real_AirPlayCopyServerInfo = NULL;

void *AirPlayCopyServerInfo(void *session, void *properties, int32_t *outErr) {
    if (!real_AirPlayCopyServerInfo) {
        real_AirPlayCopyServerInfo = (AirPlayCopyServerInfo_fn)dlsym(RTLD_NEXT, "AirPlayCopyServerInfo");
    }

    int32_t err = 0;
    void *base = NULL;
    if (real_AirPlayCopyServerInfo) {
        base = real_AirPlayCopyServerInfo(session, properties, &err);
    }
    log_msg("SERVERINFO", "Stock AirPlayCopyServerInfo ret base=%p, err=%d", base, err);

    if (outErr) *outErr = err;
    if (!base || err != 0) return base;

    init_cf_symbols();

    /* 1. Clona o dicionario base do servidor para mutacao */
    save_info_snapshot(base,"stock-info",1);
    void *info = dict_clone(base);
    if (!info) return base;

    /* 2. Habilita bit 26 em root features (AIRPLAY_FEATURE_BIT26) */
    void *k_features = s_cf("features");
    if (k_features) {
        int32_t get_err = 0;
        int64_t feat = s_CFDictionaryGetInt64 ? s_CFDictionaryGetInt64(info, k_features, &get_err) : 0;
        if (!get_err) {
            int64_t feat_new = (int64_t)((uint64_t)feat | AIRPLAY_FEATURE_BIT26);
            if (s_CFDictionarySetInt64) s_CFDictionarySetInt64(info, k_features, feat_new);
            log_msg("SERVERINFO", "Root features: 0x%llx -> 0x%llx (bit 26 ativado)",
                    (unsigned long long)feat, (unsigned long long)feat_new);
        }
        if (s_CFRelease) s_CFRelease(k_features);
    }

    /* 3. Define root enabledFeatures */
    int use_va = 0;
    set_reference_enabled_features(info, use_va);

    /* 4. Extrai e clona o array de displays */
    void *k_displays = s_cf("displays");
    void *stock_displays = (k_displays && s_CFDictionaryGetValue) ? (void *)s_CFDictionaryGetValue(base, k_displays) : NULL;
    if (stock_displays && s_CFArrayGetCount && s_CFArrayGetCount(stock_displays) > 0) {
        void *displays = array_clone(stock_displays);
        void *stock_disp0 = (void *)s_CFArrayGetValueAtIndex(stock_displays, 0);
        void *alt = dict_clone(stock_disp0);

        if (displays && alt) {
            remove_key(alt, "primaryInputDevice");
            remove_key(alt, "edid");
            remove_key(alt, "platformLayer");
            remove_key(alt, "avcc");
            remove_key(alt, "xOffset");
            remove_key(alt, "yOffset");
            remove_key(alt, "windowWidth");
            remove_key(alt, "windowHeight");

            set_i64(alt, "type", K_STREAM_ALT);
            set_i64(alt, "maxFPS", 30);
            set_i64(alt, "features", 0);
            set_i64(alt, "widthPixels", 1280);
            set_i64(alt, "heightPixels", 480);
            set_i64(alt, "widthPhysical", 320);
            set_i64(alt, "heightPhysical", 120);
            set_str(alt, "uuid", ALT_UUID_DEFAULT);
            set_str(alt, "initialURL", ALT_TOP_URL);

            void *urls = NULL;
            if (s_CFArrayEnsureCreatedAndAppendCString) {
                s_CFArrayEnsureCreatedAndAppendCString(&urls, ALT_TOP_URL, (size_t)-1);
                s_CFArrayEnsureCreatedAndAppendCString(&urls, ALT_URL_DEFAULT, (size_t)-1);
                s_CFArrayEnsureCreatedAndAppendCString(&urls, ALT_URL_MAP, (size_t)-1);
                s_CFArrayEnsureCreatedAndAppendCString(&urls, ALT_URL_INSTRUCTIONCARD, (size_t)-1);
                void *k_urls = s_cf("altScreenSuggestUIURLs");
                if (k_urls && s_CFDictionarySetValue) {
                    s_CFDictionarySetValue(alt, k_urls, urls);
                    if (s_CFRelease) s_CFRelease(k_urls);
                }
                if (urls && s_CFRelease) s_CFRelease(urls);
            }

            void *prepared=layout_prepare_displays(stock_displays,alt);
            if(prepared && set_reference_enabled_features(info,1)) {
                s_CFRelease(displays);displays=prepared;prepared=NULL;use_va=1;
                log_msg("LAYOUT","Top-aligned maneuver request; auxiliary safeArea=%d,%d,%dx%d; primary geometry preserved; app response unvalidated",ALT_SAFE_X,ALT_SAFE_Y,ALT_SAFE_W,ALT_SAFE_H);
            } else log_msg("LAYOUT","Complete view-area construction failed; using prior map-only display schema");
            if(prepared)s_CFRelease(prepared);

            if (s_CFArrayAppendValue) {
                if(!use_va)s_CFArrayAppendValue(displays, alt);
                if(!layout_object(info,"displays",displays)) {
                    if(alt)s_CFRelease(alt);s_CFRelease(displays);s_CFRelease(k_displays);s_CFRelease(info);
                    log_msg("LAYOUT","Display transaction not committed; returning factory info");
                    return base;
                }
                if(use_va){pthread_mutex_lock(&g_info_snapshot_lock);g_layout_advertised=1;pthread_mutex_unlock(&g_info_snapshot_lock);}
                log_msg("SERVERINFO", ">>> DISPLAY 1 (Cluster AID 1280x480, Type 111, AltScreen%s) ANUNCIADO NO RTSP GET /info! <<<",
                        use_va ? "+viewAreas" : "");
            }
        }
        if (alt && s_CFRelease) s_CFRelease(alt);
        if (displays && s_CFRelease) s_CFRelease(displays);
    }
    if (k_displays && s_CFRelease) s_CFRelease(k_displays);

    save_info_snapshot(info,"advertised-info",2);

    if (s_CFRelease) s_CFRelease(base);
    if (outErr) *outErr = 0;
    return info;
}

/* =============================================================================
 * HOOK 2: AirPlayReceiverSessionSetup (libAirPlay.so)
 * Trata o SETUP do Stream 111 com servidor TCP local
 * ============================================================================= */
typedef int32_t (*AirPlayReceiverSessionSetup_fn)(void *inSession, void *inRequest, void **outResponse);
static AirPlayReceiverSessionSetup_fn real_AirPlayReceiverSessionSetup = NULL;

int32_t AirPlayReceiverSessionSetup(void *inSession, void *inRequest, void **outResponse) {
    if (!real_AirPlayReceiverSessionSetup) {
        real_AirPlayReceiverSessionSetup = (AirPlayReceiverSessionSetup_fn)dlsym(RTLD_NEXT, "AirPlayReceiverSessionSetup");
    }

    init_cf_symbols();
    init_crypto_symbols();

    void *alt_desc = NULL;
    int other_count = 0;
    int has_alt = inRequest ? contains_stream111(inRequest, &alt_desc, &other_count) : 0;

    log_msg("SETUP", "AirPlayReceiverSessionSetup: has_stream111=%d, other_streams=%d, session=%p",
            has_alt, other_count, inSession);

    int32_t ret = 0;
    if (real_AirPlayReceiverSessionSetup) {
        ret = real_AirPlayReceiverSessionSetup(inSession, inRequest, outResponse);
    }

    log_msg("SETUP", "Stock AirPlayReceiverSessionSetup ret=%d, outResponse=%p",
            ret, (outResponse ? *outResponse : NULL));
    if(ret==0 && outResponse && *outResponse)log_stream_ports("factory",*outResponse);

    /* Initial SETUP has no media streams. Acknowledge AltScreen there as in
     * the v1 trace that reached stream 111; leave primary media SETUP alone. */
    void *requested_streams=inRequest ? get_streams(inRequest) : NULL;
    int initial_setup=inRequest && (!requested_streams || (s_CFArrayGetCount && s_CFArrayGetCount(requested_streams)==0));
    if (ret == 0 && (has_alt || initial_setup) && outResponse && *outResponse) {
        int use_va = layout_advertised();
        set_reference_enabled_features(*outResponse, use_va);
        log_msg("SETUP","AltScreen acknowledgement: initial=%d auxiliary=%d",initial_setup,has_alt);

        if (has_alt && alt_desc) {
            uint64_t cid = stream_connection_id(alt_desc);
            log_msg("SETUP", ">>> SOLICITACAO DE STREAM 111 DETECTADA! cid=%llu <<<", (unsigned long long)cid);

            int port = start_alt_receiver(inSession,cid);
            if (port > 0) {
                if(append_alt_setup_response(*outResponse, alt_desc, port)) {
                    log_msg("SETUP","Auxiliary response committed: type=111 dataPort=%d; awaiting TCP peer",port);
                    log_stream_ports("advertised",*outResponse);
                } else {
                    stop_alt_receiver();log_msg("SETUP","Auxiliary response construction failed; listener closed");
                }
            } else {
                log_msg("SETUP", "Falha ao iniciar receptor TCP do Stream 111!");
            }
        }
    }

    return ret;
}

/* =============================================================================
 * HOOK 3: AirPlayReceiverSessionTearDown (libAirPlay.so)
 * ============================================================================= */
typedef void (*AirPlayReceiverSessionTearDown_fn)(void *inSession, void *inRequest, int32_t inReason, uint8_t *outDone);
static AirPlayReceiverSessionTearDown_fn real_AirPlayReceiverSessionTearDown = NULL;

/* Matching MU0841 native teardown treats a nonempty typed streams array as a
 * partial teardown (outDone=0). Missing/empty streams ends the whole session.
 * Do not destroy the auxiliary receiver or retained pairing on primary/audio
 * teardown; app transitions can tear down those streams independently. */
typedef uint32_t (*cf_type_id_fn)(void);
typedef const void *(*cf_dict_typed_fn)(const void *,const void *,uint32_t,int32_t *);
typedef const void *(*cf_array_typed_fn)(const void *,int64_t,uint32_t,int32_t *);
static cf_type_id_fn s_teardown_array_type,s_teardown_dict_type;
static cf_dict_typed_fn s_teardown_dict_get;
static cf_array_typed_fn s_teardown_array_get;
static int teardown_scope(void *request,int *auxiliary) {
    *auxiliary=0;
    if(!request)return 1;
    if(!s_teardown_dict_get)s_teardown_dict_get=(cf_dict_typed_fn)dlsym(RTLD_DEFAULT,"CFDictionaryGetTypedValue");
    if(!s_teardown_array_get)s_teardown_array_get=(cf_array_typed_fn)dlsym(RTLD_DEFAULT,"CFArrayGetTypedValueAtIndex");
    if(!s_teardown_array_type)s_teardown_array_type=(cf_type_id_fn)dlsym(RTLD_DEFAULT,"CFArrayGetTypeID");
    if(!s_teardown_dict_type)s_teardown_dict_type=(cf_type_id_fn)dlsym(RTLD_DEFAULT,"CFDictionaryGetTypeID");
    if(!s_teardown_dict_get || !s_teardown_array_get || !s_teardown_array_type || !s_teardown_dict_type ||
       !s_CFArrayGetCount || !s_CFDictionaryGetInt64)return 1;
    void *key=s_cf("streams");
    const void *streams=key?s_teardown_dict_get(request,key,s_teardown_array_type(),NULL):NULL;
    if(key && s_CFRelease)s_CFRelease(key);
    if(!streams)return 1;
    int64_t count=s_CFArrayGetCount(streams);
    if(count<=0)return 1;
    if(count>64)return 1; /* Invalid/unbounded request: fail closed. */
    for(int64_t i=0;i<count;i++) {
        int32_t error=0;
        const void *stream=s_teardown_array_get(streams,i,s_teardown_dict_type(),&error);
        if(error || !stream)return 1;
        if(stream_type(stream)==K_STREAM_ALT)*auxiliary=1;
    }
    return 0;
}
void AirPlayReceiverSessionTearDown(void *inSession, void *inRequest, int32_t inReason, uint8_t *outDone) {
    if (!real_AirPlayReceiverSessionTearDown) {
        real_AirPlayReceiverSessionTearDown = (AirPlayReceiverSessionTearDown_fn)dlsym(RTLD_NEXT, "AirPlayReceiverSessionTearDown");
    }
    int auxiliary=0,whole=teardown_scope(inRequest,&auxiliary);
    log_msg("TEARDOWN", "scope=%s auxiliary=%d reason=%d",whole?"session":"streams",auxiliary,inReason);
    if(inRequest)log_stream_ports("teardown",inRequest);
    pthread_mutex_lock(&g_crypto_lock);
    int owns_stream=g_alt_session==inSession;
    if(whole && g_master_session==inSession) {
        secret_zero(g_master_key,sizeof(g_master_key));g_master_valid=0;g_master_session=NULL;
    }
    pthread_mutex_unlock(&g_crypto_lock);
    if(owns_stream && (whole || auxiliary))stop_alt_receiver();
    if (real_AirPlayReceiverSessionTearDown) {
        real_AirPlayReceiverSessionTearDown(inSession, inRequest, inReason, outDone);
    }
    if(whole)remember_pairing(inSession,NULL);
}

/* =============================================================================
 * HOOK 4: ScreenStreamCreate, ScreenStreamStop & ScreenStreamSetWidthHeight (libAirPlaySupport.so)
 * Garante que Display 0 (Central Multimidia 784x468) permaneca 100% nativo para o Cinemo
 * ============================================================================= */
typedef int32_t (*ScreenStreamCreate_fn)(void **outStream);
static ScreenStreamCreate_fn real_ScreenStreamCreate = NULL;

int32_t ScreenStreamCreate(void **outStream) {
    if (!real_ScreenStreamCreate) {
        real_ScreenStreamCreate = (ScreenStreamCreate_fn)dlsym(RTLD_NEXT, "ScreenStreamCreate");
    }
    int32_t ret = 0;
    if (real_ScreenStreamCreate) {
        ret = real_ScreenStreamCreate(outStream);
    }
    log_msg("STREAM", "ScreenStreamCreate ret=%d (streamRef=%p)", ret, (outStream ? *outStream : NULL));
    return ret;
}

typedef int32_t (*ScreenStreamStop_fn)(void *inStream);
static ScreenStreamStop_fn real_ScreenStreamStop = NULL;

int32_t ScreenStreamStop(void *inStream) {
    if (!real_ScreenStreamStop) {
        real_ScreenStreamStop = (ScreenStreamStop_fn)dlsym(RTLD_NEXT, "ScreenStreamStop");
    }
    log_msg("STREAM", "ScreenStreamStop: streamRef=%p", inStream);
    if (real_ScreenStreamStop) {
        return real_ScreenStreamStop(inStream);
    }
    return 0;
}

typedef void (*ScreenStreamSetWidthHeight_fn)(void *inStream, uint32_t inWidth, uint32_t inHeight);
static ScreenStreamSetWidthHeight_fn real_ScreenStreamSetWidthHeight = NULL;

void ScreenStreamSetWidthHeight(void *inStream, uint32_t inWidth, uint32_t inHeight) {
    if (!real_ScreenStreamSetWidthHeight) {
        real_ScreenStreamSetWidthHeight = (ScreenStreamSetWidthHeight_fn)dlsym(RTLD_NEXT, "ScreenStreamSetWidthHeight");
    }
    log_msg("STREAM", "ScreenStreamSetWidthHeight: streamRef=%p, %ux%u (Display 0 Cinemo mantido nativo)",
            inStream, inWidth, inHeight);
    if (real_ScreenStreamSetWidthHeight) {
        real_ScreenStreamSetWidthHeight(inStream, inWidth, inHeight);
    }
}

/* =============================================================================
 * Ciclo de Vida da Biblioteca
 * ============================================================================= */
#ifndef ALTSCREEN_TEST
__attribute__((constructor))
static void altscreen_init(void) {
    log_msg("INIT", "=================================================");
    log_msg("INIT", "altscreen_handler.so MU0841 v15 (session lease recovery) ativo no PID: %d", getpid());
    log_msg("INIT", "Display 0: Cinemo 100%% nativo | Display 1: Type 111 (1280x480) -> RTP [ff14::2:4]:42800 (PID 17, eth0.5)");
    log_msg("INIT", "=================================================");
    init_cf_symbols();
    init_crypto_symbols();
    /* Network socket ownership starts only after the root encoder grant. */
}

__attribute__((destructor))
static void altscreen_cleanup(void) {
    log_msg("CLEANUP", "altscreen_handler.so finalizado (PID: %d)", getpid());
    stop_alt_receiver();
    for(int i=0;i<4;i++)if(g_pairings[i].session)remember_pairing(g_pairings[i].session,NULL);
    if (g_rtp_sock >= 0) {
        close(g_rtp_sock);
        g_rtp_sock = -1;
    }
    pthread_mutex_lock(&g_log_mutex);
    g_log_closed=1;
    if (g_kmsg_fd >= 0) {
        close(g_kmsg_fd);
        g_kmsg_fd = -1;
    }
    if (g_log_fp) {
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
    pthread_mutex_unlock(&g_log_mutex);
}
#endif

