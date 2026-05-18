#include "../bench_common.h"
 
#include "skein.c"
#include "skein_block.c"
 
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
 
  u08b_t *msg = (u08b_t *)malloc(msg_bytes);
  if (!msg) {
    fprintf(stderr, "malloc failed\n");
    exit(1);
  }
  for (size_t i = 0; i < msg_bytes; i++) msg[i] = (u08b_t)(i * 131u + 17u);
 
  u08b_t out[32];
  Skein_256_Ctxt_t ctx;
 
  const size_t warmup_loops = 256;
  for (size_t i = 0; i < warmup_loops; i++) {
    if (Skein_256_Init(&ctx, 256) != SKEIN_SUCCESS) {
      fprintf(stderr, "Skein_256_Init failed\n");
      exit(1);
    }
    Skein_256_Update(&ctx, msg, msg_bytes);
    Skein_256_Final(&ctx, out);
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
      if (Skein_256_Init(&ctx, 256) != SKEIN_SUCCESS) {
        fprintf(stderr, "Skein_256_Init failed\n");
        exit(1);
      }
      Skein_256_Update(&ctx, msg, msg_bytes);
      Skein_256_Final(&ctx, out);
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
  if (csv) bench_csv_write_row_hash(csv, "skein", "skein256_hash", 0, msg_bytes, reps, ns_med, ns_sd, mib_med, mib_sd, cyc_med, cyc_sd);

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
  printf("Skein-256 full (Init+Update+Final) benchmark  reps=%u\n", args.reps);
  run_case(csv, args.reps, 1024);
  run_case(csv, args.reps, 64ull * 1024ull);
  run_case(csv, args.reps, 1024ull * 1024ull);
  printf("sink=%llu\n", (unsigned long long)g_sink64);
  if (csv) fclose(csv);
  return 0;
}
