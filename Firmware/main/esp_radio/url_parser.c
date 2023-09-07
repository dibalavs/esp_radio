#include "esp_radio/url_parser.h"
#include "esp_err.h"

#include <stdlib.h>
#include <string.h>

esp_err_t url_parse(const char *url, struct url_parsed *parsed)
{
    unsigned domain_offset = 0;
    unsigned domain_end = 0;
    const char *curr;

    memset(parsed, 0, sizeof(*parsed));

    if (strstr(url, "http://") || strstr(url, "HTTP://")) {
        parsed->proto = URL_PROTO_HTTP;
        domain_offset += sizeof("http://") - 1;
        strcpy(parsed->domain, "http://");
        parsed->port = 80;

    } else if (strstr(url, "https://") || strstr(url, "HTTPS://")) {
        parsed->proto = URL_PROTO_HTTPS;
        domain_offset += sizeof("https://") - 1;
        strcpy(parsed->domain, "https://");
        parsed->port = 443;

    } else if (strstr(url, "file://") || strstr(url, "FILE://")) {
        parsed->proto = URL_PROTO_FILE;
        domain_offset += sizeof("file://") - 1;
        strcpy(parsed->domain, "file://");

    } else if (strstr(url, "://")) {
        parsed->proto = URL_PROTO_UNKNOWN;
        return ESP_FAIL;

    } else {
        // Default - http
        parsed->proto = URL_PROTO_HTTP;
        parsed->port = 80;
        strcpy(parsed->domain, "http://");
    }

    curr = strstr(url + domain_offset, ":");
    if (curr) {
        parsed->port = atoi(curr + 1);
        domain_end = curr - url - domain_offset;
    }

    curr = strstr(url + domain_offset, "/");
    if (curr) {
        domain_end = curr - url - domain_offset;
        strcpy(parsed->file, curr+1);
    }

    if (domain_end) {
        strncat(parsed->domain, url + domain_offset, domain_end);
    } else {
        strcat(parsed->domain, url + domain_offset);
    }

    return ESP_OK;
}
