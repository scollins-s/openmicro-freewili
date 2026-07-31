#ifndef OM_CMD_BRIDGE_H
#define OM_CMD_BRIDGE_H

/*
 * Main-CPU OpenMicro bridge (a\om\*).
 *
 * Parse quiet-path lines from display FwGUI and emit line-JSON toward the PC
 * USB CDC. Feed PC JSON lines back to convert into spontaneous display events:
 *   [*omFb …] [*omCfg …] [*omHello …]
 *
 * Does NOT open UART/USB itself — integrator supplies callbacks.
 * See docs/openmicro-freewili/ONEWILI-OPENMICRO-COMMANDS.md and INTEGRATION.md.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OM_CMD_CDC_CHUNK
#define OM_CMD_CDC_CHUNK 240u
#endif

typedef void (*om_cmd_emit_fn)(const char *event_id, const char *args, void *user);
typedef void (*om_cmd_cdc_write_fn)(const char *line, void *user);

typedef struct om_cmd_bridge {
    om_cmd_emit_fn emit;
    om_cmd_cdc_write_fn cdc_write;
    void *user;
    uint32_t actions_forwarded;
    uint32_t feedback_forwarded;
} om_cmd_bridge_t;

void om_cmd_bridge_init(om_cmd_bridge_t *b,
                        om_cmd_emit_fn emit,
                        om_cmd_cdc_write_fn cdc_write,
                        void *user);

/*
 * Handle one quiet-path line from display (optional leading 0x02).
 * Recognizes a\om\* and writes one JSON line to CDC.
 * Returns true if consumed.
 */
bool om_cmd_bridge_handle_display_line(om_cmd_bridge_t *b, const char *line);

/*
 * Handle one line from PC CDC (JSON). Converts feedback/config/hello into
 * [*omFb]/[*omCfg]/[*omHello] toward display. Returns true if consumed.
 */
bool om_cmd_bridge_handle_cdc_line(om_cmd_bridge_t *b, const char *line);

#ifdef __cplusplus
}
#endif

#endif
