// include/vt.h
#ifndef VT_H
#define VT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  VT_STATE_GND = 0,
  VT_STATE_ESC,
  VT_STATE_ESC_INT,
  VT_STATE_CSI_ENT,
  VT_STATE_CSI_PRM,
  VT_STATE_CSI_INT,
  VT_STATE_CSI_IGN,
  VT_STATE_DCS_ENT,
  VT_STATE_DCS_PRM,
  VT_STATE_DCS_INT,
  VT_STATE_DCS_PASS,
  VT_STATE_DCS_IGN,
  VT_STATE_OSC_STR,
  VT_STATE_SOS_STR,
  VT_STATE_PM_STR,
  VT_STATE_APC_STR
} vt_state_t;

typedef enum {
  VT_ACT_NONE = 0,
  VT_ACT_IGN,
  VT_ACT_PRN,
  VT_ACT_EXEC,
  VT_ACT_CLR,
  VT_ACT_COLL,
  VT_ACT_PRM,
  VT_ACT_ESC_DISP,
  VT_ACT_CSI_DISP,
  VT_ACT_HOOK,
  VT_ACT_PUT,
  VT_ACT_UNHK,
  VT_ACT_STR_PUT
} vt_act_t;

typedef enum {
  VT_CTRL_NUL = 0x00,
  VT_CTRL_BEL = 0x07,
  VT_CTRL_BS = 0x08,
  VT_CTRL_HT = 0x09,
  VT_CTRL_LF = 0x0A,
  VT_CTRL_VT = 0x0B,
  VT_CTRL_FF = 0x0C,
  VT_CTRL_CR = 0x0D,
  VT_CTRL_ESC = 0x1B
} vt_ctrl_t;

typedef struct {
  int32_t value;
  bool sub;
} vt_param_t;

typedef struct vt_parser vt_parser_t;

typedef struct {
  void (*print)(vt_parser_t *parser, uint8_t c);
  void (*execute)(vt_parser_t *parser, uint8_t c);
  void (*clear)(vt_parser_t *parser);
  void (*collect)(vt_parser_t *parser, uint8_t c);
  void (*param)(vt_parser_t *parser, uint8_t c);
  void (*esc_dispatch)(vt_parser_t *parser, uint8_t c,
                       const uint8_t *intermediates, int intermediates_len,
                       bool ignore);
  void (*csi_dispatch)(vt_parser_t *parser, uint8_t c, const vt_param_t *params,
                       int params_len, const uint8_t *intermediates,
                       int intermediates_len, bool ignore);
  void (*hook)(vt_parser_t *parser, uint8_t c, const vt_param_t *params,
               int params_len, const uint8_t *intermediates,
               int intermediates_len, bool ignore);
  void (*put)(vt_parser_t *parser, uint8_t c);
  void (*unhook)(vt_parser_t *parser);
  void (*osc_start)(vt_parser_t *parser);
  void (*osc_put)(vt_parser_t *parser, uint8_t c);
  void (*osc_end)(vt_parser_t *parser);
  void (*sos_start)(vt_parser_t *parser);
  void (*sos_put)(vt_parser_t *parser, uint8_t c);
  void (*sos_end)(vt_parser_t *parser);
  void (*pm_start)(vt_parser_t *parser);
  void (*pm_put)(vt_parser_t *parser, uint8_t c);
  void (*pm_end)(vt_parser_t *parser);
  void (*apc_start)(vt_parser_t *parser);
  void (*apc_put)(vt_parser_t *parser, uint8_t c);
  void (*apc_end)(vt_parser_t *parser);
} vt_callbacks_t;

struct vt_parser {
  vt_state_t state;
  uint8_t intermediates[8];
  int intermediates_len;
  vt_param_t params[64];
  int params_len;
  bool ignore;
  void *user_data;
  const vt_callbacks_t *cb;
};

typedef struct {
  uint8_t buf[4];
  uint8_t len;
  uint8_t expected;
} vt_utf8_t;

void vt_init(vt_parser_t *parser, const vt_callbacks_t *cb, void *user_data);
void vt_reset(vt_parser_t *parser);
bool vt_idle(const vt_parser_t *parser);
void vt_parse(vt_parser_t *parser, const uint8_t *bytes, size_t len);

void vt_utf8_init(vt_utf8_t *utf8);
bool vt_utf8_decode(vt_utf8_t *utf8, uint8_t byte, uint32_t *out_cp);

#endif
