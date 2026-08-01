#include "catalog_parse.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "store_uart_proto.h"

static const char *skip_ws(const char *p, const char *end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
    return p;
}

static bool match(const char *p, const char *end, const char *lit) {
    size_t n = strlen(lit);
    return (size_t)(end - p) >= n && memcmp(p, lit, n) == 0;
}

/* Extract JSON string value after key "key": — writes into out (NUL-term). */
static bool extract_string_after(const char *start, const char *end, const char *key,
                                 char *out, size_t out_sz) {
    char pat[80];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = start;
    size_t klen = strlen(pat);
    while (p + klen < end) {
        if (memcmp(p, pat, klen) == 0) {
            p += klen;
            p = skip_ws(p, end);
            if (p < end && *p == ':') {
                p++;
                p = skip_ws(p, end);
                if (p < end && *p == '"') {
                    p++;
                    size_t i = 0;
                    while (p < end && *p != '"' && i + 1 < out_sz) {
                        if (*p == '\\' && p + 1 < end) {
                            p++;
                            out[i++] = *p++;
                        } else {
                            out[i++] = *p++;
                        }
                    }
                    out[i] = 0;
                    return true;
                }
                if (p < end && match(p, end, "null")) {
                    out[0] = 0;
                    return true;
                }
            }
        }
        p++;
    }
    return false;
}

static bool extract_bool_after(const char *start, const char *end, const char *key,
                               bool *out) {
    char pat[80];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = start;
    size_t klen = strlen(pat);
    while (p + klen < end) {
        if (memcmp(p, pat, klen) == 0) {
            p += klen;
            p = skip_ws(p, end);
            if (p < end && *p == ':') {
                p++;
                p = skip_ws(p, end);
                if (match(p, end, "true")) {
                    *out = true;
                    return true;
                }
                if (match(p, end, "false")) {
                    *out = false;
                    return true;
                }
            }
        }
        p++;
    }
    return false;
}

static bool extract_u32_after(const char *start, const char *end, const char *key,
                              uint32_t *out) {
    char pat[80];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = start;
    size_t klen = strlen(pat);
    while (p + klen < end) {
        if (memcmp(p, pat, klen) == 0) {
            p += klen;
            p = skip_ws(p, end);
            if (p < end && *p == ':') {
                p++;
                p = skip_ws(p, end);
                if (p < end && match(p, end, "null")) return false;
                uint32_t v = 0;
                if (p >= end || !isdigit((unsigned char)*p)) return false;
                while (p < end && isdigit((unsigned char)*p)) {
                    v = v * 10u + (uint32_t)(*p - '0');
                    p++;
                }
                *out = v;
                return true;
            }
        }
        p++;
    }
    return false;
}

/* Find next object starting at `{` within apps array region. */
static const char *find_object_end(const char *p, const char *end) {
    if (p >= end || *p != '{') return NULL;
    int depth = 0;
    bool in_str = false;
    bool esc = false;
    for (const char *q = p; q < end; q++) {
        char c = *q;
        if (in_str) {
            if (esc) {
                esc = false;
            } else if (c == '\\') {
                esc = true;
            } else if (c == '"') {
                in_str = false;
            }
            continue;
        }
        if (c == '"') {
            in_str = true;
        } else if (c == '{') {
            depth++;
        } else if (c == '}') {
            depth--;
            if (depth == 0) return q + 1;
        }
    }
    return NULL;
}

uint8_t catalog_kind_code(const char *kind) {
    if (!kind) return STORE_KIND_OTHER;
    if (strcmp(kind, "script") == 0) return STORE_KIND_SCRIPT;
    if (strcmp(kind, "firmware") == 0) return STORE_KIND_FIRMWARE;
    if (strcmp(kind, "bundle") == 0) return STORE_KIND_BUNDLE;
    if (strcmp(kind, "content") == 0) return STORE_KIND_CONTENT;
    return STORE_KIND_OTHER;
}

uint8_t catalog_art_code(const char *type) {
    if (!type) return STORE_ART_OTHER;
    if (strcmp(type, "wasm") == 0) return STORE_ART_WASM;
    if (strcmp(type, "uf2") == 0) return STORE_ART_UF2;
    if (strcmp(type, "bin") == 0) return STORE_ART_BIN;
    if (strcmp(type, "zip") == 0) return STORE_ART_ZIP;
    return STORE_ART_OTHER;
}

