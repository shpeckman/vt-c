// test.c
#include "vt.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char event_log[65536];
static size_t event_log_pos = 0;

static void LOG_EVENT(const char *fmt, ...) {
  if (event_log_pos >= sizeof(event_log) - 1)
    return;

  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(event_log + event_log_pos,
                    sizeof(event_log) - event_log_pos, fmt, args);
  va_end(args);

  if (n > 0) {
    size_t written = (size_t)n;
    if (written >= sizeof(event_log) - event_log_pos) {
      event_log_pos = sizeof(event_log) - 1;
    } else {
      event_log_pos += written;
    }
  }

  if (event_log_pos < sizeof(event_log) - 1) {
    event_log[event_log_pos++] = '\n';
    event_log[event_log_pos] = '\0';
  }
}

#define CLEAR_LOG()                                                            \
  do {                                                                         \
    event_log[0] = '\0';                                                       \
    event_log_pos = 0;                                                         \
  } while (0)

static void cb_print(vt_parser_t *p, uint8_t c) {
  (void)p;
  LOG_EVENT("print(%d)", c);
}
static void cb_execute(vt_parser_t *p, uint8_t c) {
  (void)p;
  LOG_EVENT("execute(%d)", c);
}
static void cb_clear(vt_parser_t *p) {
  (void)p;
  LOG_EVENT("clear");
}
static void cb_param(vt_parser_t *p, uint8_t c) {
  (void)p;
  LOG_EVENT("param(%d)", c);
}
static void cb_osc_start(vt_parser_t *p) {
  (void)p;
  LOG_EVENT("osc_start");
}
static void cb_osc_put(vt_parser_t *p, uint8_t c) {
  (void)p;
  LOG_EVENT("osc_put(%d)", c);
}
static void cb_osc_end(vt_parser_t *p) {
  (void)p;
  LOG_EVENT("osc_end");
}

static void cb_csi_dispatch(vt_parser_t *p, uint8_t c, const vt_param_t *params,
                            int params_len, const uint8_t *intermediates,
                            int intermediates_len, bool ignore) {
  (void)p;
  (void)intermediates;
  (void)intermediates_len;

  char pbuf[4096] = "[";
  size_t pos = 1;
  for (int i = 0; i < params_len; i++) {
    int n = snprintf(pbuf + pos, sizeof(pbuf) - pos, "{%d, %s}%s",
                     params[i].value, params[i].sub ? "true" : "false",
                     i == params_len - 1 ? "" : ", ");
    if (n > 0) {
      size_t written = (size_t)n;
      if (written >= sizeof(pbuf) - pos)
        pos = sizeof(pbuf) - 1;
      else
        pos += written;
    }
  }
  snprintf(pbuf + pos, sizeof(pbuf) - pos, "]");
  LOG_EVENT("csi_dispatch(%d, %s, [], %s)", c, pbuf, ignore ? "true" : "false");
}

static vt_callbacks_t test_cb = {
    .print = cb_print,
    .execute = cb_execute,
    .clear = cb_clear,
    .param = cb_param,
    .csi_dispatch = cb_csi_dispatch,
    .osc_start = cb_osc_start,
    .osc_put = cb_osc_put,
    .osc_end = cb_osc_end,
};

static void assert_log_equals(const char *expected) {
  if (strcmp(event_log, expected) != 0) {
    fprintf(stderr, "EXPECTED:\n%s\nGOT:\n%s\n", expected, event_log);
    assert(false);
  }
}

static void test_plain_text() {
  vt_parser_t parser;
  vt_init(&parser, &test_cb, NULL);
  CLEAR_LOG();
  const char *seq = "abc";
  vt_parse(&parser, (const uint8_t *)seq, strlen(seq));
  assert_log_equals("print(97)\n"
                    "print(98)\n"
                    "print(99)\n");
}

static void test_basic_csi() {
  vt_parser_t parser;
  vt_init(&parser, &test_cb, NULL);
  CLEAR_LOG();
  const char *seq = "\x1B[m";
  vt_parse(&parser, (const uint8_t *)seq, strlen(seq));
  assert_log_equals("clear\n"
                    "csi_dispatch(109, [], [], false)\n");
}

static void test_csi_params() {
  vt_parser_t parser;
  vt_init(&parser, &test_cb, NULL);
  CLEAR_LOG();
  const char *seq = "\x1B[1;23m";
  vt_parse(&parser, (const uint8_t *)seq, strlen(seq));
  assert_log_equals(
      "clear\n"
      "param(49)\n"
      "param(59)\n"
      "param(50)\n"
      "param(51)\n"
      "csi_dispatch(109, [{1, false}, {23, false}], [], false)\n");
}

