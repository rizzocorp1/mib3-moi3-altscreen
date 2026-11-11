/* Bounded metadata only. Parameter bytes are collected only for NAL7/8;
 * no slice/picture bytes are retained or logged. Receiver-independent tap. */
typedef struct {
 uint64_t gap_samples,last_pcr;int pcr_valid;
 uint64_t prev_us,bucket_ms,bucket_count,max_bucket,min_gap_us,max_same_clock;
 uint32_t previous_rtp;uint64_t same_clock;
 uint64_t pes_count,pes_bytes,pes_start_us,pes_end_us,max_pes_bytes,max_pes_span_us;
 uint64_t cc_gaps,cc_duplicates;unsigned cc;int cc_valid;
 int kind;unsigned zeros;size_t parameter_size;int parameter_overflow;
 uint8_t parameter[512],previous_sps[512],previous_pps[512];size_t sps_size,pps_size;
 int64_t lead_min,lead_max;uint64_t lead_samples;
} PacketDetail;
static uint64_t detail_now_us(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t)!=0)return 0;return (uint64_t)t.tv_sec*1000000+(uint64_t)t.tv_nsec/1000;}
static void detail_packet(PacketDetail *d,uint64_t now,uint32_t rtp){
 if(d->prev_us && now>=d->prev_us){uint64_t gap=now-d->prev_us;if(!d->gap_samples || gap<d->min_gap_us)d->min_gap_us=gap;d->gap_samples++;}
 d->prev_us=now;
 uint64_t bucket=now/1000;
 if(bucket!=d->bucket_ms){d->bucket_ms=bucket;d->bucket_count=0;}
 if(++d->bucket_count>d->max_bucket)d->max_bucket=d->bucket_count;
 if(d->same_clock && rtp==d->previous_rtp)d->same_clock++;else d->same_clock=1;
 if(d->same_clock>d->max_same_clock)d->max_same_clock=d->same_clock;
 d->previous_rtp=rtp;
}
static void detail_parameter_finish(FILE *out,unsigned flow,PacketDetail *d){
 if((d->kind!=7 && d->kind!=8) || d->parameter_overflow || !d->parameter_size)return;
 uint8_t *old=d->kind==7?d->previous_sps:d->previous_pps;size_t *size=d->kind==7?&d->sps_size:&d->pps_size;
 if(*size==d->parameter_size && !memcmp(old,d->parameter,*size))return;
 memcpy(old,d->parameter,d->parameter_size);*size=d->parameter_size;
 fprintf(out,"CODEC flow=%u nalType=%d bytes=%zu hex=",flow,d->kind,*size);
 for(size_t i=0;i<*size;i++){fprintf(out,"%02x",old[i]);}fputc('\n',out);
}
static void detail_parameter_byte(PacketDetail *d,uint8_t b){
 if(d->kind!=7 && d->kind!=8)return;
 if(d->parameter_size<sizeof(d->parameter))d->parameter[d->parameter_size++]=b;else d->parameter_overflow=1;
}
static void detail_annexb(FILE *out,unsigned flow,PacketDetail *d,const uint8_t *bytes,size_t n){
 for(size_t i=0;i<n;i++){
  uint8_t b=bytes[i];
  if(!b){if(d->zeros<512)d->zeros++;else d->parameter_overflow=1;continue;}
  if(b==1 && d->zeros>=2){detail_parameter_finish(out,flow,d);d->kind=-1;d->parameter_size=0;d->parameter_overflow=0;d->zeros=0;continue;}
  while(d->zeros){detail_parameter_byte(d,0);d->zeros--;}
  if(d->kind==-1)d->kind=b&31;
  detail_parameter_byte(d,b);
 }
}
static void detail_finish_pes(PacketDetail *d){
 if(d->pes_bytes>d->max_pes_bytes)d->max_pes_bytes=d->pes_bytes;
 if(d->pes_end_us>=d->pes_start_us && d->pes_end_us-d->pes_start_us>d->max_pes_span_us)d->max_pes_span_us=d->pes_end_us-d->pes_start_us;
}
static uint64_t detail_pts(const uint8_t *p){return ((uint64_t)((p[0]>>1)&7)<<30)|((uint64_t)p[1]<<22)|((uint64_t)(p[2]>>1)<<15)|((uint64_t)p[3]<<7)|(p[4]>>1);}
static void detail_video(FILE *out,unsigned flow,PacketDetail *d,const uint8_t *t,size_t o,uint64_t now){
 unsigned cc=t[3]&15;int discontinuity=(t[3]&0x20) && t[4]>0 && (t[5]&0x80);
 if(discontinuity){d->cc_valid=0;d->kind=0;d->zeros=0;d->parameter_size=0;}
 if(d->cc_valid){if(cc==d->cc)d->cc_duplicates++;else if(cc!=((d->cc+1)&15)){d->cc_gaps++;d->kind=0;d->zeros=0;d->parameter_size=0;}}
 d->cc=cc;d->cc_valid=1;
 if((t[1]&0x40) && o+9<=188 && !memcmp(t+o,"\0\0\1\xe0",4)){
  detail_finish_pes(d);d->pes_count++;d->pes_bytes=0;d->pes_start_us=now;
  size_t head=9+t[o+8];if(head>188-o)return;
  if((t[o+7]&0x80) && head>=14 && d->pcr_valid){
   uint64_t pcr=d->last_pcr;
   uint64_t pts=detail_pts(t+o+9);int64_t lead=(int64_t)((pts-pcr)&((1ULL<<33)-1));if(lead>=(1LL<<32))lead-=1LL<<33;
   if(!d->lead_samples || lead<d->lead_min)d->lead_min=lead;
   if(!d->lead_samples || lead>d->lead_max){d->lead_max=lead;}d->lead_samples++;
  }
  o+=head;
 }
 if(o<188){d->pes_bytes+=188-o;d->pes_end_us=now;detail_annexb(out,flow,d,t+o,188-o);}
}
static void detail_summary(FILE *out,unsigned flow,PacketDetail *d){
 detail_finish_pes(d);
 fprintf(out,"DETAIL flow=%u maxPacketsIn1ms=%llu minPacketGapUs=%llu maxSameRtpClockPackets=%llu pesStarts=%llu maxPesBytes=%llu maxPesSpanUs=%llu videoCcGaps=%llu videoCcDuplicates=%llu ptsLatestPcrSamples=%llu ptsPcrLeadMin90k=%lld ptsPcrLeadMax90k=%lld spsBytes=%zu ppsBytes=%zu\n",flow,(unsigned long long)d->max_bucket,(unsigned long long)d->min_gap_us,(unsigned long long)d->max_same_clock,(unsigned long long)d->pes_count,(unsigned long long)d->max_pes_bytes,(unsigned long long)d->max_pes_span_us,(unsigned long long)d->cc_gaps,(unsigned long long)d->cc_duplicates,(unsigned long long)d->lead_samples,(long long)d->lead_min,(long long)d->lead_max,d->sps_size,d->pps_size);
}
