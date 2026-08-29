// vt.c
#include "vt.h"

static uint8_t vt_table[16][256];
static bool vt_table_initialized = false;

#define VT_TRANS(st, start, end, act, nxt)                                     \
  for (int i = (start); i <= (end); i++) {                                     \
    vt_table[st][i] = (uint8_t)(((act) << 4) | (nxt));                         \
  }

#define VT_ANY(start, end, act, nxt)                                           \
  for (int s = 0; s < 16; s++) {                                               \
    VT_TRANS(s, start, end, act, nxt)                                          \
  }

static void vt_init_table(void) {
  if (vt_table_initialized)
    return;

  VT_ANY(0x18, 0x18, VT_ACT_EXEC, VT_STATE_GND)
  VT_ANY(0x1A, 0x1A, VT_ACT_EXEC, VT_STATE_GND)
  VT_ANY(0x1B, 0x1B, VT_ACT_CLR, VT_STATE_ESC)

  VT_TRANS(VT_STATE_GND, 0x00, 0x17, VT_ACT_EXEC, VT_STATE_GND)
  VT_TRANS(VT_STATE_GND, 0x19, 0x19, VT_ACT_EXEC, VT_STATE_GND)
  VT_TRANS(VT_STATE_GND, 0x1C, 0x1F, VT_ACT_EXEC, VT_STATE_GND)
  VT_TRANS(VT_STATE_GND, 0x20, 0xFF, VT_ACT_PRN, VT_STATE_GND)

  VT_TRANS(VT_STATE_ESC, 0x00, 0x17, VT_ACT_EXEC, VT_STATE_ESC)
  VT_TRANS(VT_STATE_ESC, 0x19, 0x19, VT_ACT_EXEC, VT_STATE_ESC)
  VT_TRANS(VT_STATE_ESC, 0x1C, 0x1F, VT_ACT_EXEC, VT_STATE_ESC)
  VT_TRANS(VT_STATE_ESC, 0x7F, 0x7F, VT_ACT_IGN, VT_STATE_ESC)
  VT_TRANS(VT_STATE_ESC, 0x80, 0xFF, VT_ACT_PRN, VT_STATE_GND)
  VT_TRANS(VT_STATE_ESC, 0x20, 0x2F, VT_ACT_COLL, VT_STATE_ESC_INT)
  VT_TRANS(VT_STATE_ESC, 0x30, 0x4F, VT_ACT_ESC_DISP, VT_STATE_GND)
  VT_TRANS(VT_STATE_ESC, 0x51, 0x57, VT_ACT_ESC_DISP, VT_STATE_GND)
  VT_TRANS(VT_STATE_ESC, 0x59, 0x59, VT_ACT_ESC_DISP, VT_STATE_GND)
  VT_TRANS(VT_STATE_ESC, 0x5A, 0x5A, VT_ACT_ESC_DISP, VT_STATE_GND)
  VT_TRANS(VT_STATE_ESC, 0x5C, 0x5C, VT_ACT_ESC_DISP, VT_STATE_GND)
  VT_TRANS(VT_STATE_ESC, 0x60, 0x7E, VT_ACT_ESC_DISP, VT_STATE_GND)
  VT_TRANS(VT_STATE_ESC, 0x50, 0x50, VT_ACT_NONE, VT_STATE_DCS_ENT)
  VT_TRANS(VT_STATE_ESC, 0x5D, 0x5D, VT_ACT_NONE, VT_STATE_OSC_STR)
  VT_TRANS(VT_STATE_ESC, 0x5B, 0x5B, VT_ACT_NONE, VT_STATE_CSI_ENT)
  VT_TRANS(VT_STATE_ESC, 0x58, 0x58, VT_ACT_NONE, VT_STATE_SOS_STR)
  VT_TRANS(VT_STATE_ESC, 0x5E, 0x5E, VT_ACT_NONE, VT_STATE_PM_STR)
  VT_TRANS(VT_STATE_ESC, 0x5F, 0x5F, VT_ACT_NONE, VT_STATE_APC_STR)

  VT_TRANS(VT_STATE_ESC_INT, 0x00, 0x17, VT_ACT_EXEC, VT_STATE_ESC_INT)
  VT_TRANS(VT_STATE_ESC_INT, 0x19, 0x19, VT_ACT_EXEC, VT_STATE_ESC_INT)
  VT_TRANS(VT_STATE_ESC_INT, 0x1C, 0x1F, VT_ACT_EXEC, VT_STATE_ESC_INT)
  VT_TRANS(VT_STATE_ESC_INT, 0x20, 0x2F, VT_ACT_COLL, VT_STATE_ESC_INT)
  VT_TRANS(VT_STATE_ESC_INT, 0x7F, 0x7F, VT_ACT_IGN, VT_STATE_ESC_INT)
  VT_TRANS(VT_STATE_ESC_INT, 0x80, 0xFF, VT_ACT_PRN, VT_STATE_GND)
  VT_TRANS(VT_STATE_ESC_INT, 0x30, 0x7E, VT_ACT_ESC_DISP, VT_STATE_GND)

  VT_TRANS(VT_STATE_CSI_ENT, 0x00, 0x17, VT_ACT_EXEC, VT_STATE_CSI_ENT)
  VT_TRANS(VT_STATE_CSI_ENT, 0x19, 0x19, VT_ACT_EXEC, VT_STATE_CSI_ENT)
  VT_TRANS(VT_STATE_CSI_ENT, 0x1C, 0x1F, VT_ACT_EXEC, VT_STATE_CSI_ENT)
  VT_TRANS(VT_STATE_CSI_ENT, 0x7F, 0x7F, VT_ACT_IGN, VT_STATE_CSI_ENT)
  VT_TRANS(VT_STATE_CSI_ENT, 0x80, 0xFF, VT_ACT_PRN, VT_STATE_GND)
  VT_TRANS(VT_STATE_CSI_ENT, 0x20, 0x2F, VT_ACT_COLL, VT_STATE_CSI_INT)
  VT_TRANS(VT_STATE_CSI_ENT, 0x30, 0x39, VT_ACT_PRM, VT_STATE_CSI_PRM)
  VT_TRANS(VT_STATE_CSI_ENT, 0x3A, 0x3A, VT_ACT_PRM, VT_STATE_CSI_PRM)
  VT_TRANS(VT_STATE_CSI_ENT, 0x3B, 0x3B, VT_ACT_PRM, VT_STATE_CSI_PRM)
  VT_TRANS(VT_STATE_CSI_ENT, 0x3C, 0x3F, VT_ACT_COLL, VT_STATE_CSI_PRM)
  VT_TRANS(VT_STATE_CSI_ENT, 0x40, 0x7E, VT_ACT_CSI_DISP, VT_STATE_GND)

  VT_TRANS(VT_STATE_CSI_PRM, 0x00, 0x17, VT_ACT_EXEC, VT_STATE_CSI_PRM)
  VT_TRANS(VT_STATE_CSI_PRM, 0x19, 0x19, VT_ACT_EXEC, VT_STATE_CSI_PRM)
  VT_TRANS(VT_STATE_CSI_PRM, 0x1C, 0x1F, VT_ACT_EXEC, VT_STATE_CSI_PRM)
  VT_TRANS(VT_STATE_CSI_PRM, 0x30, 0x39, VT_ACT_PRM, VT_STATE_CSI_PRM)
  VT_TRANS(VT_STATE_CSI_PRM, 0x3A, 0x3A, VT_ACT_PRM, VT_STATE_CSI_PRM)
  VT_TRANS(VT_STATE_CSI_PRM, 0x3B, 0x3B, VT_ACT_PRM, VT_STATE_CSI_PRM)
  VT_TRANS(VT_STATE_CSI_PRM, 0x7F, 0x7F, VT_ACT_IGN, VT_STATE_CSI_PRM)
  VT_TRANS(VT_STATE_CSI_PRM, 0x80, 0xFF, VT_ACT_PRN, VT_STATE_GND)
  VT_TRANS(VT_STATE_CSI_PRM, 0x3C, 0x3F, VT_ACT_NONE, VT_STATE_CSI_IGN)
  VT_TRANS(VT_STATE_CSI_PRM, 0x20, 0x2F, VT_ACT_COLL, VT_STATE_CSI_INT)
  VT_TRANS(VT_STATE_CSI_PRM, 0x40, 0x7E, VT_ACT_CSI_DISP, VT_STATE_GND)

  VT_TRANS(VT_STATE_CSI_INT, 0x00, 0x17, VT_ACT_EXEC, VT_STATE_CSI_INT)
  VT_TRANS(VT_STATE_CSI_INT, 0x19, 0x19, VT_ACT_EXEC, VT_STATE_CSI_INT)
  VT_TRANS(VT_STATE_CSI_INT, 0x1C, 0x1F, VT_ACT_EXEC, VT_STATE_CSI_INT)
  VT_TRANS(VT_STATE_CSI_INT, 0x20, 0x2F, VT_ACT_COLL, VT_STATE_CSI_INT)
  VT_TRANS(VT_STATE_CSI_INT, 0x7F, 0x7F, VT_ACT_IGN, VT_STATE_CSI_INT)
  VT_TRANS(VT_STATE_CSI_INT, 0x80, 0xFF, VT_ACT_PRN, VT_STATE_GND)
  VT_TRANS(VT_STATE_CSI_INT, 0x30, 0x3F, VT_ACT_NONE, VT_STATE_CSI_IGN)
  VT_TRANS(VT_STATE_CSI_INT, 0x40, 0x7E, VT_ACT_CSI_DISP, VT_STATE_GND)

  VT_TRANS(VT_STATE_CSI_IGN, 0x00, 0x17, VT_ACT_EXEC, VT_STATE_CSI_IGN)
  VT_TRANS(VT_STATE_CSI_IGN, 0x19, 0x19, VT_ACT_EXEC, VT_STATE_CSI_IGN)
  VT_TRANS(VT_STATE_CSI_IGN, 0x1C, 0x1F, VT_ACT_EXEC, VT_STATE_CSI_IGN)
  VT_TRANS(VT_STATE_CSI_IGN, 0x20, 0x3F, VT_ACT_IGN, VT_STATE_CSI_IGN)
  VT_TRANS(VT_STATE_CSI_IGN, 0x7F, 0x7F, VT_ACT_IGN, VT_STATE_CSI_IGN)
  VT_TRANS(VT_STATE_CSI_IGN, 0x80, 0xFF, VT_ACT_PRN, VT_STATE_GND)
  VT_TRANS(VT_STATE_CSI_IGN, 0x40, 0x7E, VT_ACT_NONE, VT_STATE_GND)

  VT_TRANS(VT_STATE_DCS_ENT, 0x00, 0x17, VT_ACT_IGN, VT_STATE_DCS_ENT)
  VT_TRANS(VT_STATE_DCS_ENT, 0x19, 0x19, VT_ACT_IGN, VT_STATE_DCS_ENT)
  VT_TRANS(VT_STATE_DCS_ENT, 0x1C, 0x1F, VT_ACT_IGN, VT_STATE_DCS_ENT)
  VT_TRANS(VT_STATE_DCS_ENT, 0x7F, 0x7F, VT_ACT_IGN, VT_STATE_DCS_ENT)
  VT_TRANS(VT_STATE_DCS_ENT, 0x80, 0xFF, VT_ACT_PRN, VT_STATE_GND)
  VT_TRANS(VT_STATE_DCS_ENT, 0x20, 0x2F, VT_ACT_COLL, VT_STATE_DCS_INT)
  VT_TRANS(VT_STATE_DCS_ENT, 0x30, 0x39, VT_ACT_PRM, VT_STATE_DCS_PRM)
  VT_TRANS(VT_STATE_DCS_ENT, 0x3A, 0x3A, VT_ACT_PRM, VT_STATE_DCS_PRM)
  VT_TRANS(VT_STATE_DCS_ENT, 0x3B, 0x3B, VT_ACT_PRM, VT_STATE_DCS_PRM)
  VT_TRANS(VT_STATE_DCS_ENT, 0x3C, 0x3F, VT_ACT_COLL, VT_STATE_DCS_PRM)
  VT_TRANS(VT_STATE_DCS_ENT, 0x40, 0x7E, VT_ACT_HOOK, VT_STATE_DCS_PASS)

  VT_TRANS(VT_STATE_DCS_PRM, 0x00, 0x17, VT_ACT_IGN, VT_STATE_DCS_PRM)
  VT_TRANS(VT_STATE_DCS_PRM, 0x19, 0x19, VT_ACT_IGN, VT_STATE_DCS_PRM)
  VT_TRANS(VT_STATE_DCS_PRM, 0x1C, 0x1F, VT_ACT_IGN, VT_STATE_DCS_PRM)
  VT_TRANS(VT_STATE_DCS_PRM, 0x30, 0x39, VT_ACT_PRM, VT_STATE_DCS_PRM)
  VT_TRANS(VT_STATE_DCS_PRM, 0x3A, 0x3A, VT_ACT_PRM, VT_STATE_DCS_PRM)
  VT_TRANS(VT_STATE_DCS_PRM, 0x3B, 0x3B, VT_ACT_PRM, VT_STATE_DCS_PRM)
  VT_TRANS(VT_STATE_DCS_PRM, 0x7F, 0x7F, VT_ACT_IGN, VT_STATE_DCS_PRM)
  VT_TRANS(VT_STATE_DCS_PRM, 0x80, 0xFF, VT_ACT_PRN, VT_STATE_GND)
  VT_TRANS(VT_STATE_DCS_PRM, 0x3C, 0x3F, VT_ACT_NONE, VT_STATE_DCS_IGN)
  VT_TRANS(VT_STATE_DCS_PRM, 0x20, 0x2F, VT_ACT_COLL, VT_STATE_DCS_INT)
  VT_TRANS(VT_STATE_DCS_PRM, 0x40, 0x7E, VT_ACT_HOOK, VT_STATE_DCS_PASS)

  VT_TRANS(VT_STATE_DCS_INT, 0x00, 0x17, VT_ACT_IGN, VT_STATE_DCS_INT)
  VT_TRANS(VT_STATE_DCS_INT, 0x19, 0x19, VT_ACT_IGN, VT_STATE_DCS_INT)
  VT_TRANS(VT_STATE_DCS_INT, 0x1C, 0x1F, VT_ACT_IGN, VT_STATE_DCS_INT)
  VT_TRANS(VT_STATE_DCS_INT, 0x20, 0x2F, VT_ACT_COLL, VT_STATE_DCS_INT)
  VT_TRANS(VT_STATE_DCS_INT, 0x7F, 0x7F, VT_ACT_IGN, VT_STATE_DCS_INT)
  VT_TRANS(VT_STATE_DCS_INT, 0x80, 0xFF, VT_ACT_PRN, VT_STATE_GND)
  VT_TRANS(VT_STATE_DCS_INT, 0x30, 0x3F, VT_ACT_NONE, VT_STATE_DCS_IGN)
  VT_TRANS(VT_STATE_DCS_INT, 0x40, 0x7E, VT_ACT_HOOK, VT_STATE_DCS_PASS)

  VT_TRANS(VT_STATE_DCS_PASS, 0x00, 0x17, VT_ACT_PUT, VT_STATE_DCS_PASS)
  VT_TRANS(VT_STATE_DCS_PASS, 0x19, 0x19, VT_ACT_PUT, VT_STATE_DCS_PASS)
  VT_TRANS(VT_STATE_DCS_PASS, 0x1C, 0x1F, VT_ACT_PUT, VT_STATE_DCS_PASS)
  VT_TRANS(VT_STATE_DCS_PASS, 0x20, 0x7E, VT_ACT_PUT, VT_STATE_DCS_PASS)
  VT_TRANS(VT_STATE_DCS_PASS, 0x7F, 0x7F, VT_ACT_IGN, VT_STATE_DCS_PASS)
  VT_TRANS(VT_STATE_DCS_PASS, 0x80, 0xFF, VT_ACT_PUT, VT_STATE_DCS_PASS)

  VT_TRANS(VT_STATE_DCS_IGN, 0x00, 0x17, VT_ACT_IGN, VT_STATE_DCS_IGN)
  VT_TRANS(VT_STATE_DCS_IGN, 0x19, 0x19, VT_ACT_IGN, VT_STATE_DCS_IGN)
  VT_TRANS(VT_STATE_DCS_IGN, 0x1C, 0x1F, VT_ACT_IGN, VT_STATE_DCS_IGN)
  VT_TRANS(VT_STATE_DCS_IGN, 0x20, 0x7F, VT_ACT_IGN, VT_STATE_DCS_IGN)
  VT_TRANS(VT_STATE_DCS_IGN, 0x80, 0xFF, VT_ACT_IGN, VT_STATE_DCS_IGN)

  VT_TRANS(VT_STATE_OSC_STR, 0x00, 0x17, VT_ACT_IGN, VT_STATE_OSC_STR)
  VT_TRANS(VT_STATE_OSC_STR, 0x19, 0x19, VT_ACT_IGN, VT_STATE_OSC_STR)
  VT_TRANS(VT_STATE_OSC_STR, 0x1C, 0x1F, VT_ACT_IGN, VT_STATE_OSC_STR)
  VT_TRANS(VT_STATE_OSC_STR, 0x20, 0xFF, VT_ACT_STR_PUT, VT_STATE_OSC_STR)
  VT_TRANS(VT_STATE_OSC_STR, 0x07, 0x07, VT_ACT_NONE, VT_STATE_GND)

  VT_TRANS(VT_STATE_SOS_STR, 0x00, 0x17, VT_ACT_IGN, VT_STATE_SOS_STR)
  VT_TRANS(VT_STATE_SOS_STR, 0x19, 0x19, VT_ACT_IGN, VT_STATE_SOS_STR)
  VT_TRANS(VT_STATE_SOS_STR, 0x1C, 0x1F, VT_ACT_IGN, VT_STATE_SOS_STR)
  VT_TRANS(VT_STATE_SOS_STR, 0x20, 0xFF, VT_ACT_STR_PUT, VT_STATE_SOS_STR)

  VT_TRANS(VT_STATE_PM_STR, 0x00, 0x17, VT_ACT_IGN, VT_STATE_PM_STR)
  VT_TRANS(VT_STATE_PM_STR, 0x19, 0x19, VT_ACT_IGN, VT_STATE_PM_STR)
  VT_TRANS(VT_STATE_PM_STR, 0x1C, 0x1F, VT_ACT_IGN, VT_STATE_PM_STR)
  VT_TRANS(VT_STATE_PM_STR, 0x20, 0xFF, VT_ACT_STR_PUT, VT_STATE_PM_STR)

  VT_TRANS(VT_STATE_APC_STR, 0x00, 0x17, VT_ACT_IGN, VT_STATE_APC_STR)
  VT_TRANS(VT_STATE_APC_STR, 0x19, 0x19, VT_ACT_IGN, VT_STATE_APC_STR)
  VT_TRANS(VT_STATE_APC_STR, 0x1C, 0x1F, VT_ACT_IGN, VT_STATE_APC_STR)
  VT_TRANS(VT_STATE_APC_STR, 0x20, 0xFF, VT_ACT_STR_PUT, VT_STATE_APC_STR)

  vt_table_initialized = true;
}

