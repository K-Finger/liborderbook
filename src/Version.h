#pragma once

#define ORDERBOOK_VERSION_MAJOR 2
#define ORDERBOOK_VERSION_MINOR 2
#define ORDERBOOK_VERSION_PATCH 0

#define ORDERBOOK_VERSION_STRING "2.2.0"

#define ORDERBOOK_VERSION \
    (ORDERBOOK_VERSION_MAJOR * 10000 + ORDERBOOK_VERSION_MINOR * 100 + ORDERBOOK_VERSION_PATCH)

namespace orderbook {

constexpr int versionMajor = ORDERBOOK_VERSION_MAJOR;
constexpr int versionMinor = ORDERBOOK_VERSION_MINOR;
constexpr int versionPatch = ORDERBOOK_VERSION_PATCH;
constexpr const char* versionString = ORDERBOOK_VERSION_STRING;

}
