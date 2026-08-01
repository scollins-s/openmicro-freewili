#ifndef VIEW_H
#define VIEW_H

#include "http_net.h"
#include "response_store.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    VIEW_HIT_NONE = 0,
    VIEW_HIT_PREV,
    VIEW_HIT_REFRESH,
    VIEW_HIT_CANCEL,
    VIEW_HIT_NEXT,
    VIEW_HIT_OFF,
} view_hit_t;

typedef struct view_state {
    bool dirty;
    bool asleep;
} view_state_t;

void view_init(view_state_t *v);
void view_draw(view_state_t *v, const http_net_t *net, const response_store_t *store);
view_hit_t view_hit(uint16_t x, uint16_t y);

void view_enter_standby(view_state_t *v);
void view_leave_standby(view_state_t *v);

#define VIEW_BODY_ROWS 26

#endif
