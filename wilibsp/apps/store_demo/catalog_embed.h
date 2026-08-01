#ifndef CATALOG_EMBED_H
#define CATALOG_EMBED_H

/* Embedded copy of web/public/api/v1/catalog.json (built-in store demo). */
static const char k_catalog_json[] =
    "{\"store\":\"FW2 Store\",\"catalog_version\":1,\"apps\":[{\"id\":\"wilii"
    "r\",\"name\":\"WiliIR\",\"version\":\"0.0.0-placeholder\",\"description\""
    ":\"Learn, decode, and replay IR with Flipper-compatible .ir files on a U"
    "SB stick.\",\"tags\":[\"ir\",\"tool\",\"usb-content\"],\"targets\":{\"di"
    "splaycpu\":{\"file\":\"apps/wiliir/wiliir-placeholder.txt\",\"size\":143"
    ",\"sha256\":\"6841c03a3b5fef5d41c2f884db2a21711f9c9472086336f1b682948675"
    "3e3f32\"}}},{\"id\":\"sensorview\",\"name\":\"SensorView\",\"version\":\""
    "0.0.0-placeholder\",\"description\":\"Live instruments for environment, "
    "motion, compass, spectrum, and acoustic direction finding.\",\"tags\":[\""
    "sensors\",\"audio\",\"tool\"],\"targets\":{\"displaycpu\":{\"file\":\"ap"
    "ps/sensorview/sensorview-placeholder.txt\",\"size\":103,\"sha256\":\"8e5"
    "657868f3c389c9dc0b315588bbafcb956ad9729d8ad0291ff99a5c46180a4\"}}},{\"id"
    "\":\"subghz\",\"name\":\"subghz\",\"version\":\"0.0.0-placeholder\",\"de"
    "scription\":\"CC1101 spectrum analyzer, live monitor, and OOK capture/re"
    "play for ISM bands.\",\"tags\":[\"radio\",\"tool\"],\"targets\":{\"displ"
    "aycpu\":{\"file\":\"apps/subghz/subghz-placeholder.txt\",\"size\":158,\""
    "sha256\":\"095fc4e687b5bd45f2daf387ad4d7accbd4d073b33a51762d85a46062b404"
    "85b\"}}},{\"id\":\"guitarman\",\"name\":\"GuitarMan\",\"version\":\"0.0."
    "0-placeholder\",\"description\":\"Scrolling chord-over-lyric teleprompte"
    "r for guitar players; songs from USB or ROM.\",\"tags\":[\"music\",\"usb"
    "-content\"],\"targets\":{\"displaycpu\":{\"file\":\"apps/guitarman/guita"
    "rman-placeholder.txt\",\"size\":101,\"sha256\":\"113c2e47e718b454b132c0d"
    "d2d9096e83cd18a5ded78f055f1a2bb71b36666ec\"}}},{\"id\":\"wiliplayer\",\""
    "name\":\"wiliplayer\",\"version\":\"0.0.0-placeholder\",\"description\":"
    "\"USB movie player for LCD and DVI/HDMI, controlled with a Roku IR remot"
    "e.\",\"tags\":[\"media\",\"usb-content\",\"dvi\"],\"targets\":{\"display"
    "cpu\":{\"file\":\"apps/wiliplayer/wiliplayer-placeholder.txt\",\"size\":"
    "104,\"sha256\":\"650a96674ace34f71ab977819d45273eaa8c6298993b233ac609fd8"
    "f3dfc65c7\"}}},{\"id\":\"leds-rainbow\",\"name\":\"LED Rainbow\",\"versi"
    "on\":\"0.1.0-demo\",\"description\":\"Demo script artifact for store E2E"
    " \\u2014 keeps stock firmware; copy to stock /scripts after download.\","
    "\"tags\":[\"leds\",\"demo\",\"offline\",\"script\"],\"targets\":{\"mainc"
    "pu\":{\"file\":\"apps/leds-rainbow/hello.wasm\",\"size\":213,\"sha256\":"
    "\"3bb7c35e6e13f205b5ae014eceeea5d6cb0e716dba34efba168d7322b70da101\"}}},"
    "{\"id\":\"demo-content\",\"name\":\"Demo Content Pack\",\"version\":\"0."
    "1.0-demo\",\"description\":\"Tiny content file for store download E2E (U"
    "SB content/ tree).\",\"tags\":[\"demo\",\"usb-content\"],\"targets\":{\""
    "maincpu\":{\"file\":\"apps/demo-content/readme.txt\",\"size\":293,\"sha2"
    "56\":\"ba3c6ec807a0d830a652df2b957ccc536d5192de2ab43db7a833be0d03c09db9\""
    "}}}]}";

static const unsigned k_catalog_json_len = sizeof(k_catalog_json) - 1;

#endif
