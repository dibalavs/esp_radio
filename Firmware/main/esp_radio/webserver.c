#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "esp_radio/webserver.h"

#include "esp_mac.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_radio/action_manager.h"
#include "esp_radio/app_state.h"
#include "esp_radio/url_parser.h"

#include <cJSON.h>
#include <eeprom.h>
#include <esp_mac.h>
#include <esp_app_desc.h>
#include <esp_chip_info.h>
#include <esp_heap_caps.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_random.h>
#include <esp_vfs.h>
#include <esp_wifi.h>

#include <fcntl.h>
#include <stdlib.h>

static const char *TAG = "webserver_new";

#define API_METHOD(name) "/api/v1/" # name

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (65536)
#define BASE_PATH "/w"

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html.gz")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js.gz")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css.gz")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png.gz")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico.gz")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg.gz")) {
        type = "text/xml";
    }

    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_set_type(req, type);
}

static esp_err_t sysinfo_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "sysinfo handler");
    uint32_t all = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    uint32_t iram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    uint32_t psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateArray();

    /// system info
    cJSON *system_root = cJSON_CreateObject();
    cJSON *values = cJSON_CreateArray();

    cJSON *object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "name", "Version");
    cJSON_AddStringToObject(object, "value", esp_app_get_description()->version);
    cJSON_AddItemToArray(values, object);

    object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "name", "Uptime (min)");
    cJSON_AddNumberToObject(object, "value", (double)(xTaskGetTickCount() * portTICK_PERIOD_MS / 1000 / 60));
    cJSON_AddItemToArray(values, object);

    object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "name", "Total free RAM(kb)");
    cJSON_AddNumberToObject(object, "value", all / 1024);
    cJSON_AddItemToArray(values, object);

    object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "name", "Free IRAM (Kb)");
    cJSON_AddNumberToObject(object, "value", iram / 1024);
    cJSON_AddItemToArray(values, object);

    object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "name", "Free PSRAM (Kb)");
    cJSON_AddNumberToObject(object, "value", psram / 1024);
    cJSON_AddItemToArray(values, object);

    cJSON_AddStringToObject(system_root, "name", "System");
    cJSON_AddItemToObject(system_root, "values", values);
    cJSON_AddItemToArray(root, system_root);

    // WiFi info
    cJSON *wifi_root = cJSON_CreateObject();
    cJSON *wifi = cJSON_CreateArray();
    wifi_mode_t mode;
    wifi_config_t conf;
    uint8_t mac[6];
    char mac_str[20];
    wifi_ap_record_t ap;
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap));
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
    ESP_ERROR_CHECK(esp_read_mac(mac, mode == WIFI_MODE_STA ? ESP_MAC_WIFI_STA : ESP_MAC_WIFI_SOFTAP));
    snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(mac));

    object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "name", "Mode");
    cJSON_AddStringToObject(object, "value", mode == WIFI_MODE_AP ? "Access point" : "Station");
    cJSON_AddItemToArray(wifi, object);

    object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "name", "SSID");
    cJSON_AddStringToObject(object, "value", (char *)(mode == WIFI_MODE_STA ? conf.sta.ssid : conf.ap.ssid));
    cJSON_AddItemToArray(wifi, object);

    object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "name", "Ip");
    cJSON_AddStringToObject(object, "value", app_get_ip());
    cJSON_AddItemToArray(wifi, object);

    object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "name", "MAC");
    cJSON_AddStringToObject(object, "value", mac_str);
    cJSON_AddItemToArray(wifi, object);

    object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "name", "Signal");
    cJSON_AddNumberToObject(object, "value", ap.rssi);
    cJSON_AddItemToArray(wifi, object);

    object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "name", "Hostname");
    cJSON_AddStringToObject(object, "value", g_device->hostname);
    cJSON_AddItemToArray(wifi, object);

    cJSON_AddStringToObject(wifi_root, "name", "Wifi");
    cJSON_AddItemToObject(wifi_root, "values", wifi);
    cJSON_AddItemToArray(root, wifi_root);

    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t player_info_get_handler(httpd_req_t *req)
{
    struct shoutcast_info *ipinfo = NULL;
    struct fmstation_info fminfo;
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "is_fm", app_state_is_fm());
    cJSON_AddBoolToObject(root, "is_playing", action_is_running());
    cJSON_AddNumberToObject(root, "volume", app_state_get_ivol());
    cJSON_AddStringToObject(root, "station_type", app_state_is_fm() ? "FM" : "IP");
    cJSON_AddNumberToObject(root, "station_no", action_getstation());
    if (app_state_is_fm()) {
        eeprom_get_fmstation(action_getstation(), &fminfo);
        cJSON_AddStringToObject(root, "station_name", fminfo.name);
    } else {
        ipinfo = eeprom_get_webstation(action_getstation());
        if (ipinfo)
            cJSON_AddStringToObject(root, "station_name", ipinfo->name);
    }

    cJSON_AddStringToObject(root, "track_name", "TODO");
    char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free(sys_info);
    free(ipinfo);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t player_pause_get_handler(httpd_req_t *req)
{
    if (action_is_running())
        action_stop();
    else
        action_switch(0);
    httpd_resp_sendstr(req, "Ok");
    return ESP_OK;
}

