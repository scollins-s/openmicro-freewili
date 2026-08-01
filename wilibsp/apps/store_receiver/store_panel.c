#include "store_panel.h"

#include "fw2.h"
#include <stdio.h>
#include <string.h>

/* 5x7 glyph cell at scale s is 6*s wide (5 + 1 gap) and 8*s tall. */
static int glyph_w(int scale) { return 6 * scale; }
static int glyph_h(int scale) { return 8 * scale; }

void panel_fill_rounded(panel_rect_t r, int radius, uint16_t color) {
    if (r.w <= 0 || r.h <= 0) return;
    if (radius < 0) radius = 0;
    if (radius * 2 > r.w) radius = r.w / 2;
    if (radius * 2 > r.h) radius = r.h / 2;

    /* Center body */
    st7796_fill_rect(r.x + radius, r.y, r.w - 2 * radius, r.h, color);
    st7796_fill_rect(r.x, r.y + radius, radius, r.h - 2 * radius, color);
    st7796_fill_rect(r.x + r.w - radius, r.y + radius, radius, r.h - 2 * radius, color);

    /* Stepped corner cutouts (approximate round) */
    for (int i = 0; i < radius; i++) {
        int inset = radius - 1 - i;
        if (inset < 0) inset = 0;
        /* top strip growing toward center */
        int tw = r.w - 2 * inset;
        if (tw > 0) {
            st7796_fill_rect(r.x + inset, r.y + i, tw, 1, color);
            st7796_fill_rect(r.x + inset, r.y + r.h - 1 - i, tw, 1, color);
        }
    }
}

void panel_draw_border(panel_rect_t r, int radius, int thickness, uint16_t color) {
    if (thickness < 1) thickness = 1;
    for (int t = 0; t < thickness; t++) {
        panel_rect_t inner = {r.x + t, r.y + t, r.w - 2 * t, r.h - 2 * t};
        int rad = radius - t;
        if (rad < 0) rad = 0;
        /* Draw as hollow: fill outer then punch? Simpler: four edges + corners via fill strips */
        if (inner.w <= 0 || inner.h <= 0) break;
        st7796_fill_rect(inner.x + rad, inner.y, inner.w - 2 * rad, 1, color);
        st7796_fill_rect(inner.x + rad, inner.y + inner.h - 1, inner.w - 2 * rad, 1, color);
        st7796_fill_rect(inner.x, inner.y + rad, 1, inner.h - 2 * rad, color);
        st7796_fill_rect(inner.x + inner.w - 1, inner.y + rad, 1, inner.h - 2 * rad, color);
        /* corner pixels */
        for (int i = 0; i < rad; i++) {
            int inset = rad - 1 - i;
            st7796_fill_rect(inner.x + inset, inner.y + i, 1, 1, color);
            st7796_fill_rect(inner.x + inner.w - 1 - inset, inner.y + i, 1, 1, color);
            st7796_fill_rect(inner.x + inset, inner.y + inner.h - 1 - i, 1, 1, color);
            st7796_fill_rect(inner.x + inner.w - 1 - inset, inner.y + inner.h - 1 - i, 1, 1, color);
        }
    }
}

void panel_draw_badge(int x, int y, const char *text, uint16_t bg, uint16_t fg) {
    if (!text || !text[0]) return;
    int scale = 1;
    int len = (int)strlen(text);
    int pad_x = 6;
    int pad_y = 3;
    int tw = len * glyph_w(scale);
    int th = glyph_h(scale);
    panel_rect_t br = {x, y, tw + pad_x * 2, th + pad_y * 2};
    panel_fill_rounded(br, 4, bg);
    st7796_draw_text(x + pad_x, y + pad_y, scale, fg, bg, text);
}

void panel_draw_placeholder(panel_rect_t r, uint16_t border, uint16_t fill) {
    st7796_fill_rect(r.x, r.y, r.w, r.h, fill);
    /* Dashed border (approx) */
    for (int x = r.x; x < r.x + r.w; x += 6) {
        int seg = 3;
        if (x + seg > r.x + r.w) seg = r.x + r.w - x;
        st7796_fill_rect(x, r.y, seg, 1, border);
        st7796_fill_rect(x, r.y + r.h - 1, seg, 1, border);
    }
    for (int y = r.y; y < r.y + r.h; y += 6) {
        int seg = 3;
        if (y + seg > r.y + r.h) seg = r.y + r.h - y;
        st7796_fill_rect(r.x, y, 1, seg, border);
        st7796_fill_rect(r.x + r.w - 1, y, 1, seg, border);
    }
}

