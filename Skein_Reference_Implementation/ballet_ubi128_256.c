#include <string.h>

#include "ballet_ubi128_256.h"

static __inline__ uint32_t rotl32(uint32_t x, unsigned int n) {
  return (x << (n & 31u)) | (x >> ((32u - n) & 31u));
}

static __inline__ u64b_t rotl64(u64b_t x, unsigned int n) {
  return (x << (n & 63u)) | (x >> ((64u - n) & 63u));
}

static __inline__ u64b_t swap64(u64b_t w64) {
  const u64b_t ONE = 1;
  if (1 == ((const u08b_t *)&ONE)[0]) return w64;
  return ((w64 & 0xFFULL) << 56) | (((w64 >> 8) & 0xFFULL) << 48) | (((w64 >> 16) & 0xFFULL) << 40) |
         (((w64 >> 24) & 0xFFULL) << 32) | (((w64 >> 32) & 0xFFULL) << 24) | (((w64 >> 40) & 0xFFULL) << 16) |
         (((w64 >> 48) & 0xFFULL) << 8) | (((w64 >> 56) & 0xFFULL));
}

static __inline__ uint64_t load64_le(const u08b_t *p) {
  return ((uint64_t)p[0]) | ((uint64_t)p[1] << 8) | ((uint64_t)p[2] << 16) | ((uint64_t)p[3] << 24) | ((uint64_t)p[4] << 32) |
         ((uint64_t)p[5] << 40) | ((uint64_t)p[6] << 48) | ((uint64_t)p[7] << 56);
}

