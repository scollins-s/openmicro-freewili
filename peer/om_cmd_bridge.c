#include "om_cmd_bridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void emit(om_cmd_bridge_t *b, const char *id, const char *args) {
    if (b->emit) b->emit(id, args ? args : "", b->user);
}

static void cdc(om_cmd_bridge_t *b, const char *line) {
    if (b->cdc_write) b->cdc_write(line, b->user);
}

void om_cmd_bridge_init(om_cmd_bridge_t *b,
                        om_cmd_emit_fn emit_fn,
                        om_cmd_cdc_write_fn cdc_write,
                        void *user) {
    memset(b, 0, sizeof(*b));
    b->emit = emit_fn;
    b->cdc_write = cdc_write;
    b->user = user;
}

static bool json_escape_and_send_action_name(om_cmd_bridge_t *b, const char *name) {
    char line[192];
    snprintf(line, sizeof line,
             "{\"v\":1,\"type\":\"action\",\"action\":{\"type\":\"%s\"}}\n", name);
    cdc(b, line);
    b->actions_forwarded++;
    return true;
}

bool om_cmd_bridge_handle_display_line(om_cmd_bridge_t *b, const char *line) {
    if (!b || !line) return false;
    while (*line == ' ' || *line == '\t') line++;
    if ((unsigned char)line[0] == 0x02) line++;

    if (strncmp(line, "a\\om\\", 4) != 0) return false;
    const char *rest = line + 4;
    char op = rest[0];
    if (op == 0) return false;
    const char *args = rest + 1;
    while (*args == ' ') args++;

    char out[256];
    switch (op) {
        case 'a': {
            if (strcmp(args, "ping") == 0) {
                cdc(b, "{\"v\":1,\"type\":\"ping\"}\n");
                return true;
            }
            if (strcmp(args, "accept") == 0 || strcmp(args, "reject") == 0 ||
                strcmp(args, "new_chat") == 0 || strcmp(args, "push_to_talk") == 0 ||
                strcmp(args, "herdr_space") == 0 || strcmp(args, "open_model") == 0) {
                return json_escape_and_send_action_name(b, args);
            }
            if (strcmp(args, "model") == 0) {
                return json_escape_and_send_action_name(b, "open_model");
            }
            return true; /* consumed unknown a\om\a */
        }
        case 'w':
            snprintf(out, sizeof out,
                     "{\"v\":1,\"type\":\"action\",\"action\":{\"type\":\"workflow\","
                     "\"presetId\":\"%s\"}}\n",
                     args[0] ? args : "review-pr");
            cdc(b, out);
            b->actions_forwarded++;
            return true;
        case 'f':
            snprintf(out, sizeof out,
                     "{\"v\":1,\"type\":\"action\",\"action\":{\"type\":\"focus_session\","
                     "\"index\":%d}}\n",
                     atoi(args));
            cdc(b, out);
            b->actions_forwarded++;
            return true;
        case 't':
            snprintf(out, sizeof out,
                     "{\"v\":1,\"type\":\"action\",\"action\":{\"type\":\"thinking_depth\","
                     "\"delta\":%d}}\n",
                     atoi(args) < 0 ? -1 : 1);
            cdc(b, out);
            b->actions_forwarded++;
            return true;
        case 'l':
            snprintf(out, sizeof out,
                     "{\"v\":1,\"type\":\"action\",\"action\":{\"type\":\"layer\","
                     "\"index\":%d}}\n",
                     atoi(args));
            cdc(b, out);
            b->actions_forwarded++;
            return true;
        case 'k':
            snprintf(out, sizeof out,
                     "{\"v\":1,\"type\":\"action\",\"action\":{\"type\":\"keys\","
                     "\"bytes\":\"%s\"}}\n",
                     args[0] ? args : "up");
            cdc(b, out);
            b->actions_forwarded++;
            return true;
        default:
            return true;
    }
}

bool om_cmd_bridge_handle_cdc_line(om_cmd_bridge_t *b, const char *line) {
    if (!b || !line) return false;
    while (*line == ' ' || *line == '\t') line++;
    if (line[0] != '{') return false;

    /* Lightweight scrape — avoid a full JSON parser on main. */
    if (strstr(line, "\"type\":\"feedback\"") || strstr(line, "\"type\": \"feedback\"")) {
        emit(b, "omFb", line);
        b->feedback_forwarded++;
        return true;
    }
    if (strstr(line, "\"type\":\"config\"") || strstr(line, "\"type\": \"config\"")) {
        emit(b, "omCfg", line);
        return true;
    }
    if (strstr(line, "\"type\":\"hello\"") || strstr(line, "\"type\": \"hello\"")) {
        emit(b, "omHello", "host");
        return true;
    }
    if (strstr(line, "\"type\":\"ping\"") || strstr(line, "\"type\": \"ping\"")) {
        cdc(b, "{\"v\":1,\"type\":\"pong\"}\n");
        return true;
    }
    return false;
}
