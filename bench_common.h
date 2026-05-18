#ifndef BENCH_COMMON_H
#define BENCH_COMMON_H

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#if defined(__i386__) || defined(__x86_64__)
#include <cpuid.h>
#include <x86intrin.h>
#endif

typedef struct {
  uint32_t rounds;
  uint32_t reps;
  const char *csv_path;
} bench_args_t;

static __inline__ void bench_enable_amd64_optimizations(void) {
#if defined(__x86_64__) || defined(_M_X64)
  HANDLE proc = GetCurrentProcess();
  HANDLE th = GetCurrentThread();
  SetPriorityClass(proc, HIGH_PRIORITY_CLASS);
  SetThreadPriority(th, THREAD_PRIORITY_HIGHEST);
  DWORD_PTR pm = 0, sm = 0;
  if (GetProcessAffinityMask(proc, &pm, &sm) && pm) {
    DWORD_PTR mask = pm & (~(pm - 1));
    if (mask) SetThreadAffinityMask(th, mask);
  }
  SetThreadIdealProcessor(th, 0);
#endif
}

static __inline__ double bench_qpc_seconds(void) {
  LARGE_INTEGER freq;
  LARGE_INTEGER now;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&now);
  return (double)now.QuadPart / (double)freq.QuadPart;
}

#if defined(__i386__) || defined(__x86_64__)
static __inline__ uint64_t bench_tsc_start(void) {
  unsigned int a, b, c, d;
  __cpuid(0, a, b, c, d);
  return __rdtsc();
}

static __inline__ uint64_t bench_tsc_stop(void) {
  unsigned int a, b, c, d;
  unsigned int aux = 0;
  uint64_t t = __rdtscp(&aux);
  __cpuid(0, a, b, c, d);
  return t;
}
#endif

static __inline__ uint32_t bench_parse_u32(const char *s, uint32_t def) {
  if (!s || !*s) return def;
  char *end = NULL;
  unsigned long v = strtoul(s, &end, 10);
  if (!end || *end) return def;
  return (uint32_t)v;
}

static __inline__ void bench_parse_args(int argc, char **argv, uint32_t default_rounds, uint32_t default_reps, bench_args_t *out) {
  out->rounds = default_rounds;
  out->reps = default_reps;
  out->csv_path = NULL;
  for (int i = 1; i < argc; i++) {
    const char *a = argv[i];
    if (!strcmp(a, "--rounds") && i + 1 < argc) {
      out->rounds = bench_parse_u32(argv[++i], out->rounds);
    } else if (!strcmp(a, "--reps") && i + 1 < argc) {
      out->reps = bench_parse_u32(argv[++i], out->reps);
      if (out->reps == 0) out->reps = 1;
    } else if (!strcmp(a, "--csv") && i + 1 < argc) {
      out->csv_path = argv[++i];
    } else if (a[0] != '-' && i == 1) {
      out->rounds = bench_parse_u32(a, out->rounds);
    }
  }
}

static int bench_cmp_double(const void *a, const void *b) {
  double da = *(const double *)a;
  double db = *(const double *)b;
  return (da > db) - (da < db);
}

static __inline__ double bench_median(const double *x, size_t n) {
  if (n == 0) return 0.0;
  double *tmp = (double *)malloc(n * sizeof(double));
  if (!tmp) return x[0];
  memcpy(tmp, x, n * sizeof(double));
  qsort(tmp, n, sizeof(double), bench_cmp_double);
  double med = (n & 1) ? tmp[n / 2] : 0.5 * (tmp[n / 2 - 1] + tmp[n / 2]);
  free(tmp);
  return med;
}

static __inline__ double bench_stddev(const double *x, size_t n) {
  if (n <= 1) return 0.0;
  double mean = 0.0;
  for (size_t i = 0; i < n; i++) mean += x[i];
  mean /= (double)n;
  double s2 = 0.0;
  for (size_t i = 0; i < n; i++) {
    double d = x[i] - mean;
    s2 += d * d;
  }
  s2 /= (double)(n - 1);
  return sqrt(s2);
}

static __inline__ const char *bench_basename(const char *path) {
  const char *p = path;
  for (const char *q = path; *q; q++) {
    if (*q == '\\' || *q == '/') p = q + 1;
  }
  return p;
}

static __inline__ void bench_build_csv_path(const char *argv0, char *dst, size_t dst_sz) {
  const char *base = bench_basename(argv0);
  size_t n = 0;
  while (base[n] && base[n] != '.' && n + 1 < dst_sz) {
    dst[n] = base[n];
    n++;
  }
  const char *ext = ".csv";
  size_t e = 0;
  while (ext[e] && n + 1 < dst_sz) {
    dst[n++] = ext[e++];
  }
  dst[n] = 0;
}

static __inline__ FILE *bench_csv_open(const char *argv0, const char *csv_path) {
  char buf[260];
  const char *path = csv_path;
  if (!path) {
    bench_build_csv_path(argv0, buf, sizeof(buf));
    path = buf;
  }
  FILE *f = fopen(path, "wb");
  return f;
}

static __inline__ void bench_csv_write_header_hash(FILE *f) {
  fprintf(f, "bench,test,rounds,msg_bytes,reps,ns_per_byte_median,ns_per_byte_stddev,mib_per_s_median,mib_per_s_stddev,cycles_per_byte_median,cycles_per_byte_stddev\n");
}

static __inline__ void bench_csv_write_row_hash(FILE *f, const char *bench, const char *test, uint32_t rounds, size_t msg_bytes, uint32_t reps,
                                                double ns_med, double ns_sd, double mib_med, double mib_sd, double cyc_med, double cyc_sd) {
  fprintf(f, "%s,%s,%u,%zu,%u,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f\n", bench, test, rounds, msg_bytes, reps, ns_med, ns_sd, mib_med, mib_sd, cyc_med, cyc_sd);
}

static __inline__ void bench_csv_write_header_micro(FILE *f) {
  fprintf(f, "bench,test,variant,lanes,iters,reps,cycles_per_op_median,cycles_per_op_stddev,ns_per_op_median,ns_per_op_stddev\n");
}

static __inline__ void bench_csv_write_row_micro(FILE *f, const char *bench, const char *test, const char *variant, uint32_t lanes, uint32_t iters, uint32_t reps,
                                                 double cyc_med, double cyc_sd, double ns_med, double ns_sd) {
  fprintf(f, "%s,%s,%s,%u,%u,%u,%.9f,%.9f,%.9f,%.9f\n", bench, test, variant, lanes, iters, reps, cyc_med, cyc_sd, ns_med, ns_sd);
}

#endif
