#include "om_link.h"
#include "onewili.h"
#include "onewili_fwgui.h"
#include "onewili_events.h"
#include "platform/diag.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static ow_device g_dev;
static bool g_up;

static ow_status send_wire(const char *cmd) {
    if (!g_up) {
        DIAG("om_link: DROP (no FwGUI): %s\n", cmd ? cmd : "?");
        return OW_ERR_IO;
    }
    uint8_t out[OW_CMD_MAX + 2];
    size_t clen = strlen(cmd);
    if (clen + 2 > sizeof out) return OW_ERR_ARG;
    out[0] = 0x02;
    memcpy(out + 1, cmd, clen);
    out[1 + clen] = '\n';
    if (g_dev.t.write(g_dev.t.ctx, out, clen + 2) < 0) {
        DIAG("om_link: WRITE FAIL: %s\n", cmd);
        return OW_ERR_IO;
    }
    DIAG("OMTX %s\n", cmd);
    return OW_OK;
}

bool om_link_open(void) {
    if (ow_open_fwgui(&g_dev) != OW_OK) {
        g_up = false;
        DIAG("om_link: FwGUI open failed — offline demo (taps stay local)\n");
        return false;
    }
    g_up = true;
    DIAG("om_link: FwGUI up (wires go to main; stock main may not relay to PC)\n");
    /* Best-effort hello nudge for future main relay */
    (void)send_wire("a\\om\\a ping");
    return true;
}

bool om_link_is_up(void) { return g_up; }

void om_link_send_action(const char *action) {
    char cmd[96];
    snprintf(cmd, sizeof cmd, "a\\om\\a %s", action ? action : "accept");
    (void)send_wire(cmd);
}

void om_link_send_workflow(const char *preset_id) {
    char cmd[96];
    snprintf(cmd, sizeof cmd, "a\\om\\w %s", preset_id ? preset_id : "review-pr");
    (void)send_wire(cmd);
}

void om_link_send_keys(const char *name) {
    char cmd[64];
    snprintf(cmd, sizeof cmd, "a\\om\\k %s", name ? name : "up");
    (void)send_wire(cmd);
}

void om_link_send_focus(int index) {
    char cmd[48];
    snprintf(cmd, sizeof cmd, "a\\om\\f %d", index);
    (void)send_wire(cmd);
}

void om_link_send_thinking(int delta) {
    char cmd[48];
    snprintf(cmd, sizeof cmd, "a\\om\\t %d", delta);
    (void)send_wire(cmd);
}

void om_link_send_layer(int index) {
    char cmd[48];
    snprintf(cmd, sizeof cmd, "a\\om\\l %d", index);
    (void)send_wire(cmd);
}

static om_agent_state_t parse_state(const char *s) {
    if (!s) return OM_STATE_IDLE;
    if (strstr(s, "executing")) return OM_STATE_EXECUTING;
    if (strstr(s, "waiting")) return OM_STATE_WAITING;
    if (strstr(s, "complete")) return OM_STATE_COMPLETE;
    if (strstr(s, "error")) return OM_STATE_ERROR;
    return OM_STATE_IDLE;
}

void om_link_poll(om_ui_state_t *st) {
    if (!g_up || !st) return;
    ow_event ev;
    for (;;) {
        int r = ow_poll_text_event(&g_dev, &ev);
        if (r <= 0) break;
        if (ev.kind != OW_EV_TEXT) continue;
        const char *id = ev.u.text.id;
        const char *args = ev.u.text.args;
        if (strcmp(id, "omHello") == 0) {
            st->link_up = true;
            st->dirty = true;
            DIAG("om_link: omHello %s\n", args);
        } else if (strcmp(id, "omFb") == 0) {
            /* Lightweight scrape — full JSON parser deferred */
            if (strstr(args, "\"executing\"")) st->state = OM_STATE_EXECUTING;
            else if (strstr(args, "\"waiting\"")) st->state = OM_STATE_WAITING;
            else if (strstr(args, "\"complete\"")) st->state = OM_STATE_COMPLETE;
            else if (strstr(args, "\"error\"")) st->state = OM_STATE_ERROR;
            else if (strstr(args, "\"idle\"")) st->state = OM_STATE_IDLE;
            const char *fi = strstr(args, "\"focusIndex\"");
            if (fi) {
                const char *colon = strchr(fi, ':');
                if (colon) st->focus_index = atoi(colon + 1);
            }
            const char *pl = strstr(args, "\"playerLeds\"");
            if (pl) {
                const char *colon = strchr(pl, ':');
                if (colon) st->session_mask = atoi(colon + 1);
            }
            const char *ly = strstr(args, "\"layer\"");
            if (ly) {
                const char *colon = strchr(ly, ':');
                if (colon) st->layer = atoi(colon + 1);
            }
            st->link_up = true;
            st->dirty = true;
            (void)parse_state;
        } else if (strcmp(id, "omCfg") == 0) {
            st->link_up = true;
            st->dirty = true;
        }
    }
}
