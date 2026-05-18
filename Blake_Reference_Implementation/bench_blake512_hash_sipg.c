#include "../bench_common.h"
 
#include "blake_ref.c"
 
static __inline__ u64 rotl64_sip(u64 x, unsigned int n) {
  return (u64)((x << n) | (x >> (64u - n)));
}
 
static HashReturn compress64_sipg(hashState *state, const BitSequence *datablock) {
  u64 v[16];
  u64 m[16];
  int round;
 
#define G64_SIP(a,b,c,d,i) \
  do { \
    v[a] = (u64)(v[a] + v[b]); \
    v[c] = (u64)(v[c] + v[d]); \
    v[a] = (u64)(v[a] + m[sigma[round][2*i]]); \
    v[c] = (u64)(v[c] + m[sigma[round][2*i+1]]); \
    v[b] = rotl64_sip(v[b], 13); \
    v[d] = rotl64_sip(v[d], 16); \
    v[b] ^= v[a]; \
    v[d] ^= v[c]; \
    v[a] = rotl64_sip(v[a], 32); \
    v[c] = (u64)(v[c] + v[b]); \
    v[a] = (u64)(v[a] + v[d]); \
    v[b] = rotl64_sip(v[b], 13); \
    v[d] = rotl64_sip(v[d], 16); \
    v[b] ^= v[c]; \
    v[d] ^= v[a]; \
    v[c] = rotl64_sip(v[c], 32); \
  } while (0)
 
  m[ 0] = U8TO64_BE(datablock +  0);
  m[ 1] = U8TO64_BE(datablock +  8);
  m[ 2] = U8TO64_BE(datablock + 16);
  m[ 3] = U8TO64_BE(datablock + 24);
  m[ 4] = U8TO64_BE(datablock + 32);
  m[ 5] = U8TO64_BE(datablock + 40);
  m[ 6] = U8TO64_BE(datablock + 48);
  m[ 7] = U8TO64_BE(datablock + 56);
  m[ 8] = U8TO64_BE(datablock + 64);
  m[ 9] = U8TO64_BE(datablock + 72);
  m[10] = U8TO64_BE(datablock + 80);
  m[11] = U8TO64_BE(datablock + 88);
  m[12] = U8TO64_BE(datablock + 96);
  m[13] = U8TO64_BE(datablock +104);
  m[14] = U8TO64_BE(datablock +112);
  m[15] = U8TO64_BE(datablock +120);
 
  v[ 0] = state->h64[0];
  v[ 1] = state->h64[1];
  v[ 2] = state->h64[2];
  v[ 3] = state->h64[3];
  v[ 4] = state->h64[4];
  v[ 5] = state->h64[5];
  v[ 6] = state->h64[6];
  v[ 7] = state->h64[7];
  v[ 8] = state->salt64[0] ^ c64[0];
  v[ 9] = state->salt64[1] ^ c64[1];
  v[10] = state->salt64[2] ^ c64[2];
  v[11] = state->salt64[3] ^ c64[3];
  if (state->nullt) {
    v[12] = c64[4];
    v[13] = c64[5];
    v[14] = c64[6];
    v[15] = c64[7];
  } else {
    v[12] = state->t64[0] ^ c64[4];
    v[13] = state->t64[0] ^ c64[5];
    v[14] = state->t64[1] ^ c64[6];
    v[15] = state->t64[1] ^ c64[7];
  }
 
  for (round = 0; round < NB_ROUNDS64; ++round) {
    G64_SIP( 0, 4, 8,12, 0);
    G64_SIP( 1, 5, 9,13, 1);
    G64_SIP( 2, 6,10,14, 2);
    G64_SIP( 3, 7,11,15, 3);
    G64_SIP( 0, 5,10,15, 4);
    G64_SIP( 1, 6,11,12, 5);
    G64_SIP( 2, 7, 8,13, 6);
    G64_SIP( 3, 4, 9,14, 7);
  }
 
  state->h64[0] ^= v[ 0] ^ v[ 8] ^ state->salt64[0];
  state->h64[1] ^= v[ 1] ^ v[ 9] ^ state->salt64[1];
  state->h64[2] ^= v[ 2] ^ v[10] ^ state->salt64[2];
  state->h64[3] ^= v[ 3] ^ v[11] ^ state->salt64[3];
  state->h64[4] ^= v[ 4] ^ v[12] ^ state->salt64[0];
  state->h64[5] ^= v[ 5] ^ v[13] ^ state->salt64[1];
  state->h64[6] ^= v[ 6] ^ v[14] ^ state->salt64[2];
  state->h64[7] ^= v[ 7] ^ v[15] ^ state->salt64[3];
 
  return SUCCESS;
}
 
