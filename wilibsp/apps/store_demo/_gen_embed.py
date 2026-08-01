#!/usr/bin/env python3
"""Generate catalog_embed.h from web/public/api/v1/catalog.json."""
import json
import pathlib

root = pathlib.Path(__file__).resolve().parents[4]
src = root / "web" / "public" / "api" / "v1" / "catalog.json"
out = pathlib.Path(__file__).resolve().parent / "catalog_embed.h"

obj = json.loads(src.read_text(encoding="utf-8"))
compact = json.dumps(obj, separators=(",", ":"))

chunks = []
line = ""
for ch in compact:
    if ch == "\\":
        line += "\\\\"
    elif ch == '"':
        line += '\\"'
    elif ch == "\n":
        line += "\\n"
    else:
        line += ch
    if len(line) >= 72:
        chunks.append(line)
        line = ""
if line:
    chunks.append(line)

body = "\n".join(f'    "{c}"' for c in chunks)
hdr = f"""#ifndef CATALOG_EMBED_H
#define CATALOG_EMBED_H

/* Embedded copy of web/public/api/v1/catalog.json (built-in store demo). */
static const char k_catalog_json[] =
{body};

static const unsigned k_catalog_json_len = sizeof(k_catalog_json) - 1;

#endif
"""
out.write_text(hdr, encoding="utf-8")
print(f"wrote {out} ({len(compact)} bytes JSON)")
