#ifndef OM_UI_H
#define OM_UI_H

#include <stdint.h>
#include <stdbool.h>

/* Agent-ish states mirrored from OpenMicro feedback.ts */
typedef enum {
    OM_STATE_IDLE = 0,
    OM_STATE_EXECUTING,
    OM_STATE_WAITING,
    OM_STATE_COMPLETE,
    OM_STATE_ERROR,
} om_agent_state_t;

typedef struct {
    om_agent_state_t state;
    int focus_index;       /* 0..4 */
    int session_mask;      /* bits 0..4 occupied */
    int layer;             /* 0..5 */
    int thinking;          /* display level 0..4 */
    bool link_up;
    bool dirty;
} om_ui_state_t;

typedef enum {
    OM_HIT_NONE = 0,
    OM_HIT_ACCEPT,
    OM_HIT_REJECT,
    OM_HIT_VOICE,
    OM_HIT_NEW,
    OM_HIT_MODEL,
    OM_HIT_WF_REVIEW,
    OM_HIT_WF_DEBUG,
    OM_HIT_WF_REFACTOR,
    OM_HIT_WF_TESTS,
    OM_HIT_SESSION0,
    OM_HIT_SESSION1,
    OM_HIT_SESSION2,
    OM_HIT_SESSION3,
    OM_HIT_SESSION4,
    OM_HIT_THINK_MINUS,
    OM_HIT_THINK_PLUS,
    OM_HIT_LAYER,
    OM_HIT_DPAD_UP,
    OM_HIT_DPAD_DOWN,
    OM_HIT_DPAD_LEFT,
    OM_HIT_DPAD_RIGHT,
} om_hit_t;

void om_ui_init(om_ui_state_t *st);
void om_ui_draw(const om_ui_state_t *st);
om_hit_t om_ui_hit(uint16_t x, uint16_t y);

#endif
