#ifndef CATALOG_PARSE_H
#define CATALOG_PARSE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define CATALOG_MAX_APPS       32
#define CATALOG_ID_MAX         48
#define CATALOG_NAME_MAX       64
#define CATALOG_VERSION_MAX    24
#define CATALOG_DESC_MAX       160
#define CATALOG_CPU_BADGE_MAX  32
#define CATALOG_URL_MAX        192
#define CATALOG_FILENAME_MAX   96
#define CATALOG_SHA_HEX_MAX    65

typedef struct catalog_artifact {
    char type[8];          /* uf2, wasm, bin, zip, other */
    char url[CATALOG_URL_MAX];
    char filename[CATALOG_FILENAME_MAX];
    char sha256_hex[CATALOG_SHA_HEX_MAX];
    uint32_t size;
    bool has_sha;
    bool has_size;
} catalog_artifact_t;

typedef struct catalog_app {
    char id[CATALOG_ID_MAX];
    char name[CATALOG_NAME_MAX];
    char version[CATALOG_VERSION_MAX];
    char description[CATALOG_DESC_MAX];
    char cpu_badge[CATALOG_CPU_BADGE_MAX]; /* e.g. "display · main" */
    char kind[16];         /* firmware, script, content, … */
    bool replaces_stock;
    catalog_artifact_t art; /* first installable artifact */
    bool has_artifact;
} catalog_app_t;

typedef struct catalog {
    int version;
    int app_count;
    catalog_app_t apps[CATALOG_MAX_APPS];
} catalog_t;

/* Parse catalog.json text. Returns true on success (partial OK if truncated). */
bool catalog_parse(const char *json, size_t len, catalog_t *out);

/* Map kind string → STORE_KIND_* (0..4). */
uint8_t catalog_kind_code(const char *kind);

/* Map artifact type string → STORE_ART_* . */
uint8_t catalog_art_code(const char *type);

/* Find app by id; NULL if missing. */
const catalog_app_t *catalog_find(const catalog_t *c, const char *id);

#endif
