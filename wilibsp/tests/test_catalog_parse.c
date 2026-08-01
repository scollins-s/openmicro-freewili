#include "test_util.h"
#include "catalog_parse.h"
#include <string.h>

static const char *kSample =
    "{"
    "\"version\":1,"
    "\"apps\":["
    "{"
    "\"id\":\"demo-script\","
    "\"name\":\"Demo Script\","
    "\"kind\":\"script\","
    "\"kind_details\":{\"replaces_stock_firmware\":false},"
    "\"targets\":[{\"device\":\"freewili2\",\"artifacts\":[{"
    "\"type\":\"wasm\",\"url\":\"/artifacts/demo-script/hello.wasm\","
    "\"sha256\":\"abc\",\"size\":12,\"filename\":\"hello.wasm\""
    "}]}]"
    "},"
    "{"
    "\"id\":\"wiliir\","
    "\"name\":\"WiliIR\","
    "\"kind\":\"firmware\","
    "\"kind_details\":{\"replaces_stock_firmware\":true},"
    "\"targets\":[{\"device\":\"freewili2\",\"artifacts\":[{"
    "\"type\":\"uf2\",\"url\":\"/artifacts/wiliir/x.uf2\","
    "\"sha256\":\"def\",\"size\":100,\"filename\":\"wiliir.uf2\""
    "}]}]"
    "},"
    "{"
    "\"id\":\"community\","
    "\"name\":\"Community\","
    "\"kind\":\"project\","
    "\"source_url\":\"https://github.com/x/y\","
    "\"targets\":[]"
    "}"
    "]"
    "}";

int main(void) {
    catalog_t cat;
    ASSERT_TRUE(catalog_parse(kSample, strlen(kSample), &cat));
    ASSERT_EQ(cat.version, 1);
    ASSERT_EQ(cat.app_count, 2);
    ASSERT_TRUE(strcmp(cat.apps[0].id, "demo-script") == 0);
    ASSERT_TRUE(strcmp(cat.apps[0].kind, "script") == 0);
    ASSERT_TRUE(cat.apps[0].has_artifact);
    ASSERT_TRUE(strcmp(cat.apps[0].art.filename, "hello.wasm") == 0);
    ASSERT_EQ(cat.apps[0].art.size, 12);
    ASSERT_TRUE(!cat.apps[0].replaces_stock);
    ASSERT_TRUE(cat.apps[1].replaces_stock);
    ASSERT_EQ(catalog_kind_code("firmware"), 2);
    ASSERT_EQ(catalog_art_code("wasm"), 1);
    ASSERT_TRUE(catalog_find(&cat, "wiliir") != NULL);
    ASSERT_TRUE(catalog_find(&cat, "missing") == NULL);
    puts("catalog_parse: ok");
    TEST_RETURN();
}