static __inline__ uint32_t load32_le(const u08b_t *p) {
  return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static __inline__ void store64_le(u08b_t *p, uint64_t x) {
  p[0] = (u08b_t)(x);
  p[1] = (u08b_t)(x >> 8);
  p[2] = (u08b_t)(x >> 16);
  p[3] = (u08b_t)(x >> 24);
  p[4] = (u08b_t)(x >> 32);
  p[5] = (u08b_t)(x >> 40);
  p[6] = (u08b_t)(x >> 48);
  p[7] = (u08b_t)(x >> 56);
}

static void put32_lsb(u08b_t *dst, const uint32_t *src, size_t bCnt) {
  for (size_t n = 0; n < bCnt; n++) dst[n] = (u08b_t)(src[n >> 2] >> (8 * (n & 3)));
}

static void get32_lsb(uint32_t *dst, const u08b_t *src, size_t wCnt) {
  for (size_t i = 0; i < wCnt; i++) dst[i] = load32_le(src + 4 * i);
}

static __inline__ void ks_step_128_256(u64b_t *k0, u64b_t *k1, u64b_t *t0, u64b_t *t1, uint32_t i) {
  u64b_t t_temp = *t1;
  u64b_t k_temp = *k1;
  u64b_t new_t1 = (u64b_t)(*t0 ^ rotl64(*t1, 7) ^ rotl64(*t1, 17));
  u64b_t new_k1 = (u64b_t)(*k0 ^ rotl64(*k1, 3) ^ rotl64(*k1, 5));
  *t0 = t_temp;
  *k0 = k_temp;
  *t1 = new_t1;
  *k1 = (u64b_t)(new_k1 ^ new_t1 ^ (u64b_t)i);
}

static __inline__ void ballet128_256_rounds(uint32_t X[4], u64b_t k0, u64b_t k1, u64b_t t0, u64b_t t1, uint32_t rounds) {
  if (rounds == 0) rounds = 46;
  for (uint32_t r = 0; r < rounds; r++) {
    uint32_t skL = (uint32_t)k0;
    uint32_t skR = (uint32_t)(k0 >> 32);
    uint32_t tmp = (uint32_t)(X[1] ^ X[2]);
    if (r + 1 < rounds) {
      uint32_t n0 = (uint32_t)(X[1] ^ skL);
      uint32_t n1 = (uint32_t)(rotl32(X[0], 6) + rotl32(tmp, 9));
      uint32_t n2 = (uint32_t)(rotl32(X[3], 15) + rotl32(tmp, 14));
      uint32_t n3 = (uint32_t)(X[2] ^ skR);
      X[0] = n0;
      X[1] = n1;
      X[2] = n2;
      X[3] = n3;
      ks_step_128_256(&k0, &k1, &t0, &t1, r);
    } else {
      X[0] = (uint32_t)(rotl32(X[0], 6) + rotl32(tmp, 9));
      X[1] ^= skL;
      X[2] ^= skR;
      X[3] = (uint32_t)(rotl32(X[3], 15) + rotl32(tmp, 14));
    }
  }
}

static void BalletUBI_128_256_Process_Block(BalletUBI_128_256_Ctxt_t *ctx, const u08b_t *blkPtr, size_t blkCnt, size_t byteCntAdd) {
  enum { WCNT = 4, BLK_BYTES = 16 };
  uint32_t w[WCNT];
  uint32_t X[WCNT];
  Skein_assert(blkCnt != 0);
  do {
    ctx->s.h.T[0] += byteCntAdd;
    get32_lsb(w, blkPtr, WCNT);
    for (size_t i = 0; i < WCNT; i++) X[i] = w[i];

    u64b_t k0 = (u64b_t)((uint64_t)ctx->s.X[0] | ((uint64_t)ctx->s.X[1] << 32));
    u64b_t k1 = (u64b_t)((uint64_t)ctx->s.X[2] | ((uint64_t)ctx->s.X[3] << 32));
    u64b_t t0 = ctx->s.h.T[0];
    u64b_t t1 = ctx->s.h.T[1];
    ballet128_256_rounds(X, k0, k1, t0, t1, ctx->rounds);

    for (size_t i = 0; i < WCNT; i++) ctx->s.X[i] = (uint32_t)(X[i] ^ w[i]);

    Skein_Clear_First_Flag(ctx->s.h);
    blkPtr += BLK_BYTES;
  } while (--blkCnt);
}

int BalletUBI_128_256_Init(BalletUBI_128_256_Ctxt_t *ctx, size_t hashBitLen, uint32_t rounds) {
  union {
    u08b_t b[SKEIN_CFG_STR_LEN];
    u64b_t w[4];
  } cfg;

  Skein_Assert(hashBitLen > 0, SKEIN_BAD_HASHLEN);
  ctx->rounds = rounds;
  ctx->s.h.hashBitLen = hashBitLen;
  ctx->s.h.bCnt = 0;

  memset(&cfg.w, 0, sizeof(cfg.w));
  cfg.w[0] = swap64(SKEIN_SCHEMA_VER);
  cfg.w[1] = swap64((u64b_t)hashBitLen);
  cfg.w[2] = swap64(SKEIN_CFG_TREE_INFO_SEQUENTIAL);

  memset(ctx->s.X, 0, sizeof(ctx->s.X));
  ctx->s.h.T[0] = 0;
  ctx->s.h.T[1] = (u64b_t)(SKEIN_T1_FLAG_FIRST | SKEIN_T1_BLK_TYPE_CFG);
  BalletUBI_128_256_Process_Block(ctx, cfg.b, 1, 16);
  ctx->s.h.T[1] |= SKEIN_T1_FLAG_FINAL;
  BalletUBI_128_256_Process_Block(ctx, cfg.b + 16, 1, 16);

  ctx->s.h.T[0] = 0;
  ctx->s.h.T[1] = (u64b_t)(SKEIN_T1_FLAG_FIRST | SKEIN_T1_BLK_TYPE_MSG);
  ctx->s.h.bCnt = 0;
  return SKEIN_SUCCESS;
}

int BalletUBI_128_256_Update(BalletUBI_128_256_Ctxt_t *ctx, const u08b_t *msg, size_t msgByteCnt) {
  enum { BLK_BYTES = 16 };
  size_t n;
  Skein_Assert(ctx->s.h.bCnt <= BLK_BYTES, SKEIN_FAIL);

  if (msgByteCnt + ctx->s.h.bCnt > BLK_BYTES) {
    if (ctx->s.h.bCnt) {
      n = BLK_BYTES - ctx->s.h.bCnt;
      if (n) {
        Skein_assert(n < msgByteCnt);
        memcpy(&ctx->s.b[ctx->s.h.bCnt], msg, n);
        msgByteCnt -= n;
        msg += n;
        ctx->s.h.bCnt += n;
      }
      Skein_assert(ctx->s.h.bCnt == BLK_BYTES);
      BalletUBI_128_256_Process_Block(ctx, ctx->s.b, 1, BLK_BYTES);
      ctx->s.h.bCnt = 0;
    }
    if (msgByteCnt > BLK_BYTES) {
      n = (msgByteCnt - 1) / BLK_BYTES;
      BalletUBI_128_256_Process_Block(ctx, msg, n, BLK_BYTES);
      msgByteCnt -= n * BLK_BYTES;
      msg += n * BLK_BYTES;
    }
    Skein_assert(ctx->s.h.bCnt == 0);
  }

  if (msgByteCnt) {
    Skein_assert(msgByteCnt + ctx->s.h.bCnt <= BLK_BYTES);
    memcpy(&ctx->s.b[ctx->s.h.bCnt], msg, msgByteCnt);
    ctx->s.h.bCnt += msgByteCnt;
  }
  return SKEIN_SUCCESS;
}

int BalletUBI_128_256_Final(BalletUBI_128_256_Ctxt_t *ctx, u08b_t *hashVal) {
  enum { BLK_BYTES = 16 };
  size_t i, n, byteCnt;
  uint32_t Xsave[4];

  Skein_Assert(ctx->s.h.bCnt <= BLK_BYTES, SKEIN_FAIL);
  ctx->s.h.T[1] |= SKEIN_T1_FLAG_FINAL;
  if (ctx->s.h.bCnt < BLK_BYTES) memset(&ctx->s.b[ctx->s.h.bCnt], 0, BLK_BYTES - ctx->s.h.bCnt);
  BalletUBI_128_256_Process_Block(ctx, ctx->s.b, 1, ctx->s.h.bCnt);

  byteCnt = (ctx->s.h.hashBitLen + 7) >> 3;
  memset(ctx->s.b, 0, sizeof(ctx->s.b));
  memcpy(Xsave, ctx->s.X, sizeof(Xsave));
  for (i = 0; i * BLK_BYTES < byteCnt; i++) {
    memset(ctx->s.b, 0, BLK_BYTES);
    store64_le(ctx->s.b, (uint64_t)i);
    ctx->s.h.T[0] = 0;
    ctx->s.h.T[1] = (u64b_t)(SKEIN_T1_FLAG_FIRST | SKEIN_T1_BLK_TYPE_OUT | SKEIN_T1_FLAG_FINAL);
    ctx->s.h.bCnt = 0;
    BalletUBI_128_256_Process_Block(ctx, ctx->s.b, 1, 8);
    n = byteCnt - i * BLK_BYTES;
    if (n >= BLK_BYTES) n = BLK_BYTES;
    put32_lsb(hashVal + i * BLK_BYTES, ctx->s.X, n);
    memcpy(ctx->s.X, Xsave, sizeof(Xsave));
  }
  return SKEIN_SUCCESS;
}

int BalletUBI_128_256_Hash(uint32_t rounds, size_t hashBitLen, const u08b_t *msg, size_t msgByteCnt, u08b_t *hashVal) {
  BalletUBI_128_256_Ctxt_t ctx;
  int ret = BalletUBI_128_256_Init(&ctx, hashBitLen, rounds);
  if (ret != SKEIN_SUCCESS) return ret;
  ret = BalletUBI_128_256_Update(&ctx, msg, msgByteCnt);
  if (ret != SKEIN_SUCCESS) return ret;
  return BalletUBI_128_256_Final(&ctx, hashVal);
}

