#pragma once

#include <esp_err.h>

enum proto_t {
    URL_PROTO_UNKNOWN,
    URL_PROTO_HTTP,
    URL_PROTO_HTTPS,
    URL_PROTO_FILE
};

struct url_parsed {
    enum proto_t proto;
    unsigned short port;
    char domain[73];
    char file[116];
};

esp_err_t url_parse(const char *url, struct url_parsed *parsed);