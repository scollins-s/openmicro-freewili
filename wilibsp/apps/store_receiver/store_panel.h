#ifndef STORE_PANEL_H
#define STORE_PANEL_H

#include "catalog_parse.h"
#include <stdint.h>

/* RGB565 (big-endian as used by st7796_*) — approx web tokens.css */
#define PANEL_COL_PAGE   0xEF5D  /* #f3f4f6 light gray */
#define PANEL_COL_CARD   0xFFFF  /* white */
#define PANEL_COL_BORDER 0x8410  /* muted border */
#define PANEL_COL_TEXT   0x18C3  /* near #1a1d23 */
#define PANEL_COL_MUTED  0x8410  /* #8b919c-ish */
#define PANEL_COL_BADGE  0x2D7A  /* #2f6fad blue */
#define PANEL_COL_BADGE_FG 0xFFFF
#define PANEL_COL_ACCENT 0x1C4B  /* #1f8a5b green-ish */

typedef struct panel_rect {
    int x, y, w, h;
} panel_rect_t;

void panel_fill_rounded(panel_rect_t r, int radius, uint16_t color);
void panel_draw_border(panel_rect_t r, int radius, int thickness, uint16_t color);
void panel_draw_badge(int x, int y, const char *text, uint16_t bg, uint16_t fg);
void panel_draw_placeholder(panel_rect_t r, uint16_t border, uint16_t fill);
void panel_draw_text_wrapped(int x, int y, int max_w, int scale, int max_lines,
                             uint16_t fg, uint16_t bg, const char *text);

/* Compact app card: name + badge, 2-line desc, version (no media placeholder). */
void panel_draw_card(panel_rect_t r, const catalog_app_t *app, bool selected);

#endif
