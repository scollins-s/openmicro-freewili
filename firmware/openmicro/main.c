/* openmicro — FreeWili touch UI for OpenMicro agent control.
 * Power-aware: idle backlight-off, LED updates only on change, slower idle poll. */
#include "fw2.h"
#include "platform/diag.h"
#include "pico/stdlib.h"
#include "om_ui.h"
#include "om_link.h"
#include "om_leds.h"

/* Idle → standby after this much inactivity (backlight off). */
#define OM_IDLE_MS          30000u
/* Poll cadence while the UI is awake and recently used. */
#define OM_POLL_ACTIVE_MS   16u
/* Poll cadence after brief quiet (still awake, backlight on). */
#define OM_POLL_QUIET_MS    40u
/* Poll cadence in standby (backlight off). */
#define OM_POLL_SLEEP_MS    80u
/* Quiet threshold before we stretch the active poll interval. */
#define OM_QUIET_MS         2000u

static bool hit_wants_haptic(om_hit_t hit) {
    switch (hit) {
        case OM_HIT_ACCEPT:
        case OM_HIT_REJECT:
        case OM_HIT_VOICE:
        case OM_HIT_NEW:
        case OM_HIT_MODEL:
        case OM_HIT_RESUME:
        case OM_HIT_WF_REVIEW:
        case OM_HIT_WF_DEBUG:
        case OM_HIT_WF_REFACTOR:
        case OM_HIT_WF_TESTS:
            return true;
        default:
            return false; /* d-pad / thinking / layer / sessions: no buzz */
    }
}

static void handle_hit(om_hit_t hit, om_ui_state_t *st) {
    switch (hit) {
        case OM_HIT_ACCEPT:
            om_link_send_action("accept");
            break;
        case OM_HIT_REJECT:
            om_link_send_action("reject");
            break;
        case OM_HIT_VOICE:
            om_link_send_action("push_to_talk");
            break;
        case OM_HIT_MODEL:
            /* Claude: /model picker. Host maps action name → keys. */
            om_link_send_action("model");
            break;
        case OM_HIT_NEW:
            om_link_send_action("new_chat");
            break;
        case OM_HIT_RESUME:
            om_link_send_action("resume");
            break;
        case OM_HIT_WF_REVIEW:
            om_link_send_workflow("review-pr");
            break;
        case OM_HIT_WF_DEBUG:
            om_link_send_workflow("debug");
            break;
        case OM_HIT_WF_REFACTOR:
            om_link_send_workflow("refactor");
            break;
        case OM_HIT_WF_TESTS:
            om_link_send_workflow("write-tests");
            break;
        case OM_HIT_SESSION0:
        case OM_HIT_SESSION1:
        case OM_HIT_SESSION2:
        case OM_HIT_SESSION3:
        case OM_HIT_SESSION4: {
            int idx = (int)hit - (int)OM_HIT_SESSION0;
            st->focus_index = idx;
            st->session_mask |= (1 << idx);
            om_link_send_focus(idx);
            st->dirty = true;
            break;
        }
        case OM_HIT_THINK_MINUS:
            if (st->thinking > 0) st->thinking--;
            om_link_send_thinking(-1);
            st->dirty = true;
            break;
        case OM_HIT_THINK_PLUS:
            if (st->thinking < 4) st->thinking++;
            om_link_send_thinking(1);
            st->dirty = true;
            break;
        case OM_HIT_LAYER:
            st->layer = (st->layer + 1) % 6;
            om_link_send_layer(st->layer);
            st->dirty = true;
            break;
        case OM_HIT_DPAD_UP:
            om_link_send_keys("up");
            break;
        case OM_HIT_DPAD_DOWN:
            om_link_send_keys("down");
            break;
        case OM_HIT_DPAD_LEFT:
            om_link_send_keys("left");
            break;
        case OM_HIT_DPAD_RIGHT:
            om_link_send_keys("right");
            break;
        default:
            break;
    }
}

static void enter_standby(om_ui_state_t *ui) {
    DIAG("openmicro: standby (backlight off)\n");
    om_leds_blank();
    st7796_fill_screen(0x0000);
    board_backlight_set(0);
    ui->dirty = false;
}

static void leave_standby(om_ui_state_t *ui) {
    board_backlight_set(1);
    ui->dirty = true;
    DIAG("openmicro: wake\n");
}

/** Wait for current touch to release so the wake tap does not fire a control. */
static void wait_touch_release(void) {
    uint16_t x, y;
    absolute_time_t t0 = get_absolute_time();
    while (ft6336_poll(&x, &y)) {
        sleep_ms(20);
        if (absolute_time_diff_us(t0, get_absolute_time()) > 2000000) break;
    }
}