void panel_draw_text_wrapped(int x, int y, int max_w, int scale, int max_lines,
                             uint16_t fg, uint16_t bg, const char *text) {
    if (!text || max_lines < 1 || max_w < glyph_w(scale)) return;
    int max_chars = max_w / glyph_w(scale);
    if (max_chars < 1) return;

    const char *p = text;
    for (int line = 0; line < max_lines && *p; line++) {
        char buf[96];
        int n = 0;
        /* Skip leading spaces */
        while (*p == ' ') p++;
        if (!*p) break;

        /* Take up to max_chars; prefer break at space */
        const char *start = p;
        int take = 0;
        int last_space = -1;
        while (start[take] && take < max_chars && take + 1 < (int)sizeof(buf)) {
            if (start[take] == ' ') last_space = take;
            take++;
        }
        if (start[take] && last_space > 0) take = last_space;
        if (take < 1) take = 1;
        memcpy(buf, start, (size_t)take);
        buf[take] = 0;
        st7796_draw_text(x, y + line * glyph_h(scale), scale, fg, bg, buf);
        p = start + take;
        while (*p == ' ') p++;
        /* Ellipsis on last line if more remains */
        if (line == max_lines - 1 && *p && take >= 3) {
            buf[take - 1] = '.';
            if (take >= 2) buf[take - 2] = '.';
            if (take >= 3) buf[take - 3] = '.';
            st7796_draw_text(x, y + line * glyph_h(scale), scale, fg, bg, buf);
        }
    }
}

void panel_draw_card(panel_rect_t r, const catalog_app_t *app, bool selected) {
    if (!app) return;
    uint16_t border = selected ? PANEL_COL_ACCENT : PANEL_COL_BORDER;
    int rad = 8;

    panel_fill_rounded(r, rad, PANEL_COL_CARD);
    panel_draw_border(r, rad, selected ? 2 : 1, border);

    int pad = 8;
    int cx = r.x + pad;
    int cy = r.y + pad;
    int inner_w = r.w - 2 * pad;

    /* Title (scale 1 so it fits small 2x2 tiles) */
    char title[CATALOG_NAME_MAX];
    snprintf(title, sizeof(title), "%s", app->name[0] ? app->name : app->id);
    int max_title = inner_w / glyph_w(1);
    if (max_title > 0 && (int)strlen(title) > max_title) {
        if (max_title > 3) {
            title[max_title - 3] = '.';
            title[max_title - 2] = '.';
            title[max_title - 1] = '.';
            title[max_title] = 0;
        } else {
            title[max_title] = 0;
        }
    }
    st7796_draw_text(cx, cy, 1, PANEL_COL_TEXT, PANEL_COL_CARD, title);
    cy += glyph_h(1) + 4;

    /* CPU badge */
    const char *badge = app->cpu_badge[0] ? app->cpu_badge
                        : (app->kind[0] ? app->kind : "app");
    panel_draw_badge(cx, cy, badge, PANEL_COL_BADGE, PANEL_COL_BADGE_FG);
    cy += glyph_h(1) + 8;

    /* Description (2 lines) */
    const char *desc = app->description[0] ? app->description : "No description.";
    int desc_lines = 2;
    int room = (r.y + r.h - pad - glyph_h(1) - 4) - cy;
    if (room < glyph_h(1) * 2) desc_lines = 1;
    panel_draw_text_wrapped(cx, cy, inner_w, 1, desc_lines, PANEL_COL_MUTED,
                            PANEL_COL_CARD, desc);

    /* Version footnote */
    char ver[40];
    if (app->version[0]) snprintf(ver, sizeof(ver), "V%s", app->version);
    else snprintf(ver, sizeof(ver), "V?");
    st7796_draw_text(cx, r.y + r.h - pad - glyph_h(1), 1, PANEL_COL_MUTED,
                     PANEL_COL_CARD, ver);

    if (app->replaces_stock) {
        st7796_draw_text(r.x + r.w - pad - 6 * 6, r.y + r.h - pad - glyph_h(1), 1,
                         0xF800, PANEL_COL_CARD, "FLASH");
    }
}
