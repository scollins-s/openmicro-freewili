#ifndef OM_LINK_H
#define OM_LINK_H

#include <stdbool.h>
#include "om_ui.h"

/* FwGUI/OneWili bridge for OpenMicro. When open fails, UI stays in offline demo. */

bool om_link_open(void);
bool om_link_is_up(void);
void om_link_poll(om_ui_state_t *st);

void om_link_send_action(const char *action);
void om_link_send_workflow(const char *preset_id);
void om_link_send_focus(int index);
void om_link_send_thinking(int delta);
void om_link_send_layer(int index);
/** Raw TUI keys (arrow CSI etc). Wire: a\om\k <escaped-bytes-or-name> */
void om_link_send_keys(const char *name);

#endif
