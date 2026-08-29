// cli.c
#include "vt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

typedef struct {
    FILE *out;
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
        fprintf(out, "{\"v\":%d,\"sub\":%s}%s", params[i].value, params[i].sub ? "true" : "false", i < len - 1 ? "," : "");
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

static void cb_esc_dispatch(vt_parser_t *p, uint8_t c, const uint8_t *intermediates, int intermediates_len, bool ignore) {
    FILE *out = ((client_ctx_t *)p->user_data)->out;
    fprintf(out, "{\"event\":\"esc_dispatch\",\"byte\":%d,\"ignore\":%s,\"intermediates\":", c, ignore ? "true" : "false");
    print_u8_array(out, intermediates, intermediates_len);
    fprintf(out, "}\n");
}

static void cb_csi_dispatch(vt_parser_t *p, uint8_t c, const vt_param_t *params, int params_len, const uint8_t *intermediates, int intermediates_len, bool ignore) {
    FILE *out = ((client_ctx_t *)p->user_data)->out;
    fprintf(out, "{\"event\":\"csi_dispatch\",\"byte\":%d,\"ignore\":%s,\"intermediates\":", c, ignore ? "true" : "false");
    print_u8_array(out, intermediates, intermediates_len);
    fprintf(out, ",\"params\":");
    print_params_array(out, params, params_len);
    fprintf(out, "}\n");
}

static void cb_hook(vt_parser_t *p, uint8_t c, const vt_param_t *params, int params_len, const uint8_t *intermediates, int intermediates_len, bool ignore) {
    FILE *out = ((client_ctx_t *)p->user_data)->out;
    fprintf(out, "{\"event\":\"hook\",\"byte\":%d,\"ignore\":%s,\"intermediates\":", c, ignore ? "true" : "false");
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
    FILE *out = ((client_ctx_t *)p->user_data)->out;
    fprintf(out, "{\"event\":\"apc_start\"}\n");
}

static void cb_apc_put(vt_parser_t *p, uint8_t c) {
    FILE *out = ((client_ctx_t *)p->user_data)->out;
    fprintf(out, "{\"event\":\"apc_put\",\"byte\":%d}\n", c);
}

static void cb_apc_end(vt_parser_t *p) {
    FILE *out = ((client_ctx_t *)p->user_data)->out;
    fprintf(out, "{\"event\":\"apc_end\"}\n");
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

static void run_parser(int in_fd, int out_fd) {
    client_ctx_t ctx;
    bool is_stdio = (out_fd == STDOUT_FILENO);
    
    if (is_stdio) {
        ctx.out = stdout;
    } else {
        ctx.out = fdopen(dup(out_fd), "w");
    }
    
    setvbuf(ctx.out, NULL, _IOLBF, 0);

    vt_parser_t parser;
    vt_init(&parser, &json_cb, &ctx);

    uint8_t buf[4096];
    ssize_t n;
    while ((n = read(in_fd, buf, sizeof(buf))) > 0) {
        vt_parse(&parser, buf, n);
    }

    if (!is_stdio) {
        fclose(ctx.out);
    }
}

static void daemonize_tcp(int port) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(s, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Listening on TCP port %d...\n", port);
    signal(SIGCHLD, SIG_IGN); // Auto-reap zombie processes

    while (1) {
        int client = accept(s, NULL, NULL);
        if (client < 0) continue;
        if (fork() == 0) {
            close(s);
            run_parser(client, client);
            close(client);
            exit(0);
        }
        close(client);
    }
}

static void daemonize_uds(const char *path) {
    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); exit(1); }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    unlink(path); // Remove existing socket if any
    
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(s, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Listening on Unix Domain Socket %s...\n", path);
    signal(SIGCHLD, SIG_IGN); // Auto-reap zombie processes

    while (1) {
        int client = accept(s, NULL, NULL);
        if (client < 0) continue;
        if (fork() == 0) {
            close(s);
            run_parser(client, client);
            close(client);
            exit(0);
        }
        close(client);
    }
}

int main(int argc, char **argv) {
    if (argc >= 3 && strcmp(argv[1], "-p") == 0) {
        daemonize_tcp(atoi(argv[2]));
    } else if (argc >= 3 && strcmp(argv[1], "-s") == 0) {
        daemonize_uds(argv[2]);
    } else {
        // Run in standard stdio mode
        run_parser(STDIN_FILENO, STDOUT_FILENO);
    }
    return 0;
}