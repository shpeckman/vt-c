// src/vt-daemon.c
#include "vt.h"
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CONNECTIONS 50
#define TIMEOUT_SEC 10

static volatile sig_atomic_t keep_running = 1;
static volatile sig_atomic_t active_children = 0;

static void handle_signal(int signum) {
  (void)signum;
  keep_running = 0;
}

static void handle_sigchld(int signum) {
  (void)signum;
  int saved_errno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0) {
    if (active_children > 0) {
      active_children--;
    }
  }
  errno = saved_errno;
}

typedef struct {
  FILE *out;
  char apc_buf[256];
  size_t apc_len;
} client_ctx_t;

static void print_u8_array(FILE *out, const uint8_t *arr, int len) {
  fprintf(out, "[");
  for (int i = 0; i < len; i++) {
    fprintf(out, "%d%s", arr[i], i < len - 1 ? "," : "");
  }
  fprintf(out, "]");
}

static void print_params_array(FILE *out, const vt_param_t *params, int len) {
  fprintf(out, "[");
  for (int i = 0; i < len; i++) {
    fprintf(out, "{\"v\":%d,\"sub\":%s}%s", params[i].value,
            params[i].sub ? "true" : "false", i < len - 1 ? "," : "");
  }
  fprintf(out, "]");
}

static void cb_print(vt_parser_t *p, uint8_t c) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"print\",\"byte\":%d}\n", c);
}

static void cb_execute(vt_parser_t *p, uint8_t c) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"execute\",\"byte\":%d}\n", c);
}

static void cb_clear(vt_parser_t *p) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"clear\"}\n");
}

static void cb_collect(vt_parser_t *p, uint8_t c) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"collect\",\"byte\":%d}\n", c);
}

static void cb_param(vt_parser_t *p, uint8_t c) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"param\",\"byte\":%d}\n", c);
}

static void cb_esc_dispatch(vt_parser_t *p, uint8_t c,
                            const uint8_t *intermediates, int intermediates_len,
                            bool ignore) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out,
          "{\"event\":\"esc_dispatch\",\"byte\":%d,\"ignore\":%s,"
          "\"intermediates\":",
          c, ignore ? "true" : "false");
  print_u8_array(out, intermediates, intermediates_len);
  fprintf(out, "}\n");
}

static void cb_csi_dispatch(vt_parser_t *p, uint8_t c, const vt_param_t *params,
                            int params_len, const uint8_t *intermediates,
                            int intermediates_len, bool ignore) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out,
          "{\"event\":\"csi_dispatch\",\"byte\":%d,\"ignore\":%s,"
          "\"intermediates\":",
          c, ignore ? "true" : "false");
  print_u8_array(out, intermediates, intermediates_len);
  fprintf(out, ",\"params\":");
  print_params_array(out, params, params_len);
  fprintf(out, "}\n");
}

static void cb_hook(vt_parser_t *p, uint8_t c, const vt_param_t *params,
                    int params_len, const uint8_t *intermediates,
                    int intermediates_len, bool ignore) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out,
          "{\"event\":\"hook\",\"byte\":%d,\"ignore\":%s,\"intermediates\":", c,
          ignore ? "true" : "false");
  print_u8_array(out, intermediates, intermediates_len);
  fprintf(out, ",\"params\":");
  print_params_array(out, params, params_len);
  fprintf(out, "}\n");
}

static void cb_put(vt_parser_t *p, uint8_t c) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"put\",\"byte\":%d}\n", c);
}

static void cb_unhook(vt_parser_t *p) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"unhook\"}\n");
}

static void cb_osc_start(vt_parser_t *p) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"osc_start\"}\n");
}

static void cb_osc_put(vt_parser_t *p, uint8_t c) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"osc_put\",\"byte\":%d}\n", c);
}

static void cb_osc_end(vt_parser_t *p) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"osc_end\"}\n");
}

static void cb_sos_start(vt_parser_t *p) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"sos_start\"}\n");
}

static void cb_sos_put(vt_parser_t *p, uint8_t c) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"sos_put\",\"byte\":%d}\n", c);
}

static void cb_sos_end(vt_parser_t *p) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"sos_end\"}\n");
}

static void cb_pm_start(vt_parser_t *p) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"pm_start\"}\n");
}

static void cb_pm_put(vt_parser_t *p, uint8_t c) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"pm_put\",\"byte\":%d}\n", c);
}

static void cb_pm_end(vt_parser_t *p) {
  FILE *out = ((client_ctx_t *)p->user_data)->out;
  fprintf(out, "{\"event\":\"pm_end\"}\n");
}

static void cb_apc_start(vt_parser_t *p) {
  client_ctx_t *ctx = (client_ctx_t *)p->user_data;
  ctx->apc_len = 0;
  fprintf(ctx->out, "{\"event\":\"apc_start\"}\n");
}

static void cb_apc_put(vt_parser_t *p, uint8_t c) {
  client_ctx_t *ctx = (client_ctx_t *)p->user_data;
  if (ctx->apc_len < sizeof(ctx->apc_buf) - 1) {
    ctx->apc_buf[ctx->apc_len++] = (char)c;
  }
  fprintf(ctx->out, "{\"event\":\"apc_put\",\"byte\":%d}\n", c);
}