static HashReturn Update64_sipg(hashState *state, const BitSequence *data, DataLength databitlen) {
  int fill;
  int left;
 
  if ((databitlen == 0) && (state->datalen != 1024)) return SUCCESS;
 
  left = (state->datalen >> 3);
  fill = 128 - left;
 
  if (left && (((databitlen >> 3) & 0x7F) >= fill)) {
    memcpy((void *)(state->data64 + left), (void *)data, fill);
    state->t64[0] += 1024;
    compress64_sipg(state, state->data64);
    data += fill;
    databitlen -= (fill << 3);
    left = 0;
  }
 
  while (databitlen >= 1024) {
    state->t64[0] += 1024;
    compress64_sipg(state, data);
    data += 128;
    databitlen -= 1024;
  }
 
  if (databitlen > 0) {
    memcpy((void *)(state->data64 + left), (void *)data, (databitlen >> 3) & 0x7F);
    state->datalen = (left << 3) + databitlen;
    if (databitlen & 0x7) state->data64[left + (databitlen >> 3)] = data[databitlen >> 3];
  } else {
    state->datalen = 0;
  }
 
  return SUCCESS;
}
 
static HashReturn Update_sipg(hashState *state, const BitSequence *data, DataLength databitlen) {
  if (state->hashbitlen < 384) return FAIL;
  return Update64_sipg(state, data, databitlen);
}
 
static HashReturn Final64_sipg(hashState *state, BitSequence *hashval) {
  unsigned char msglen[16];
  BitSequence zz = 0x00, zo = 0x01, oz = 0x80, oo = 0x81;
 
  u64 low, high;
  low = state->t64[0] + state->datalen;
  high = state->t64[1];
  if (low < state->datalen) high++;
  U64TO8_BE(msglen + 0, high);
  U64TO8_BE(msglen + 8, low);
 
  if (state->datalen % 8 == 0) {
    if (state->datalen == 888) {
      state->t64[0] -= 8;
      if (state->hashbitlen == 384)
        Update64_sipg(state, &oz, 8);
      else
        Update64_sipg(state, &oo, 8);
    } else {
      if (state->datalen < 888) {
        if (state->datalen == 0) state->nullt = 1;
        state->t64[0] -= 888 - state->datalen;
        Update64_sipg(state, padding, 888 - state->datalen);
      } else {
        state->t64[0] -= 1024 - state->datalen;
        Update64_sipg(state, padding, 1024 - state->datalen);
        state->t64[0] -= 888;
        Update64_sipg(state, padding + 1, 888);
        state->nullt = 1;
      }
      if (state->hashbitlen == 384)
        Update64_sipg(state, &zz, 8);
      else
        Update_sipg(state, &zo, 8);
      state->t64[0] -= 8;
    }
    state->t64[0] -= 128;
    Update_sipg(state, msglen, 128);
  } else {
    state->data64[state->datalen / 8] &= (0xFF << (8 - state->datalen % 8));
    state->data64[state->datalen / 8] ^= (0x80 >> (state->datalen % 8));
 
    if ((state->datalen > 888) && (state->datalen < 895)) {
      if (state->hashbitlen == 384)
        state->data64[state->datalen / 8] ^= zz;
      else
        state->data64[state->datalen / 8] ^= zo;
      state->t64[0] -= (8 - (state->datalen % 8));
      state->datalen = (state->datalen & (DataLength)0xfffffffffffffff8ULL) + 8;
    } else {
      if (state->datalen < 888) {
        state->t64[0] -= 888 - state->datalen;
        state->datalen = (state->datalen & (DataLength)0xfffffffffffffff8ULL) + 8;
        Update64_sipg(state, padding + 1, 888 - state->datalen);
      } else {
        if (state->datalen > 1016) {
          state->t64[0] -= 1024 - state->datalen;
          state->datalen = 1024;
          Update64_sipg(state, padding + 1, 0);
          state->t64[0] -= 888;
          Update64_sipg(state, padding + 1, 888);
          state->nullt = 1;
        } else {
          state->t64[0] -= 1024 - state->datalen;
          state->datalen = (state->datalen & (DataLength)0xfffffffffffffff8ULL) + 8;
          Update64_sipg(state, padding + 1, 1024 - state->datalen);
          state->t64[0] -= 888;
          Update64_sipg(state, padding + 1, 888);
          state->nullt = 1;
        }
      }
      state->t64[0] -= 8;
      if (state->hashbitlen == 384)
        Update64_sipg(state, &zz, 8);
      else
        Update64_sipg(state, &zo, 8);
    }
    state->t64[0] -= 128;
    Update_sipg(state, msglen, 128);
  }
 
  U64TO8_BE(hashval + 0, state->h64[0]);
  U64TO8_BE(hashval + 8, state->h64[1]);
  U64TO8_BE(hashval + 16, state->h64[2]);
  U64TO8_BE(hashval + 24, state->h64[3]);
  U64TO8_BE(hashval + 32, state->h64[4]);
  U64TO8_BE(hashval + 40, state->h64[5]);
 
  if (state->hashbitlen == 512) {
    U64TO8_BE(hashval + 48, state->h64[6]);
    U64TO8_BE(hashval + 56, state->h64[7]);
  }
 
  return SUCCESS;
}
 