static esp_err_t fetch_post_data(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }

    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    return ESP_OK;
}

static esp_err_t player_volume_post_handler(httpd_req_t *req)
{
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    if (fetch_post_data(req) != ESP_OK)
        return ESP_FAIL;

    cJSON *root = cJSON_Parse(buf);
    int volume = cJSON_GetObjectItem(root, "value")->valueint;
    action_set_volume(volume);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

static esp_err_t player_prev_get_handler(httpd_req_t *req)
{
    action_switch(-1);
    httpd_resp_sendstr(req, "Ok");
    return ESP_OK;
}

static esp_err_t player_next_get_handler(httpd_req_t *req)
{
    action_switch(1);
    httpd_resp_sendstr(req, "Ok");
    return ESP_OK;
}

static esp_err_t player_fmradio_get_handler(httpd_req_t *req)
{
    action_fmstation_switch(0);
    httpd_resp_sendstr(req, "Ok");
    return ESP_OK;
}

static esp_err_t player_ipradio_get_handler(httpd_req_t *req)
{
    action_webstation_switch(0);
    httpd_resp_sendstr(req, "Ok");
    return ESP_OK;
}

static esp_err_t file_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    strlcat(filepath, ".gz", sizeof(filepath));

    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);
    httpd_resp_set_hdr(req, "Cache-Control", "max-age=604800");

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        ESP_LOGI(TAG, "Read file: %s, size:%zd", filepath, read_bytes);
        if (read_bytes == -1) {
            ESP_LOGE(TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(TAG, "File sending failed!");
                httpd_resp_sendstr_chunk(req, NULL);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    close(fd);
    ESP_LOGI(TAG, "File sending complete");
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

///////////////////// ipradio handlers ///////////////////////////////////////
static esp_err_t ipradio_list_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateArray();
    uint16_t total = app_state_get_webradio_total();
    for (unsigned i = 0; total != NO_STATION && i < total; i++) {
        cJSON * item = cJSON_CreateObject();
        if (!item) {
            ESP_LOGE(TAG, "Failed to allocate memory");
            cJSON_Delete(root);
            return ESP_FAIL;
        }

        struct shoutcast_info* info = eeprom_get_webstation(i);
        if (!info) {
            ESP_LOGE(TAG, "Failed to allocate memory");
            cJSON_Delete(root);
            cJSON_Delete(item);
            return ESP_FAIL;
        }

        cJSON_AddNumberToObject(item, "no", i);
        cJSON_AddStringToObject(item, "name", info->name);

        char buff[255];
        snprintf(buff, 255, "%s:%d/%s", info->domain, info->port, info->file);
        cJSON_AddStringToObject(item, "url", buff);
        cJSON_AddNumberToObject(item, "ovol", info->ovol);
        cJSON_AddItemToArray(root, item);
        free(info);
    }

    char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free(sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

#define JS_TRY_GET(elem, name) \
  ({ val = cJSON_GetObjectItem(elem, name); \
    if (!val) { \
        ESP_LOGE(TAG, "Element '%s' not found in array", name); \
        continue; \
    } \
    val; \
  })

static esp_err_t ipradio_import_post_handler(httpd_req_t *req)
{
    struct shoutcast_info info;
    cJSON *val = NULL;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    if (fetch_post_data(req) != ESP_OK)
        return ESP_FAIL;

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        ESP_LOGE(TAG, "Unable to parse buffer. err:%s\n data:%s", cJSON_GetErrorPtr(), buf);
        return ESP_FAIL;
    }

    int n = cJSON_GetArraySize(root);
    ESP_LOGI(TAG, "Import IP radio num stations:%d", n);
    for (int i = 0; i < n; i++) {
        cJSON *elem = cJSON_GetArrayItem(root, i);
        if (!elem) {
            ESP_LOGE(TAG, "Element %d nout found in array", i);
            continue;
        }

        strcpy(info.name, JS_TRY_GET(elem, "name")->valuestring);
        int no = JS_TRY_GET(elem, "no")->valueint;
        int ovol = JS_TRY_GET(elem, "ovol")->valueint;
        const char *url_raw = JS_TRY_GET(elem, "url")->valuestring;
        struct url_parsed url;
        if (url_parse(url_raw, &url) != ESP_OK)
            return ESP_FAIL;

        strcpy(info.domain, url.domain);
        ESP_LOGI(TAG, "Import IP station:%s %s", info.name, info.domain);

        val = cJSON_GetObjectItem(elem, "File");
        if (val)
            strcpy(info.file, val->string);
        else
            strcpy(info.file, url.file);

        info.port = url.port;
        info.ovol = ovol;
        if (no < n)
            eeprom_save_webstation(&info, no);
    }

    app_state_set_webradio_total(n);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

static esp_err_t ipradio_set_post_handler(httpd_req_t *req)
{
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    if (fetch_post_data(req) != ESP_OK)
        return ESP_FAIL;

    cJSON *root = cJSON_Parse(buf);
    int station = cJSON_GetObjectItem(root, "station_no")->valueint;
    action_webstation_set(station);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Ok");
    return ESP_OK;
}

static esp_err_t ipradio_move_post_handler(httpd_req_t *req)
{
    cJSON *val = NULL;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    if (fetch_post_data(req) != ESP_OK)
        return ESP_FAIL;

    cJSON *root = cJSON_Parse(buf);
    do {
        unsigned prev = JS_TRY_GET(root, "prev")->valueint;
        unsigned curr = JS_TRY_GET(root, "curr")->valueint;

        int n = app_state_get_webradio_total();
        if (prev >= n || curr >= n) {
            ESP_LOGE(TAG, "Invalid number of station");
            cJSON_Delete(root);
            return ESP_FAIL;
        }

        struct shoutcast_info *p = eeprom_get_webstation(prev);
        struct shoutcast_info *c = eeprom_get_webstation(curr);
        eeprom_save_webstation(p, curr);
        eeprom_save_webstation(c, prev);
        free(p);
        free(c);

    } while (0);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Ok");
    return ESP_OK;
}

static esp_err_t ipradio_export_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateArray();
    uint16_t total = app_state_get_webradio_total();
    for (unsigned i = 0; total != NO_STATION && i < total; i++) {
        cJSON * item = cJSON_CreateObject();
        if (!item) {
            ESP_LOGE(TAG, "Failed to allocate memory");
            cJSON_Delete(root);
            return ESP_FAIL;
        }

        struct shoutcast_info* info = eeprom_get_webstation(i);
        if (!info) {
            ESP_LOGE(TAG, "Failed to allocate memory");
            cJSON_Delete(root);
            cJSON_Delete(item);
            return ESP_FAIL;
        }

        cJSON_AddNumberToObject(item, "no", i);
        cJSON_AddStringToObject(item, "name", info->name);

        char buff[255];
        snprintf(buff, 255, "%s:%d/%s", info->domain, info->port, info->file);
        cJSON_AddStringToObject(item, "url", buff);
        cJSON_AddNumberToObject(item, "ovol", info->ovol);
        cJSON_AddItemToArray(root, item);
        free(info);
    }

    char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free(sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

///////////////////// fmradio handlers ///////////////////////////////////////
static esp_err_t fmradio_list_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateArray();
    uint16_t total = app_state_get_fmradio_total();
    for (unsigned i = 0; total != NO_STATION && i < total; i++) {
        cJSON * item = cJSON_CreateObject();
        if (!item) {
            ESP_LOGE(TAG, "Failed to allocate memory");
            cJSON_Delete(root);
            return ESP_FAIL;
        }

        struct fmstation_info info;
        esp_err_t res = eeprom_get_fmstation(i, &info);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to allocate memory");
            cJSON_Delete(item);
            cJSON_Delete(root);
            return ESP_FAIL;
        }

        cJSON_AddNumberToObject(item, "no", i);
        cJSON_AddStringToObject(item, "name", info.name);
        cJSON_AddNumberToObject(item, "ovol", info.ovol);
        cJSON_AddNumberToObject(item, "frequency", info.frequency_mhz);
        cJSON_AddItemToArray(root, item);
    }

    char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free(sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t fmradio_import_post_handler(httpd_req_t *req)
{
    struct fmstation_info info;
    cJSON *val = NULL;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    if (fetch_post_data(req) != ESP_OK)
        return ESP_FAIL;

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        ESP_LOGE(TAG, "Unable to parse buffer. err:%s\n data:%s", cJSON_GetErrorPtr(), buf);
        return ESP_FAIL;
    }

    int n = cJSON_GetArraySize(root);
    ESP_LOGI(TAG, "Import FM radio num stations:%d", n);
    for (int i = 0; i < n; i++) {
        cJSON *elem = cJSON_GetArrayItem(root, i);
        if (!elem) {
            ESP_LOGE(TAG, "Element %d nout found in array", i);
            continue;
        }

        strcpy(info.name, JS_TRY_GET(elem, "name")->valuestring);
        int no = JS_TRY_GET(elem, "no")->valueint;
        info.ovol = JS_TRY_GET(elem, "ovol")->valueint;
        info.frequency_mhz = JS_TRY_GET(elem, "frequency")->valuedouble;
        ESP_LOGI(TAG, "Import FM station: %f Mhz %s", info.frequency_mhz, info.name);

        if (no < n)
            eeprom_save_fmstation(&info, no);
    }

    app_state_set_fmradio_total(n);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

static esp_err_t fmradio_set_post_handler(httpd_req_t *req)
{
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    if (fetch_post_data(req) != ESP_OK)
        return ESP_FAIL;

    cJSON *root = cJSON_Parse(buf);
    int station = cJSON_GetObjectItem(root, "radio_no")->valueint;
    action_fmstation_set(station);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Ok");
    return ESP_OK;
}

static esp_err_t fmradio_move_post_handler(httpd_req_t *req)
{
    cJSON *val = NULL;
    esp_err_t res;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    if (fetch_post_data(req) != ESP_OK)
        return ESP_FAIL;

    cJSON *root = cJSON_Parse(buf);
    do {
        unsigned prev = JS_TRY_GET(root, "prev")->valueint;
        unsigned curr = JS_TRY_GET(root, "curr")->valueint;

        int n = app_state_get_fmradio_total();
        if (prev >= n || curr >= n) {
            ESP_LOGE(TAG, "Invalid number of station");
            cJSON_Delete(root);
            return ESP_FAIL;
        }

        struct fmstation_info p;
        struct fmstation_info c;
        res = eeprom_get_fmstation(prev, &p);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get station %d, err:%x", prev, res);
            cJSON_Delete(root);
            return ESP_FAIL;
        }

        res = eeprom_get_fmstation(curr, &c);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get station %d, err:%x", curr, res);
            cJSON_Delete(root);
            return ESP_FAIL;
        }

        eeprom_save_fmstation(&p, curr);
        eeprom_save_fmstation(&c, prev);

    } while (0);

    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Ok");
    return ESP_OK;
}

static esp_err_t fmradio_export_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateArray();
    uint16_t total = app_state_get_fmradio_total();
    for (unsigned i = 0; total != NO_STATION && i < total; i++) {
        cJSON * item = cJSON_CreateObject();
        if (!item) {
            ESP_LOGE(TAG, "Failed to allocate memory");
            cJSON_Delete(root);
            return ESP_FAIL;
        }

        struct fmstation_info info;
        esp_err_t res = eeprom_get_fmstation(i, &info);
        if (res != ESP_OK) {
            ESP_LOGE(TAG, "Failed to allocate memory");
            cJSON_Delete(item);
            cJSON_Delete(root);
            return ESP_FAIL;
        }

        cJSON_AddNumberToObject(item, "no", i);
        cJSON_AddStringToObject(item, "name", info.name);
        cJSON_AddNumberToObject(item, "ovol", info.ovol);
        cJSON_AddNumberToObject(item, "frequency", info.frequency_mhz);
        cJSON_AddItemToArray(root, item);
    }

    char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free(sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////////

void webserver_initialize(void)
{
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    strlcpy(rest_context->base_path, BASE_PATH, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.max_uri_handlers = 50;
    config.server_port = 8080;
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    ////////////////////////// player handlers ///////////////////////////////////////
    {
        const httpd_uri_t player_info_get_uri = {
            .uri = API_METHOD(player_info),
            .method = HTTP_GET,
            .handler = player_info_get_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &player_info_get_uri);
    }

    {
        const httpd_uri_t player_pause_get_uri = {
            .uri = API_METHOD(player_pause),
            .method = HTTP_GET,
            .handler = player_pause_get_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &player_pause_get_uri);
    }

    {
        const httpd_uri_t player_volume_post_uri = {
            .uri = API_METHOD(player_volume),
            .method = HTTP_POST,
            .handler = player_volume_post_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &player_volume_post_uri);
    }

    {
        const httpd_uri_t player_prev_get_uri = {
            .uri = API_METHOD(player_prev),
            .method = HTTP_GET,
            .handler = player_prev_get_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &player_prev_get_uri);
    }

    {
        const httpd_uri_t player_next_get_uri = {
            .uri = API_METHOD(player_next),
            .method = HTTP_GET,
            .handler = player_next_get_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &player_next_get_uri);
    }

    {
        const httpd_uri_t player_fmradio_get_uri = {
            .uri = API_METHOD(player_fmradio),
            .method = HTTP_GET,
            .handler = player_fmradio_get_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &player_fmradio_get_uri);
    }

    {
        const httpd_uri_t player_ipradio_get_uri = {
            .uri = API_METHOD(player_ipradio),
            .method = HTTP_GET,
            .handler = player_ipradio_get_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &player_ipradio_get_uri);
    }
    ///////////////////////////// ipradio handlers ////////////////////////////////////
    {
        const httpd_uri_t ipradio_list_get_uri = {
            .uri = API_METHOD(ipradio_list),
            .method = HTTP_GET,
            .handler = ipradio_list_get_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &ipradio_list_get_uri);
    }

    {
        const httpd_uri_t ipradio_import_post_uri = {
            .uri = API_METHOD(ipradio_import),
            .method = HTTP_POST,
            .handler = ipradio_import_post_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &ipradio_import_post_uri);
    }

    {
        const httpd_uri_t ipradio_set_post_uri = {
            .uri = API_METHOD(ipradio_set),
            .method = HTTP_POST,
            .handler = ipradio_set_post_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &ipradio_set_post_uri);
    }

    {
        const httpd_uri_t ipradio_move_post_uri = {
            .uri = API_METHOD(ipradio_move),
            .method = HTTP_POST,
            .handler = ipradio_move_post_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &ipradio_move_post_uri);
    }

    {
        const httpd_uri_t ipradio_export_get_uri = {
            .uri = API_METHOD(ipradio_export),
            .method = HTTP_GET,
            .handler = ipradio_export_get_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &ipradio_export_get_uri);
    }

    ///////////////////////////// fmradio handlers ////////////////////////////////////
    {
        const httpd_uri_t fmradio_list_get_uri = {
            .uri = API_METHOD(fmradio_list),
            .method = HTTP_GET,
            .handler = fmradio_list_get_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &fmradio_list_get_uri);
    }

    {
        const httpd_uri_t fmradio_import_post_uri = {
            .uri = API_METHOD(fmradio_import),
            .method = HTTP_POST,
            .handler = fmradio_import_post_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &fmradio_import_post_uri);
    }

    {
        const httpd_uri_t fmradio_set_post_uri = {
            .uri = API_METHOD(fmradio_set),
            .method = HTTP_POST,
            .handler = fmradio_set_post_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &fmradio_set_post_uri);
    }

    {
        const httpd_uri_t fmradio_move_post_uri = {
            .uri = API_METHOD(fmradio_move),
            .method = HTTP_POST,
            .handler = fmradio_move_post_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &fmradio_move_post_uri);
    }

    {
        const httpd_uri_t fmradio_export_get_uri = {
            .uri = API_METHOD(fmradio_export),
            .method = HTTP_GET,
            .handler = fmradio_export_get_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &fmradio_export_get_uri);
    }

    {
        const httpd_uri_t sysinfo_get_uri = {
            .uri = API_METHOD(sysinfo),
            .method = HTTP_GET,
            .handler = sysinfo_get_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &sysinfo_get_uri);
    }

    ///////////////////////////////////////////////////////////////////////////////////

    /* URI handler for getting web server files */
    {
        const httpd_uri_t file_get_uri = {
            .uri = "/*",
            .method = HTTP_GET,
            .handler = file_get_handler,
            .user_ctx = rest_context
        };
        httpd_register_uri_handler(server, &file_get_uri);
    }
}