#undef VT_TRANS
#undef VT_ANY

void vt_init(vt_parser_t *parser, const vt_callbacks_t *cb, void *user_data) {
  vt_init_table();
  parser->state = VT_STATE_GND;
  parser->intermediates_len = 0;
  parser->params_len = 0;
  parser->ignore = false;
  parser->cb = cb;
  parser->user_data = user_data;
  for (int i = 0; i < 64; i++) {
    parser->params[i].value = -1;
    parser->params[i].sub = false;
  }
}

void vt_reset(vt_parser_t *parser) {
  parser->state = VT_STATE_GND;
  parser->intermediates_len = 0;
  parser->params_len = 0;
  parser->ignore = false;
}

bool vt_idle(const vt_parser_t *parser) {
  return parser->state == VT_STATE_GND;
}

static inline void vt_exit_action(vt_parser_t *parser, vt_state_t state) {
  switch (state) {
  case VT_STATE_OSC_STR:
    if (parser->cb && parser->cb->osc_end)
      parser->cb->osc_end(parser);
    break;
  case VT_STATE_SOS_STR:
    if (parser->cb && parser->cb->sos_end)
      parser->cb->sos_end(parser);
    break;
  case VT_STATE_PM_STR:
    if (parser->cb && parser->cb->pm_end)
      parser->cb->pm_end(parser);
    break;
  case VT_STATE_APC_STR:
    if (parser->cb && parser->cb->apc_end)
      parser->cb->apc_end(parser);
    break;
  case VT_STATE_DCS_PASS:
    if (parser->cb && parser->cb->unhook)
      parser->cb->unhook(parser);
    break;
  default:
    break;
  }
}