static HashReturn Hash_sipg(int hashbitlen, const BitSequence *data, DataLength databitlen, BitSequence *hashval) {
  HashReturn ret;
  hashState state;
 
  ret = Init(&state, hashbitlen);
  if (ret != SUCCESS) return ret;
  if (hashbitlen < 384) return BAD_HASHBITLEN;
 
  ret = Update64_sipg(&state, data, databitlen);
  if (ret != SUCCESS) return ret;
 
  return Final64_sipg(&state, hashval);
}
 
static volatile uint64_t g_sink64 = 0;
 
static uint64_t load_u64_le(const uint8_t *p) {
  return ((uint64_t)p[0]) | ((uint64_t)p[1] << 8) | ((uint64_t)p[2] << 16) |
         ((uint64_t)p[3] << 24) | ((uint64_t)p[4] << 32) | ((uint64_t)p[5] << 40) |
         ((uint64_t)p[6] << 48) | ((uint64_t)p[7] << 56);
}
 
static void run_case(FILE *csv, uint32_t reps, size_t msg_bytes) {
  const size_t target_total_bytes = 256ull * 1024ull * 1024ull;
  size_t loops = target_total_bytes / msg_bytes;
  if (loops < 1) loops = 1;
 
  uint8_t *msg = (uint8_t *)malloc(msg_bytes);
  if (!msg) {
    fprintf(stderr, "malloc failed\n");
    exit(1);
  }
  for (size_t i = 0; i < msg_bytes; i++) msg[i] = (uint8_t)(i * 131u + 17u);
 
  uint8_t out[64];
 
  const size_t warmup_loops = 256;
  for (size_t i = 0; i < warmup_loops; i++) {
    Hash_sipg(512, (const BitSequence *)msg, (DataLength)msg_bytes * 8, (BitSequence *)out);
    g_sink64 ^= load_u64_le(out);
  }

  double *ns = (double *)malloc((size_t)reps * sizeof(double));
  double *mib = (double *)malloc((size_t)reps * sizeof(double));
  double *cyc = (double *)malloc((size_t)reps * sizeof(double));
  if (!ns || !mib || !cyc) {
    fprintf(stderr, "malloc failed\n");
    exit(1);
  }

  for (uint32_t r = 0; r < reps; r++) {
    uint64_t c0 = 0, c1 = 0;
    double t0 = bench_qpc_seconds();
#if defined(__i386__) || defined(__x86_64__)
    c0 = bench_tsc_start();
#endif
    for (size_t l = 0; l < loops; l++) {
      Hash_sipg(512, (const BitSequence *)msg, (DataLength)msg_bytes * 8, (BitSequence *)out);
    }
#if defined(__i386__) || defined(__x86_64__)
    c1 = bench_tsc_stop();
#endif
    double t1 = bench_qpc_seconds();

    g_sink64 ^= load_u64_le(out);

    const double sec = t1 - t0;
    const double total_bytes = (double)loops * (double)msg_bytes;
    ns[r] = sec * 1e9 / total_bytes;
    mib[r] = total_bytes / (1024.0 * 1024.0) / sec;
#if defined(__i386__) || defined(__x86_64__)
    cyc[r] = (double)(c1 - c0) / total_bytes;
#else
    cyc[r] = 0.0;
#endif
  }

  const double ns_med = bench_median(ns, reps);
  const double ns_sd = bench_stddev(ns, reps);
  const double mib_med = bench_median(mib, reps);
  const double mib_sd = bench_stddev(mib, reps);
  const double cyc_med = bench_median(cyc, reps);
  const double cyc_sd = bench_stddev(cyc, reps);

  printf("%8zu bytes: loops=%zu  reps=%u  ns/byte=%.3f (sd %.3f)  MiB/s=%.2f (sd %.2f)  cycles/byte=%.3f (sd %.3f)\n",
         msg_bytes, loops, reps, ns_med, ns_sd, mib_med, mib_sd, cyc_med, cyc_sd);
  if (csv) bench_csv_write_row_hash(csv, "blake", "blake512_hash_sipg", 0, msg_bytes, reps, ns_med, ns_sd, mib_med, mib_sd, cyc_med, cyc_sd);

  free(ns);
  free(mib);
  free(cyc);
 
  free(msg);
}
 
int main(int argc, char **argv) {
  bench_args_t args;
  bench_parse_args(argc, argv, 0, 5, &args);
  bench_enable_amd64_optimizations();
  FILE *csv = bench_csv_open(argv[0], args.csv_path);
  if (csv) bench_csv_write_header_hash(csv);
  printf("BLAKE-512 full Hash_sipg() benchmark (SipHash-adapted G, inject 2 msg words after first add-layer)  reps=%u\n", args.reps);
  run_case(csv, args.reps, 1024);
  run_case(csv, args.reps, 64ull * 1024ull);
  run_case(csv, args.reps, 1024ull * 1024ull);
  printf("sink=%llu\n", (unsigned long long)g_sink64);
  if (csv) fclose(csv);
  return 0;
}