static void cb_apc_end(vt_parser_t *p) {
  client_ctx_t *ctx = (client_ctx_t *)p->user_data;
  fprintf(ctx->out, "{\"event\":\"apc_end\"}\n");

  ctx->apc_buf[ctx->apc_len] = '\0';

  if (strcmp(ctx->apc_buf, "VTD;health") == 0) {
    fprintf(ctx->out, "{\"status\":\"ok\"}\n");
  } else if (strcmp(ctx->apc_buf, "VTD;shutdown") == 0) {
    fprintf(ctx->out, "{\"status\":\"shutting_down\"}\n");
    kill(getppid(), SIGTERM);
  }
}

static vt_callbacks_t json_cb = {
    .print = cb_print,
    .execute = cb_execute,
    .clear = cb_clear,
    .collect = cb_collect,
    .param = cb_param,
    .esc_dispatch = cb_esc_dispatch,
    .csi_dispatch = cb_csi_dispatch,
    .hook = cb_hook,
    .put = cb_put,
    .unhook = cb_unhook,
    .osc_start = cb_osc_start,
    .osc_put = cb_osc_put,
    .osc_end = cb_osc_end,
    .sos_start = cb_sos_start,
    .sos_put = cb_sos_put,
    .sos_end = cb_sos_end,
    .pm_start = cb_pm_start,
    .pm_put = cb_pm_put,
    .pm_end = cb_pm_end,
    .apc_start = cb_apc_start,
    .apc_put = cb_apc_put,
    .apc_end = cb_apc_end,
};

static void run_parser(int fd) {
  struct timeval tv;
  tv.tv_sec = TIMEOUT_SEC;
  tv.tv_usec = 0;
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv, sizeof(tv));
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const void *)&tv, sizeof(tv));

  client_ctx_t ctx;
  ctx.out = fdopen(dup(fd), "w");
  if (!ctx.out) {
    return;
  }
  setvbuf(ctx.out, NULL, _IOLBF, 0);
  ctx.apc_len = 0;

  vt_parser_t parser;
  vt_init(&parser, &json_cb, &ctx);

  uint8_t buf[4096];
  ssize_t n;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    vt_parse(&parser, buf, n);
  }

  fclose(ctx.out);
}

static void daemonize_uds(const char *path) {
  int s = socket(AF_UNIX, SOCK_STREAM, 0);
  if (s < 0) {
    perror("socket");
    exit(1);
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

  unlink(path);

  mode_t old_mask = umask(0177);
  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(1);
  }
  umask(old_mask);

  if (listen(s, MAX_CONNECTIONS) < 0) {
    perror("listen");
    exit(1);
  }

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_signal;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  struct sigaction sa_chld;
  memset(&sa_chld, 0, sizeof(sa_chld));
  sa_chld.sa_handler = handle_sigchld;
  sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sa_chld, NULL);

  printf("Listening on Unix Domain Socket %s (Max connections: %d)...\n", path,
         MAX_CONNECTIONS);

  while (keep_running) {
    int client = accept(s, NULL, NULL);
    if (client < 0) {
      if (errno == EINTR) {
        continue;
      }
      continue;
    }

    if (active_children >= MAX_CONNECTIONS) {
      const char *err = "{\"error\":\"max_connections_reached\"}\n";
      write(client, err, strlen(err));
      close(client);
      continue;
    }

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    pid_t pid = fork();
    if (pid == 0) {
      sigprocmask(SIG_SETMASK, &oldmask, NULL);
      close(s);
      run_parser(client);
      close(client);
      exit(0);
    } else if (pid > 0) {
      active_children++;
    }

    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    close(client);
  }

  close(s);
  unlink(path);
  printf("Daemon shut down gracefully.\n");
}

static void print_usage(const char *prog_name) {
  printf("Usage: %s -s PATH\n\n", prog_name);
  printf("A language-agnostic Virtual Terminal (VT) escape sequence parser "
         "daemon.\n");
  printf("Reads raw terminal data from a Unix Domain Socket and emits NDJSON "
         "events.\n\n");

  printf("Options:\n");
  printf("  -h, --help      Show this help message and exit.\n");
  printf("  -s PATH         Run as a Unix Domain Socket (UDS) daemon on the "
         "specified path.\n\n");

  printf("Example Usage:\n");
  printf("  1. Start the daemon in the background:\n");
  printf("       %s -s /tmp/vt.sock &\n\n", prog_name);

  printf("  2. Send terminal data and stream NDJSON back (using netcat):\n");
  printf("       echo -n -e '\\x1B[1mBold' | nc -U /tmp/vt.sock\n\n");

  printf("  3. Check health status via APC protocol:\n");
  printf(
      "       echo -n -e '\\x1B_VTD;health\\x1B\\\\' | nc -U /tmp/vt.sock\n\n");

  printf("  4. Graceful shutdown via APC protocol:\n");
  printf("       echo -n -e '\\x1B_VTD;shutdown\\x1B\\\\' | nc -U "
         "/tmp/vt.sock\n\n");
}

int main(int argc, char **argv) {
  if (argc >= 2) {
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    } else if (strcmp(argv[1], "-s") == 0 && argc >= 3) {
      daemonize_uds(argv[2]);
      return 0;
    }
  }

  fprintf(stderr, "Error: Invalid arguments.\n\n");
  print_usage(argv[0]);
  return 1;
}