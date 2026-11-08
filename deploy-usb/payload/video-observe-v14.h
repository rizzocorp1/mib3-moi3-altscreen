/* Metadata only. Never stores picture payloads or changes video output. */
typedef struct {
    const uint8_t *data;
    size_t size,byte;
    unsigned bit,zeros;
    int bad;
} VideoBits;
static unsigned video_bit(VideoBits *b) {
    if(!b->bit) {
        if(b->zeros>=2 && b->byte<b->size && b->data[b->byte]==3){b->byte++;b->zeros=0;}
        if(b->byte>=b->size){b->bad=1;return 0;}
    }
    unsigned value=(b->data[b->byte]>>(7-b->bit))&1;
    if(++b->bit==8){b->zeros=b->data[b->byte]==0?b->zeros+1:0;b->byte++;b->bit=0;}
    return value;
}
static uint32_t video_bits(VideoBits *b,unsigned n) {
    if(n>32){b->bad=1;return 0;}
    uint32_t v=0;while(n--)v=(v<<1)|video_bit(b);return v;
}
static uint32_t video_ue(VideoBits *b) {
    unsigned zeros=0;
    while(!b->bad && !video_bit(b)){if(++zeros>30){b->bad=1;return 0;}}
    return b->bad?0:((1u<<zeros)-1)+video_bits(b,zeros);
}
static struct {
    unsigned sps_id,frame_bits,pps_id;
    int sps_valid,pps_valid;
    uint64_t slices,parse_fail,reference_steps,reference_duplicates,reference_gaps;
    unsigned last_reference;
    int reference_valid;
} g_video_observe;
static void video_observe_reset(void){memset(&g_video_observe,0,sizeof(g_video_observe));}
static void video_observe_parameter(const uint8_t *nal,size_t n) {
    if(n<2)return;
    VideoBits b={.data=nal+1,.size=n-1};unsigned kind=nal[0]&31;
    if(kind==7) {
        unsigned profile=video_bits(&b,8);video_bits(&b,16);unsigned id=video_ue(&b);
        if(profile==100 || profile==110 || profile==122 || profile==244 || profile==44 || profile==83 || profile==86 || profile==118 || profile==128 || profile==138 || profile==139 || profile==134 || profile==135) {
            unsigned chroma=video_ue(&b);
            if(chroma>3){b.bad=1;}
            if(chroma==3)video_bit(&b);
            video_ue(&b);video_ue(&b);video_bit(&b);
            /* Scaling matrices are valid codec syntax, but outside this small
             * observer's scope. Fail observation only; never reject video. */
            if(video_bit(&b))b.bad=1;
        } else if(profile!=66 && profile!=77 && profile!=88)b.bad=1;
        unsigned width=video_ue(&b)+4;
        if(!b.bad && id<=31 && width>=4 && width<=16) {
            if(g_video_observe.sps_valid && (id!=g_video_observe.sps_id || width!=g_video_observe.frame_bits))g_video_observe.reference_valid=0;
            g_video_observe.sps_id=id;g_video_observe.frame_bits=width;g_video_observe.sps_valid=1;
        } else {g_video_observe.sps_valid=0;g_video_observe.pps_valid=0;}
    } else if(kind==8) {
        unsigned id=video_ue(&b),sps=video_ue(&b);
        g_video_observe.pps_valid=!b.bad && id<=255 && g_video_observe.sps_valid && sps==g_video_observe.sps_id;
        if(g_video_observe.pps_valid)g_video_observe.pps_id=id;
    }
}
static void video_observe_config(const uint8_t *bytes,size_t size) {
    size_t start=0;
    while(start+4<size) {
        if(memcmp(bytes+start,"\0\0\0\1",4)){start++;continue;}
        size_t end=start+4;while(end+4<=size && memcmp(bytes+end,"\0\0\0\1",4))end++;
        if(end+4>size)end=size;
        video_observe_parameter(bytes+start+4,end-start-4);start=end;
    }
}
static void video_observe_frame(const uint8_t *bytes,size_t size,unsigned length_bytes,uint64_t count) {
    size_t pos=0;unsigned frame=0,type=0,reference=0;int seen=0,failed=0;
    while(pos<size) {
        if(length_bytes<1 || length_bytes>4 || size-pos<length_bytes){failed=1;break;}
        uint32_t n=0;for(unsigned i=0;i<length_bytes;i++)n=(n<<8)|bytes[pos++];
        if(!n || n>size-pos){failed=1;break;}
        unsigned kind=bytes[pos]&31;
        if(kind==1 || kind==5) {
            VideoBits b={.data=bytes+pos+1,.size=n-1};unsigned first=video_ue(&b),slice=video_ue(&b),pps=video_ue(&b);
            unsigned number=video_bits(&b,g_video_observe.frame_bits);
            if(b.bad || !g_video_observe.sps_valid || !g_video_observe.pps_valid || pps!=g_video_observe.pps_id || slice>9)failed=1;
            else if(!first && !seen) {
                frame=number;type=slice%5;reference=(bytes[pos]>>5)&3;seen=1;
                if(kind==5)g_video_observe.reference_valid=0;
                if(reference) {
                    if(g_video_observe.reference_valid) {
                        unsigned mask=(1u<<g_video_observe.frame_bits)-1;
                        unsigned delta=(frame-g_video_observe.last_reference)&mask;
                        if(!delta)g_video_observe.reference_duplicates++;
                        else if(delta==1)g_video_observe.reference_steps++;
                        else g_video_observe.reference_gaps++;
                    }
                    g_video_observe.last_reference=frame;g_video_observe.reference_valid=1;
                }
            }
        }
        pos+=n;
    }
    if(failed || !seen)g_video_observe.parse_fail++;else g_video_observe.slices++;
    if(count<=3 || count%300==0)
        log_msg("VIDEO_REFERENCE","parsed=%d frameNum=%u frameBits=%u sliceType=%u ref=%u observed=%llu parseFailures=%llu referenceSteps=%llu referenceDuplicates=%llu referenceGaps=%llu (metadata, not decoder status)",
            seen&&!failed,frame,g_video_observe.frame_bits,type,reference,
            (unsigned long long)g_video_observe.slices,(unsigned long long)g_video_observe.parse_fail,
            (unsigned long long)g_video_observe.reference_steps,(unsigned long long)g_video_observe.reference_duplicates,(unsigned long long)g_video_observe.reference_gaps);
}
