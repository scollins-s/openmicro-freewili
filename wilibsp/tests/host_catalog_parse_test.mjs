/**
 * Catalog parse fixture test (mirrors wilibsp/tests/test_catalog_parse.c logic).
 *   node "device stuff/wilibsp/tests/host_catalog_parse_test.mjs"
 */
import assert from "node:assert/strict";

const sample = `{
  "version": 1,
  "apps": [
    {
      "id": "demo-script",
      "name": "Demo Script",
      "kind": "script",
      "kind_details": { "replaces_stock_firmware": false },
      "targets": [{
        "device": "freewili2",
        "artifacts": [{
          "type": "wasm",
          "url": "/artifacts/demo-script/hello.wasm",
          "sha256": "abc",
          "size": 12,
          "filename": "hello.wasm"
        }]
      }]
    },
    {
      "id": "wiliir",
      "name": "WiliIR",
      "kind": "firmware",
      "kind_details": { "replaces_stock_firmware": true },
      "targets": [{
        "device": "freewili2",
        "artifacts": [{
          "type": "uf2",
          "url": "/artifacts/wiliir/x.uf2",
          "sha256": "def",
          "size": 100,
          "filename": "wiliir.uf2"
        }]
      }]
    },
    {
      "id": "community",
      "name": "Community",
      "kind": "project",
      "source_url": "https://github.com/x/y",
      "targets": []
    }
  ]
}`;

function extractString(obj, key) {
  const re = new RegExp(`"${key}"\\s*:\\s*"((?:\\\\.|[^"\\\\])*)"`);
  const m = obj.match(re);
  return m ? m[1] : "";
}

function extractBool(obj, key) {
  const re = new RegExp(`"${key}"\\s*:\\s*(true|false)`);
  const m = obj.match(re);
  return m ? m[1] === "true" : false;
}

function extractU32(obj, key) {
  const re = new RegExp(`"${key}"\\s*:\\s*(\\d+)`);
  const m = obj.match(re);
  return m ? Number(m[1]) : null;
}

const apps = [];
const appsMatch = sample.match(/"apps"\s*:\s*\[([\s\S]*)\]\s*\}\s*$/);
assert.ok(appsMatch);
// crude split on top-level objects — enough for fixture
const bodies = sample.split(/\{\s*"id"/).slice(1);
for (const body of bodies) {
  const obj = `{"id"${body}`;
  const id = extractString(obj, "id");
  const kind = extractString(obj, "kind");
  if (!id || kind === "project") continue;
  apps.push({
    id,
    name: extractString(obj, "name"),
    kind,
    replaces: extractBool(obj, "replaces_stock_firmware"),
    filename: extractString(obj, "filename"),
    size: extractU32(obj, "size"),
  });
}

assert.equal(apps.length, 2);
assert.equal(apps[0].id, "demo-script");
assert.equal(apps[0].filename, "hello.wasm");
assert.equal(apps[0].size, 12);
assert.equal(apps[0].replaces, false);
assert.equal(apps[1].replaces, true);
console.log("host_catalog_parse_test: ok");