static inline void vt_enter_action(vt_parser_t *parser, vt_state_t state) {
  switch (state) {
  case VT_STATE_OSC_STR:
    if (parser->cb && parser->cb->osc_start)
      parser->cb->osc_start(parser);
    break;
  case VT_STATE_SOS_STR:
    if (parser->cb && parser->cb->sos_start)
      parser->cb->sos_start(parser);
    break;
  case VT_STATE_PM_STR:
    if (parser->cb && parser->cb->pm_start)
      parser->cb->pm_start(parser);
    break;
  case VT_STATE_APC_STR:
    if (parser->cb && parser->cb->apc_start)
      parser->cb->apc_start(parser);
    break;
  default:
    break;
  }
}

#if defined(__GNUC__) || defined(__clang__)
#define VT_COMPUTED_GOTO 1
#endif

void vt_parse(vt_parser_t *parser, const uint8_t *bytes, size_t len) {
  if (len == 0)
    return;

  const uint8_t *p = bytes;
  const uint8_t *end = bytes + len;
  uint8_t ch;
  uint8_t transition;
  vt_act_t action;
  vt_state_t next_state;

#ifdef VT_COMPUTED_GOTO
  static const void *dispatch_table[16] = {
      [VT_ACT_NONE] = &&do_none,
      [VT_ACT_IGN] = &&do_ign,
      [VT_ACT_PRN] = &&do_prn,
      [VT_ACT_EXEC] = &&do_exec,
      [VT_ACT_CLR] = &&do_clr,
      [VT_ACT_COLL] = &&do_coll,
      [VT_ACT_PRM] = &&do_prm,
      [VT_ACT_ESC_DISP] = &&do_esc_disp,
      [VT_ACT_CSI_DISP] = &&do_csi_disp,
      [VT_ACT_HOOK] = &&do_hook,
      [VT_ACT_PUT] = &&do_put,
      [VT_ACT_UNHK] = &&do_unhk,
      [VT_ACT_STR_PUT] = &&do_str_put,
      [13] = &&do_none, /* Safety padding */
      [14] = &&do_none, /* Safety padding */
      [15] = &&do_none  /* Safety padding */
  };
#define DISPATCH() goto *dispatch_table[action]
#else
#define DISPATCH()                                                             \
  switch (action) {                                                            \
  case VT_ACT_NONE:                                                            \
    goto do_none;                                                              \
  case VT_ACT_IGN:                                                             \
    goto do_ign;                                                               \
  case VT_ACT_PRN:                                                             \
    goto do_prn;                                                               \
  case VT_ACT_EXEC:                                                            \
    goto do_exec;                                                              \
  case VT_ACT_CLR:                                                             \
    goto do_clr;                                                               \
  case VT_ACT_COLL:                                                            \
    goto do_coll;                                                              \
  case VT_ACT_PRM:                                                             \
    goto do_prm;                                                               \
  case VT_ACT_ESC_DISP:                                                        \
    goto do_esc_disp;                                                          \
  case VT_ACT_CSI_DISP:                                                        \
    goto do_csi_disp;                                                          \
  case VT_ACT_HOOK:                                                            \
    goto do_hook;                                                              \
  case VT_ACT_PUT:                                                             \
    goto do_put;                                                               \
  case VT_ACT_UNHK:                                                            \
    goto do_unhk;                                                              \
  case VT_ACT_STR_PUT:                                                         \
    goto do_str_put;                                                           \
  default:                                                                     \
    goto do_none;                                                              \
  }
#endif

next_char:
  if (p >= end)
    return;
  ch = *p++;
  transition = vt_table[parser->state][ch];
  action = (vt_act_t)(transition >> 4);
  next_state = (vt_state_t)(transition & 0x0F);

  if (parser->state != next_state) {
    vt_exit_action(parser, parser->state);
  }

  DISPATCH();

do_prn:
  if (parser->cb && parser->cb->print)
    parser->cb->print(parser, ch);
  goto state_transition;

do_exec:
  if (parser->cb && parser->cb->execute)
    parser->cb->execute(parser, ch);
  goto state_transition;

do_clr:
  parser->intermediates_len = 0;
  parser->params_len = 0;
  parser->ignore = false;
  if (parser->cb && parser->cb->clear)
    parser->cb->clear(parser);
  goto state_transition;

do_coll:
  if (parser->intermediates_len < 8) {
    parser->intermediates[parser->intermediates_len++] = ch;
  } else {
    parser->ignore = true;
  }
  if (parser->cb && parser->cb->collect)
    parser->cb->collect(parser, ch);
  goto state_transition;

do_prm:
  if (parser->ignore) {
    if (parser->cb && parser->cb->param)
      parser->cb->param(parser, ch);
  } else {
    if (ch == ';') {
      if (parser->params_len == 0) {
        parser->params[0] = (vt_param_t){-1, false};
        parser->params[1] = (vt_param_t){-1, false};
        parser->params_len = 2;
      } else if (parser->params_len < 64) {
        parser->params[parser->params_len++] = (vt_param_t){-1, false};
      } else {
        parser->ignore = true;
      }
    } else if (ch == ':') {
      if (parser->params_len == 0) {
        parser->params[0] = (vt_param_t){-1, false};
        parser->params[1] = (vt_param_t){-1, true};
        parser->params_len = 2;
      } else if (parser->params_len < 64) {
        parser->params[parser->params_len++] = (vt_param_t){-1, true};
      } else {
        parser->ignore = true;
      }
    } else {
      if (parser->params_len == 0) {
        parser->params_len = 1;
        parser->params[0] = (vt_param_t){-1, false};
      }
      if (parser->params_len <= 64) {
        int32_t val = parser->params[parser->params_len - 1].value;
        bool sub = parser->params[parser->params_len - 1].sub;
        if (val == -1)
          val = 0;
        if (val <= (2147483647 - 9) / 10) {
          parser->params[parser->params_len - 1].value = val * 10 + (ch - '0');
          parser->params[parser->params_len - 1].sub = sub;
        } else {
          parser->params[parser->params_len - 1].value = 2147483647;
          parser->params[parser->params_len - 1].sub = sub;
        }
      }
    }
    if (parser->cb && parser->cb->param)
      parser->cb->param(parser, ch);
  }
  goto state_transition;

do_esc_disp:
  if (parser->cb && parser->cb->esc_dispatch)
    parser->cb->esc_dispatch(parser, ch, parser->intermediates,
                             parser->intermediates_len, parser->ignore);
  goto state_transition;

do_csi_disp:
  if (parser->cb && parser->cb->csi_dispatch)
    parser->cb->csi_dispatch(parser, ch, parser->params, parser->params_len,
                             parser->intermediates, parser->intermediates_len,
                             parser->ignore);
  goto state_transition;

do_hook:
  if (parser->cb && parser->cb->hook)
    parser->cb->hook(parser, ch, parser->params, parser->params_len,
                     parser->intermediates, parser->intermediates_len,
                     parser->ignore);
  goto state_transition;

do_put:
  if (parser->cb && parser->cb->put)
    parser->cb->put(parser, ch);
  goto state_transition;

do_str_put:
  switch (parser->state) {
  case VT_STATE_OSC_STR:
    if (parser->cb && parser->cb->osc_put)
      parser->cb->osc_put(parser, ch);
    break;
  case VT_STATE_SOS_STR:
    if (parser->cb && parser->cb->sos_put)
      parser->cb->sos_put(parser, ch);
    break;
  case VT_STATE_PM_STR:
    if (parser->cb && parser->cb->pm_put)
      parser->cb->pm_put(parser, ch);
    break;
  case VT_STATE_APC_STR:
    if (parser->cb && parser->cb->apc_put)
      parser->cb->apc_put(parser, ch);
    break;
  default:
    break;
  }
  goto state_transition;

do_none:
do_ign:
do_unhk:
  goto state_transition;

state_transition:
  if (parser->state != next_state) {
    vt_enter_action(parser, next_state);
    parser->state = next_state;
  }
  goto next_char;
}