static bool poll_buttons_hit(om_hit_t *out) {
    buttons_state_t btns;
    buttons_poll(&btns);
    if (!btns.valid) return false;
    if (buttons_pressed(&btns, BTN_OK) || buttons_pressed(&btns, BTN_GREEN)) {
        *out = OM_HIT_ACCEPT;
        return true;
    }
    if (buttons_pressed(&btns, BTN_CANCEL) || buttons_pressed(&btns, BTN_RED)) {
        *out = OM_HIT_REJECT;
        return true;
    }
    if (buttons_pressed(&btns, BTN_YELLOW)) {
        *out = OM_HIT_VOICE;
        return true;
    }
    if (buttons_pressed(&btns, BTN_GREY)) {
        *out = OM_HIT_MODEL;
        return true;
    }
    if (buttons_pressed(&btns, BTN_BLUE) || buttons_pressed(&btns, BTN_HOME)) {
        *out = OM_HIT_NEW;
        return true;
    }
    if (buttons_pressed(&btns, BTN_UP)) {
        *out = OM_HIT_DPAD_UP;
        return true;
    }
    if (buttons_pressed(&btns, BTN_DOWN)) {
        *out = OM_HIT_DPAD_DOWN;
        return true;
    }
    if (buttons_pressed(&btns, BTN_LEFT)) {
        *out = OM_HIT_DPAD_LEFT;
        return true;
    }
    if (buttons_pressed(&btns, BTN_RIGHT)) {
        *out = OM_HIT_DPAD_RIGHT;
        return true;
    }
    return false;
}

int main(void) {
    board_init();
    st7796_init();
    board_backlight_set(1);
    ft6336_init();
    om_leds_init();
    buttons_init();
    haptic_init();

    om_ui_state_t ui;
    om_ui_init(&ui);

    bool linked = om_link_open();
    ui.link_up = linked;
    ui.dirty = true;

    DIAG("openmicro: up link=%d (idle standby %u ms)\n", linked ? 1 : 0, OM_IDLE_MS);

    absolute_time_t next_demo = make_timeout_time_ms(2500);
    absolute_time_t last_activity = get_absolute_time();
    bool asleep = false;
    bool touching = false;

    for (;;) {
        om_link_poll(&ui);

        /* Offline demo only when unlinked — avoid endless redraws while linked. */
        if (!ui.link_up && !asleep &&
            absolute_time_diff_us(get_absolute_time(), next_demo) <= 0) {
            ui.state = (om_agent_state_t)(((int)ui.state + 1) % 5);
            ui.dirty = true;
            next_demo = make_timeout_time_ms(2500);
        }

        bool activity = false;
        om_hit_t btn_hit = OM_HIT_NONE;
        if (poll_buttons_hit(&btn_hit)) {
            activity = true;
            if (asleep) {
                leave_standby(&ui);
                asleep = false;
                /* Wake-only: do not also fire the control on the wake press. */
            } else {
                if (hit_wants_haptic(btn_hit)) haptic_pulse_ms(25);
                handle_hit(btn_hit, &ui);
            }
        }

        uint16_t x, y;
        if (ft6336_poll(&x, &y)) {
            activity = true;
            if (asleep) {
                wait_touch_release();
                leave_standby(&ui);
                asleep = false;
                touching = false;
            } else if (!touching) {
                om_hit_t hit = om_ui_hit(x, y);
                if (hit != OM_HIT_NONE) {
                    if (hit_wants_haptic(hit)) haptic_pulse_ms(15);
                    handle_hit(hit, &ui);
                }
                touching = true;
            }
        } else {
            touching = false;
        }

        /* Host feedback / config may mark dirty while asleep — stay dark until wake. */
        if (activity) last_activity = get_absolute_time();

        if (!asleep) {
            int64_t idle_us = absolute_time_diff_us(last_activity, get_absolute_time());
            if (idle_us >= (int64_t)OM_IDLE_MS * 1000) {
                enter_standby(&ui);
                asleep = true;
            }
        }

        if (!asleep && ui.dirty) {
            om_ui_draw(&ui);
            om_leds_apply(&ui, (uint8_t)(ui.session_mask & 0x1f));
            ui.dirty = false;
        } else if (asleep) {
            ui.dirty = false; /* drop redraws until wake; leave_standby sets dirty */
        }

        uint32_t poll_ms = OM_POLL_ACTIVE_MS;
        if (asleep) {
            poll_ms = OM_POLL_SLEEP_MS;
        } else {
            int64_t quiet_us = absolute_time_diff_us(last_activity, get_absolute_time());
            if (quiet_us >= (int64_t)OM_QUIET_MS * 1000) poll_ms = OM_POLL_QUIET_MS;
        }
        sleep_ms(poll_ms);
    }
}