const catalog_app_t *catalog_find(const catalog_t *c, const char *id) {
    if (!c || !id) return NULL;
    for (int i = 0; i < c->app_count; i++) {
        if (strcmp(c->apps[i].id, id) == 0) return &c->apps[i];
    }
    return NULL;
}

static void basename_from_path(const char *path, char *out, size_t out_sz) {
    const char *base = path;
    for (const char *q = path; *q; q++) {
        if (*q == '/' || *q == '\\') base = q + 1;
    }
    snprintf(out, out_sz, "%s", base);
}

static void infer_type_from_name(const char *name, char *type, size_t type_sz) {
    const char *dot = strrchr(name, '.');
    if (!dot || !dot[1]) {
        snprintf(type, type_sz, "other");
        return;
    }
    if (strcmp(dot, ".wasm") == 0) snprintf(type, type_sz, "wasm");
    else if (strcmp(dot, ".uf2") == 0) snprintf(type, type_sz, "uf2");
    else if (strcmp(dot, ".bin") == 0) snprintf(type, type_sz, "bin");
    else if (strcmp(dot, ".zip") == 0) snprintf(type, type_sz, "zip");
    else snprintf(type, type_sz, "other");
}

/* Build "display · main · esp32" from which target keys exist in the app object. */
static void fill_cpu_badge(const char *obj, const char *obj_end, char *out, size_t out_sz) {
    out[0] = 0;
    struct {
        const char *key;
        const char *label;
    } cpus[] = {
        {"displaycpu", "display"},
        {"maincpu", "main"},
        {"esp32", "esp32"},
    };
    size_t used = 0;
    for (size_t i = 0; i < sizeof(cpus) / sizeof(cpus[0]); i++) {
        char pat[32];
        snprintf(pat, sizeof(pat), "\"%s\"", cpus[i].key);
        const char *hit = strstr(obj, pat);
        if (!hit || hit >= obj_end) continue;
        if (used > 0 && used + 3 < out_sz) {
            memcpy(out + used, " · ", 3);
            used += 3;
        }
        size_t n = strlen(cpus[i].label);
        if (used + n >= out_sz) break;
        memcpy(out + used, cpus[i].label, n);
        used += n;
        out[used] = 0;
    }
}

/* Prefer web schema targets.{displaycpu,maincpu,esp32}.{file,sha256,size}. */
static bool fill_artifact_from_cpu_target(const char *obj, const char *obj_end,
                                          catalog_app_t *app) {
    static const char *keys[] = {"displaycpu", "maincpu", "esp32", NULL};
    for (int i = 0; keys[i]; i++) {
        char pat[48];
        snprintf(pat, sizeof(pat), "\"%s\"", keys[i]);
        const char *k = obj;
        while (k < obj_end) {
            k = strstr(k, pat);
            if (!k || k >= obj_end) break;
            const char *brace = strchr(k, '{');
            if (!brace || brace >= obj_end) break;
            const char *tend = find_object_end(brace, obj_end);
            if (!tend) break;
            extract_string_after(brace, tend, "file", app->art.url, sizeof(app->art.url));
            if (!app->art.url[0]) {
                extract_string_after(brace, tend, "url", app->art.url, sizeof(app->art.url));
            }
            if (!app->art.url[0]) {
                k = tend;
                continue;
            }
            extract_string_after(brace, tend, "sha256", app->art.sha256_hex,
                                 sizeof(app->art.sha256_hex));
            app->art.has_sha = app->art.sha256_hex[0] != 0;
            uint32_t sz = 0;
            if (extract_u32_after(brace, tend, "size", &sz)) {
                app->art.size = sz;
                app->art.has_size = true;
            }
            basename_from_path(app->art.url, app->art.filename, sizeof(app->art.filename));
            infer_type_from_name(app->art.filename, app->art.type, sizeof(app->art.type));
            app->has_artifact = true;
            return true;
        }
    }
    return false;
}