static void test_csi_sub_params() {
  vt_parser_t parser;
  vt_init(&parser, &test_cb, NULL);
  CLEAR_LOG();
  const char *seq = "\x1B[38:2:255:0:0m";
  vt_parse(&parser, (const uint8_t *)seq, strlen(seq));
  assert_log_equals("clear\n"
                    "param(51)\n"
                    "param(56)\n"
                    "param(58)\n"
                    "param(50)\n"
                    "param(58)\n"
                    "param(50)\n"
                    "param(53)\n"
                    "param(53)\n"
                    "param(58)\n"
                    "param(48)\n"
                    "param(58)\n"
                    "param(48)\n"
                    "csi_dispatch(109, [{38, false}, {2, true}, {255, true}, "
                    "{0, true}, {0, true}], [], false)\n");
}

static void test_csi_ignore_limit() {
  vt_parser_t parser;
  vt_init(&parser, &test_cb, NULL);
  CLEAR_LOG();

  char seq[256] = "\x1B[";
  for (int i = 0; i < 70; i++)
    strcat(seq, "1;");
  strcat(seq, "m");

  vt_parse(&parser, (const uint8_t *)seq, strlen(seq));
  assert(strstr(event_log, "true)") != NULL); // Ignore flag should be true
}

static void test_osc_string() {
  vt_parser_t parser;
  vt_init(&parser, &test_cb, NULL);
  CLEAR_LOG();
  const char *seq = "\x1B]0;title\x07";
  vt_parse(&parser, (const uint8_t *)seq, strlen(seq));
  assert_log_equals("clear\n"
                    "osc_start\n"
                    "osc_put(48)\n"
                    "osc_put(59)\n"
                    "osc_put(116)\n"
                    "osc_put(105)\n"
                    "osc_put(116)\n"
                    "osc_put(108)\n"
                    "osc_put(101)\n"
                    "osc_end\n");
}

static void test_execute_in_sequence() {
  vt_parser_t parser;
  vt_init(&parser, &test_cb, NULL);
  CLEAR_LOG();
  const char *seq = "\x1B[1\nm";
  vt_parse(&parser, (const uint8_t *)seq, strlen(seq));
  assert_log_equals("clear\n"
                    "param(49)\n"
                    "execute(10)\n"
                    "csi_dispatch(109, [{1, false}], [], false)\n");
}

static void test_idle_state() {
  vt_parser_t parser;
  vt_init(&parser, &test_cb, NULL);
  assert(vt_idle(&parser) == true);

  const char *seq1 = "\x1B[";
  vt_parse(&parser, (const uint8_t *)seq1, strlen(seq1));
  assert(vt_idle(&parser) == false);

  const char *seq2 = "m";
  vt_parse(&parser, (const uint8_t *)seq2, strlen(seq2));
  assert(vt_idle(&parser) == true);
}

static void test_utf8_decoder() {
  vt_utf8_t utf8;
  uint32_t cp;
  bool ready;

  vt_utf8_init(&utf8);

  // Single byte 'a'
  ready = vt_utf8_decode(&utf8, 97, &cp);
  assert(ready && cp == 97);

  // Multi-byte '€' (E2 82 AC)
  ready = vt_utf8_decode(&utf8, 0xE2, &cp);
  assert(!ready);
  ready = vt_utf8_decode(&utf8, 0x82, &cp);
  assert(!ready);
  ready = vt_utf8_decode(&utf8, 0xAC, &cp);
  assert(ready && cp == 0x20AC);

  // 4-byte '🚀' (F0 9F 9A 80)
  ready = vt_utf8_decode(&utf8, 0xF0, &cp);
  assert(!ready);
  ready = vt_utf8_decode(&utf8, 0x9F, &cp);
  assert(!ready);
  ready = vt_utf8_decode(&utf8, 0x9A, &cp);
  assert(!ready);
  ready = vt_utf8_decode(&utf8, 0x80, &cp);
  assert(ready && cp == 0x1F680);

  // Invalid starting byte
  ready = vt_utf8_decode(&utf8, 0xFF, &cp);
  assert(ready && cp == 0xFFFD);
}

int main(void) {
  printf("Running tests...\n");
  test_plain_text();
  test_basic_csi();
  test_csi_params();
  test_csi_sub_params();
  test_csi_ignore_limit();
  test_osc_string();
  test_execute_in_sequence();
  test_idle_state();
  test_utf8_decoder();
  printf("All tests passed!\n");
  return 0;
}