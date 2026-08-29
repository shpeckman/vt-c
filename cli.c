// cli.c
#include "vt.h"
#include <stdio.h>

static void print_u8_array(const uint8_t *arr, int len) {
    printf("[");
    for (int i = 0; i < len; i++) {
        printf("%d%s", arr[i], i < len - 1 ? "," : "");
    }
    printf("]");
}

static void print_params_array(const vt_param_t *params, int len) {
    printf("[");
    for (int i = 0; i < len; i++) {
        printf("{\"v\":%d,\"sub\":%s}%s", params[i].value, params[i].sub ? "true" : "false", i < len - 1 ? "," : "");
    }
    printf("]");
}

static void cb_print(vt_parser_t *p, uint8_t c) {
    (void)p;
    printf("{\"event\":\"print\",\"byte\":%d}\n", c);
}

static void cb_execute(vt_parser_t *p, uint8_t c) {
    (void)p;
    printf("{\"event\":\"execute\",\"byte\":%d}\n", c);
}

static void cb_clear(vt_parser_t *p) {
    (void)p;
    printf("{\"event\":\"clear\"}\n");
}

static void cb_collect(vt_parser_t *p, uint8_t c) {
    (void)p;
    printf("{\"event\":\"collect\",\"byte\":%d}\n", c);
}

static void cb_param(vt_parser_t *p, uint8_t c) {
    (void)p;
    printf("{\"event\":\"param\",\"byte\":%d}\n", c);
}

static void cb_esc_dispatch(vt_parser_t *p, uint8_t c, const uint8_t *intermediates, int intermediates_len, bool ignore) {
    (void)p;
    printf("{\"event\":\"esc_dispatch\",\"byte\":%d,\"ignore\":%s,\"intermediates\":", c, ignore ? "true" : "false");
    print_u8_array(intermediates, intermediates_len);
    printf("}\n");
}

static void cb_csi_dispatch(vt_parser_t *p, uint8_t c, const vt_param_t *params, int params_len, const uint8_t *intermediates, int intermediates_len, bool ignore) {
    (void)p;
    printf("{\"event\":\"csi_dispatch\",\"byte\":%d,\"ignore\":%s,\"intermediates\":", c, ignore ? "true" : "false");
    print_u8_array(intermediates, intermediates_len);
    printf(",\"params\":");
    print_params_array(params, params_len);
    printf("}\n");
}

static void cb_hook(vt_parser_t *p, uint8_t c, const vt_param_t *params, int params_len, const uint8_t *intermediates, int intermediates_len, bool ignore) {
    (void)p;
    printf("{\"event\":\"hook\",\"byte\":%d,\"ignore\":%s,\"intermediates\":", c, ignore ? "true" : "false");
    print_u8_array(intermediates, intermediates_len);
    printf(",\"params\":");
    print_params_array(params, params_len);
    printf("}\n");
}

static void cb_put(vt_parser_t *p, uint8_t c) {
    (void)p;
    printf("{\"event\":\"put\",\"byte\":%d}\n", c);
}

static void cb_unhook(vt_parser_t *p) {
    (void)p;
    printf("{\"event\":\"unhook\"}\n");
}

static void cb_osc_start(vt_parser_t *p) {
    (void)p;
    printf("{\"event\":\"osc_start\"}\n");
}

static void cb_osc_put(vt_parser_t *p, uint8_t c) {
    (void)p;
    printf("{\"event\":\"osc_put\",\"byte\":%d}\n", c);
}

static void cb_osc_end(vt_parser_t *p) {
    (void)p;
    printf("{\"event\":\"osc_end\"}\n");
}

static void cb_sos_start(vt_parser_t *p) {
    (void)p;
    printf("{\"event\":\"sos_start\"}\n");
}

static void cb_sos_put(vt_parser_t *p, uint8_t c) {
    (void)p;
    printf("{\"event\":\"sos_put\",\"byte\":%d}\n", c);
}

static void cb_sos_end(vt_parser_t *p) {
    (void)p;
    printf("{\"event\":\"sos_end\"}\n");
}

static void cb_pm_start(vt_parser_t *p) {
    (void)p;
    printf("{\"event\":\"pm_start\"}\n");
}

static void cb_pm_put(vt_parser_t *p, uint8_t c) {
    (void)p;
    printf("{\"event\":\"pm_put\",\"byte\":%d}\n", c);
}

static void cb_pm_end(vt_parser_t *p) {
    (void)p;
    printf("{\"event\":\"pm_end\"}\n");
}

static void cb_apc_start(vt_parser_t *p) {
    (void)p;
    printf("{\"event\":\"apc_start\"}\n");
}

static void cb_apc_put(vt_parser_t *p, uint8_t c) {
    (void)p;
    printf("{\"event\":\"apc_put\",\"byte\":%d}\n", c);
}

static void cb_apc_end(vt_parser_t *p) {
    (void)p;
    printf("{\"event\":\"apc_end\"}\n");
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

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    vt_parser_t parser;
    vt_init(&parser, &json_cb, NULL);
    uint8_t buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), stdin)) > 0) {
        vt_parse(&parser, buf, n);
    }
    return 0;
}