static void infer_kind(catalog_app_t *app, const char *obj, const char *obj_end) {
    if (app->kind[0]) return;
    /* Scan tags array text for script / content hints. */
    const char *tags = strstr(obj, "\"tags\"");
    if (tags && tags < obj_end) {
        const char *bracket = strchr(tags, '[');
        const char *close = bracket ? strchr(bracket, ']') : NULL;
        if (bracket && close && close < obj_end) {
            if (strstr(bracket, "\"script\"") && strstr(bracket, "\"script\"") < close) {
                snprintf(app->kind, sizeof(app->kind), "script");
                return;
            }
            if (strstr(bracket, "\"usb-content\"") && strstr(bracket, "\"usb-content\"") < close) {
                snprintf(app->kind, sizeof(app->kind), "content");
                return;
            }
        }
    }
    if (strcmp(app->art.type, "wasm") == 0) {
        snprintf(app->kind, sizeof(app->kind), "script");
    } else if (strcmp(app->art.type, "txt") == 0 || strcmp(app->art.type, "other") == 0) {
        snprintf(app->kind, sizeof(app->kind), "content");
    } else {
        snprintf(app->kind, sizeof(app->kind), "firmware");
        app->replaces_stock = true;
    }
}

bool catalog_parse(const char *json, size_t len, catalog_t *out) {
    if (!json || !out || len == 0) return false;
    memset(out, 0, sizeof(*out));
    const char *end = json + len;

    uint32_t ver = 0;
    if (extract_u32_after(json, end, "catalog_version", &ver) ||
        extract_u32_after(json, end, "version", &ver)) {
        out->version = (int)ver;
    }

    const char *apps_key = strstr(json, "\"apps\"");
    if (!apps_key || apps_key >= end) return out->version > 0; /* empty ok */

    const char *arr = strchr(apps_key, '[');
    if (!arr || arr >= end) return false;
    arr++;

    const char *p = arr;
    while (p < end && out->app_count < CATALOG_MAX_APPS) {
        p = skip_ws(p, end);
        if (p >= end || *p == ']') break;
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p != '{') {
            p++;
            continue;
        }
        const char *obj_end = find_object_end(p, end);
        if (!obj_end) break;

        catalog_app_t *app = &out->apps[out->app_count];
        memset(app, 0, sizeof(*app));
        extract_string_after(p, obj_end, "id", app->id, sizeof(app->id));
        extract_string_after(p, obj_end, "name", app->name, sizeof(app->name));
        extract_string_after(p, obj_end, "version", app->version, sizeof(app->version));
        extract_string_after(p, obj_end, "description", app->description,
                             sizeof(app->description));
        extract_string_after(p, obj_end, "kind", app->kind, sizeof(app->kind));
        extract_bool_after(p, obj_end, "replaces_stock_firmware", &app->replaces_stock);
        fill_cpu_badge(p, obj_end, app->cpu_badge, sizeof(app->cpu_badge));

        /* Schema A: artifacts[] (docs / FWSA-oriented). */
        const char *arts = strstr(p, "\"artifacts\"");
        if (arts && arts < obj_end) {
            const char *a0 = strchr(arts, '{');
            if (a0 && a0 < obj_end) {
                const char *aend = find_object_end(a0, obj_end);
                if (aend) {
                    extract_string_after(a0, aend, "type", app->art.type, sizeof(app->art.type));
                    extract_string_after(a0, aend, "url", app->art.url, sizeof(app->art.url));
                    extract_string_after(a0, aend, "filename", app->art.filename,
                                         sizeof(app->art.filename));
                    extract_string_after(a0, aend, "sha256", app->art.sha256_hex,
                                         sizeof(app->art.sha256_hex));
                    app->art.has_sha = app->art.sha256_hex[0] != 0;
                    uint32_t sz = 0;
                    if (extract_u32_after(a0, aend, "size", &sz)) {
                        app->art.size = sz;
                        app->art.has_size = true;
                    }
                    app->has_artifact = app->art.url[0] != 0;
                }
            }
        }

        /* Schema B: web targets.{displaycpu|maincpu|esp32}. */
        if (!app->has_artifact) {
            fill_artifact_from_cpu_target(p, obj_end, app);
        }

        infer_kind(app, p, obj_end);

        if (app->id[0] && strcmp(app->kind, "project") != 0) {
            if (!app->name[0]) snprintf(app->name, sizeof(app->name), "%s", app->id);
            out->app_count++;
        }
        p = obj_end;
    }
    return out->app_count >= 0;
}
