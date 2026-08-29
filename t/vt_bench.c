// t/vt_bench.c
#include "vt.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __MACH__
#include <mach/mach_time.h>
#endif

static vt_callbacks_t bench_cb = {0};

static double get_time_sec(void) {
#ifdef __MACH__
  static mach_timebase_info_data_t timebase;
  if (timebase.denom == 0)
    mach_timebase_info(&timebase);
  uint64_t t = mach_absolute_time();
  return (double)t * timebase.numer / timebase.denom / 1e9;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#endif
}

static char *repeat_str(const char *s, int count) {
  size_t len = strlen(s);
  char *res = malloc(len * count + 1);
  for (int i = 0; i < count; i++) {
    memcpy(res + i * len, s, len);
  }
  res[len * count] = '\0';
  return res;
}

static void run_bench(const char *name, const char *payload) {
  vt_parser_t parser;
  vt_init(&parser, &bench_cb, NULL);
  size_t len = strlen(payload);

  double start = get_time_sec();
  int iters = 0;
  double elapsed = 0;

  while (elapsed < 1.0) {
    for (int i = 0; i < 1000; i++) {
      vt_parse(&parser, (const uint8_t *)payload, len);
    }
    iters += 1000;
    elapsed = get_time_sec() - start;
  }

  double ops_per_sec = iters / elapsed;
  double mb_per_sec = (iters * len) / elapsed / 1024.0 / 1024.0;
  printf("%-20s: %12.2f ops/sec (%8.2f MB/s)\n", name, ops_per_sec, mb_per_sec);
}

int main(void) {
  char *plain_text = repeat_str("a", 10000);
  char *utf8_text = repeat_str("🚀", 2500);
  char *csi_heavy =
      repeat_str("\x1B[38:2:255:128:0m\x1B[1mHello\x1B[0m\x1B[H", 1000);

  char *osc_base = repeat_str("A", 10000);
  char *osc_stream = malloc(10000 + 10);
  sprintf(osc_stream, "\x1B]52;c;%s\x07", osc_base);

  char *dcs_base = repeat_str("B", 10000);
  char *dcs_stream = malloc(10000 + 10);
  sprintf(dcs_stream, "\x1BPq%s\x1B\\", dcs_base);

  char htop_payload[100 * 100] = {0};
  for (int i = 0; i < 100; i++) {
    char temp[100];
    sprintf(temp,
            "\x1B[%d;1H\x1B[K\x1B[32mTasks:\x1B[0m 120, \x1B[31mThr:\x1B[0m "
            "250 \x1B[1m[\x1B[34m||\x1B[0m\x1B[1m]\x1B[0m",
            i);
    strcat(htop_payload, temp);
  }

  printf("--- Benchmarks (Zero Allocation C Port) ---\n");
  run_bench("plain text", plain_text);
  run_bench("utf8 text", utf8_text);
  run_bench("csi heavy", csi_heavy);
  run_bench("osc stream", osc_stream);
  run_bench("dcs stream", dcs_stream);
  run_bench("realistic", htop_payload);

  free(plain_text);
  free(utf8_text);
  free(csi_heavy);
  free(osc_base);
  free(osc_stream);
  free(dcs_base);
  free(dcs_stream);

  return 0;
}
