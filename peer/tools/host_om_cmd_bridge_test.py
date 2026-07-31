#!/usr/bin/env python3
"""Host unit test for om_cmd_bridge (no Pico SDK). Requires gcc on PATH."""
from __future__ import annotations

import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / "om_cmd_bridge.c"
HDR = ROOT / "om_cmd_bridge.h"

TEST_C = r"""
#include "om_cmd_bridge.h"
#include <stdio.h>
#include <string.h>

static char cdc_buf[8][256];
static int cdc_n;
static char ev_id[8][32];
static char ev_args[8][256];
static int ev_n;

static void on_cdc(const char *line, void *user) {
    (void)user;
    if (cdc_n < 8) snprintf(cdc_buf[cdc_n++], 256, "%s", line);
}
static void on_emit(const char *id, const char *args, void *user) {
    (void)user;
    if (ev_n < 8) {
        snprintf(ev_id[ev_n], 32, "%s", id);
        snprintf(ev_args[ev_n], 256, "%s", args);
        ev_n++;
    }
}

int main(void) {
    om_cmd_bridge_t b;
    om_cmd_bridge_init(&b, on_emit, on_cdc, 0);

    if (!om_cmd_bridge_handle_display_line(&b, "a\\om\\a accept")) return 1;
    if (cdc_n != 1) return 2;
    if (!strstr(cdc_buf[0], "\"accept\"")) return 3;

    if (!om_cmd_bridge_handle_display_line(&b, "a\\om\\w review-pr")) return 4;
    if (!strstr(cdc_buf[1], "review-pr")) return 5;

    if (!om_cmd_bridge_handle_cdc_line(&b,
            "{\"v\":1,\"type\":\"feedback\",\"sessions\":[]}")) return 6;
    if (ev_n < 1 || strcmp(ev_id[0], "omFb") != 0) return 7;

    puts("ok");
    return 0;
}
"""


def main() -> int:
    if not SRC.exists():
        print("missing", SRC, file=sys.stderr)
        return 1
    with tempfile.TemporaryDirectory() as td:
        tdp = pathlib.Path(td)
        (tdp / "om_cmd_bridge.h").write_text(HDR.read_text(encoding="utf-8"), encoding="utf-8")
        (tdp / "om_cmd_bridge.c").write_text(SRC.read_text(encoding="utf-8"), encoding="utf-8")
        (tdp / "test.c").write_text(TEST_C, encoding="utf-8")
        exe = tdp / ("test.exe" if sys.platform == "win32" else "test")
        cmd = ["gcc", "-O0", "-Wall", "-I.", str(tdp / "om_cmd_bridge.c"), str(tdp / "test.c"), "-o", str(exe)]
        print("+", " ".join(cmd))
        subprocess.check_call(cmd)
        out = subprocess.check_output([str(exe)], text=True)
        print(out.strip())
        return 0 if out.strip() == "ok" else 1


if __name__ == "__main__":
    raise SystemExit(main())
