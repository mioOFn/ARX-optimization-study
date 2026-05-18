#include "../bench_common.h"
 
static volatile uint64_t g_sink = 0;
 
static __inline__ uint64_t rotl64(uint64_t x, unsigned int n) {
  return (x << (n & 63u)) | (x >> ((64u - n) & 63u));
}
 
 
typedef void (*kernel_fn)(uint64_t *a, uint64_t *b, uint64_t *c, uint64_t *d, size_t lanes, size_t iters);
 
static void kernel_arx_add(uint64_t *a, uint64_t *b, uint64_t *c, uint64_t *d, size_t lanes, size_t iters) {
  for (size_t i = 0; i < iters; i++) {
    for (size_t j = 0; j < lanes; j++) {
      uint64_t aj = a[j], bj = b[j], cj = c[j], dj = d[j];
      aj += bj;
      dj ^= rotl64(aj, 13);
      cj += dj;
      bj ^= rotl64(cj, 16);
      aj += dj;
      bj = rotl64(bj, 32) ^ aj;
      cj += bj;
      dj = rotl64(dj, 21) ^ cj;
      a[j] = aj;
      b[j] = bj;
      c[j] = cj;
      d[j] = dj;
    }
  }
}
 
static void kernel_arx_xor(uint64_t *a, uint64_t *b, uint64_t *c, uint64_t *d, size_t lanes, size_t iters) {
  for (size_t i = 0; i < iters; i++) {
    for (size_t j = 0; j < lanes; j++) {
      uint64_t aj = a[j], bj = b[j], cj = c[j], dj = d[j];
      aj ^= bj;
      dj ^= rotl64(aj, 13);
      cj ^= dj;
      bj ^= rotl64(cj, 16);
      aj ^= dj;
      bj = rotl64(bj, 32) ^ aj;
      cj ^= bj;
      dj = rotl64(dj, 21) ^ cj;
      a[j] = aj;
      b[j] = bj;
      c[j] = cj;
      d[j] = dj;
    }
  }
}
 
static void kernel_arx_add_reglow(uint64_t *a, uint64_t *b, uint64_t *c, uint64_t *d, size_t lanes, size_t iters) {
  for (size_t i = 0; i < iters; i++) {
    for (size_t j = 0; j < lanes; j++) {
      uint64_t aj = a[j], bj = b[j], cj = c[j], dj = d[j];
      aj += bj;
      dj ^= rotl64(aj, 13);
      cj += dj;
      bj ^= rotl64(cj, 16);
      aj += dj;
      bj = rotl64(bj, 32) ^ aj;
      cj += bj;
      dj = rotl64(dj, 21) ^ cj;
      a[j] = aj;
      b[j] = bj;
      c[j] = cj;
      d[j] = dj;
    }
  }
}
 
static void kernel_arx_add_reghigh(uint64_t *a, uint64_t *b, uint64_t *c, uint64_t *d, size_t lanes, size_t iters) {
  for (size_t i = 0; i < iters; i++) {
    for (size_t j = 0; j < lanes; j++) {
      uint64_t aj = a[j], bj = b[j], cj = c[j], dj = d[j];
 
      uint64_t t0 = aj + bj;
      uint64_t t1 = rotl64(t0, 13);
      uint64_t t2 = dj ^ t1;
      uint64_t t3 = cj + t2;
      uint64_t t4 = rotl64(t3, 16);
      uint64_t t5 = bj ^ t4;
      uint64_t t6 = t0 + t2;
      uint64_t t7 = rotl64(t5, 32);
      uint64_t t8 = t7 ^ t6;
      uint64_t t9 = t3 + t8;
      uint64_t t10 = rotl64(t2, 21);
      uint64_t t11 = t10 ^ t9;
 
      uint64_t u0 = t6 ^ t3;
      uint64_t u1 = t8 ^ t2;
      uint64_t u2 = t9 ^ t0;
      uint64_t u3 = t11 ^ t5;
      uint64_t u4 = rotl64(u0, 7) ^ rotl64(u1, 11);
      uint64_t u5 = rotl64(u2, 17) ^ rotl64(u3, 19);
      uint64_t u6 = u4 + u5;
      uint64_t u7 = rotl64(u6, 23);
      uint64_t u8 = u7 ^ t6;
      uint64_t u9 = u8 + t9;
 
      a[j] = u9;
      b[j] = t8 ^ u6;
      c[j] = t9 ^ u4;
      d[j] = t11 ^ u5;
    }
  }
}
 
static void init_state(uint64_t *a, uint64_t *b, uint64_t *c, uint64_t *d, size_t lanes) {
  for (size_t i = 0; i < lanes; i++) {
    a[i] = 0x243F6A8885A308D3ULL ^ (uint64_t)(i * 0x9E3779B97F4A7C15ULL);
    b[i] = 0x13198A2E03707344ULL ^ (uint64_t)(i * 0xBF58476D1CE4E5B9ULL);
    c[i] = 0xA4093822299F31D0ULL ^ (uint64_t)(i * 0x94D049BB133111EBULL);
    d[i] = 0x082EFA98EC4E6C89ULL ^ (uint64_t)(i * 0xD6E8FEB86659FD93ULL);
  }
}
 
