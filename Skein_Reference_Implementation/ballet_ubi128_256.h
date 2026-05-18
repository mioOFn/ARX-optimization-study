#ifndef BALLET_UBI128_256_H
#define BALLET_UBI128_256_H

#include <stddef.h>
#include <stdint.h>

#include "skein.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  Skein_Ctxt_Hdr_t h;
  uint32_t X[4];
  u08b_t b[16];
} BalletUBI_128_Ctxt_t;

typedef struct {
  BalletUBI_128_Ctxt_t s;
  uint32_t rounds;
} BalletUBI_128_256_Ctxt_t;

int BalletUBI_128_256_Init(BalletUBI_128_256_Ctxt_t *ctx, size_t hashBitLen, uint32_t rounds);
int BalletUBI_128_256_Update(BalletUBI_128_256_Ctxt_t *ctx, const u08b_t *msg, size_t msgByteCnt);
int BalletUBI_128_256_Final(BalletUBI_128_256_Ctxt_t *ctx, u08b_t *hashVal);
int BalletUBI_128_256_Hash(uint32_t rounds, size_t hashBitLen, const u08b_t *msg, size_t msgByteCnt, u08b_t *hashVal);

#ifdef __cplusplus
}
#endif

#endif