void vt_utf8_init(vt_utf8_t *utf8) {
  utf8->len = 0;
  utf8->expected = 0;
}

bool vt_utf8_decode(vt_utf8_t *utf8, uint8_t byte, uint32_t *out_cp) {
  if (utf8->expected == 0) {
    if (byte <= 0x7F) {
      *out_cp = byte;
      return true;
    } else if (byte >= 0xC2 && byte <= 0xDF) {
      utf8->expected = 2;
    } else if (byte >= 0xE0 && byte <= 0xEF) {
      utf8->expected = 3;
    } else if (byte >= 0xF0 && byte <= 0xF4) {
      utf8->expected = 4;
    } else {
      *out_cp = 0xFFFD;
      return true;
    }
    utf8->buf[0] = byte;
    utf8->len = 1;
    return false;
  }

  utf8->buf[utf8->len++] = byte;

  if (utf8->len == utf8->expected) {
    uint32_t cp = 0;
    switch (utf8->expected) {
    case 2:
      cp = ((utf8->buf[0] & 0x1F) << 6) | (utf8->buf[1] & 0x3F);
      break;
    case 3:
      cp = ((utf8->buf[0] & 0x0F) << 12) | ((utf8->buf[1] & 0x3F) << 6) |
           (utf8->buf[2] & 0x3F);
      break;
    case 4:
      cp = ((utf8->buf[0] & 0x07) << 18) | ((utf8->buf[1] & 0x3F) << 12) |
           ((utf8->buf[2] & 0x3F) << 6) | (utf8->buf[3] & 0x3F);
      break;
    }
    utf8->expected = 0;
    utf8->len = 0;
    *out_cp = cp;
    return true;
  }

  return false;
}