static void bench_one(FILE *csv, uint32_t reps, const char *group, const char *variant, kernel_fn fn, size_t lanes, size_t iters) {
  uint64_t *a = (uint64_t *)malloc(lanes * sizeof(uint64_t));
  uint64_t *b = (uint64_t *)malloc(lanes * sizeof(uint64_t));
  uint64_t *c = (uint64_t *)malloc(lanes * sizeof(uint64_t));
  uint64_t *d = (uint64_t *)malloc(lanes * sizeof(uint64_t));
  if (!a || !b || !c || !d) exit(1);
 
  init_state(a, b, c, d, lanes);
  fn(a, b, c, d, lanes, 2000);

  double *cyc = (double *)malloc((size_t)reps * sizeof(double));
  double *ns = (double *)malloc((size_t)reps * sizeof(double));
  if (!cyc || !ns) exit(1);

  for (uint32_t r = 0; r < reps; r++) {
    init_state(a, b, c, d, lanes);
    uint64_t c0 = 0, c1 = 0;
    double t0 = bench_qpc_seconds();
#if defined(__i386__) || defined(__x86_64__)
    c0 = bench_tsc_start();
#endif
    fn(a, b, c, d, lanes, iters);
#if defined(__i386__) || defined(__x86_64__)
    c1 = bench_tsc_stop();
#endif
    double t1 = bench_qpc_seconds();

    uint64_t acc = 0;
    for (size_t i = 0; i < lanes; i++) acc ^= a[i] ^ b[i] ^ c[i] ^ d[i];
    g_sink ^= acc;

    const double sec = t1 - t0;
    const double ops = (double)lanes * (double)iters;
    ns[r] = sec * 1e9 / ops;
#if defined(__i386__) || defined(__x86_64__)
    cyc[r] = (double)(c1 - c0) / ops;
#else
    cyc[r] = 0.0;
#endif
  }

  const double cyc_med = bench_median(cyc, reps);
  const double cyc_sd = bench_stddev(cyc, reps);
  const double ns_med = bench_median(ns, reps);
  const double ns_sd = bench_stddev(ns, reps);

  printf("CSV,%s,%s,lanes=%zu,iters=%zu,reps=%u,cycles_per_op_median=%.6f,cycles_per_op_stddev=%.6f,ns_per_op_median=%.6f,ns_per_op_stddev=%.6f\n",
         group, variant, lanes, iters, reps, cyc_med, cyc_sd, ns_med, ns_sd);
  if (csv) bench_csv_write_row_micro(csv, "arx_microarch", group, variant, (uint32_t)lanes, (uint32_t)iters, reps, cyc_med, cyc_sd, ns_med, ns_sd);

  free(cyc);
  free(ns);
 
  free(a);
  free(b);
  free(c);
  free(d);
}
 
static size_t pick_iters(size_t lanes) {
  const size_t base = 7000000;
  size_t it = base / lanes;
  if (it < 200000) it = 200000;
  return it;
}
 
int main(int argc, char **argv) {
  bench_args_t args;
  bench_parse_args(argc, argv, 0, 5, &args);
  FILE *csv = bench_csv_open(argv[0], args.csv_path);
  if (csv) bench_csv_write_header_micro(csv);
  printf("ARX microarch microbench  reps=%u\n", args.reps);
 
  const size_t lanes_list[] = {1, 2, 4, 8, 16};
  for (size_t i = 0; i < sizeof(lanes_list) / sizeof(lanes_list[0]); i++) {
    size_t lanes = lanes_list[i];
    size_t iters = pick_iters(lanes);
    bench_one(csv, args.reps, "ILP", "ADD", kernel_arx_add, lanes, iters);
    bench_one(csv, args.reps, "ILP", "XOR", kernel_arx_xor, lanes, iters);
  }
 
  {
    size_t lanes = 4;
    size_t iters = pick_iters(lanes);
    bench_one(csv, args.reps, "ADDvXOR", "ADD", kernel_arx_add, lanes, iters);
    bench_one(csv, args.reps, "ADDvXOR", "XOR", kernel_arx_xor, lanes, iters);
  }
 
  {
    const size_t reg_lanes_list[] = {1, 2, 4, 8, 16};
    for (size_t i = 0; i < sizeof(reg_lanes_list) / sizeof(reg_lanes_list[0]); i++) {
      size_t lanes = reg_lanes_list[i];
      size_t iters = pick_iters(lanes);
      bench_one(csv, args.reps, "REG", "LOW", kernel_arx_add_reglow, lanes, iters);
      bench_one(csv, args.reps, "REG", "HIGH", kernel_arx_add_reghigh, lanes, iters);
    }
  }
 
  printf("sink=%llu\n", (unsigned long long)g_sink);
  if (csv) fclose(csv);
  return 0;
}
