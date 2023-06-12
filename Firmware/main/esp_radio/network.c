/**
 * @file network.c
 * @author Vasily Dybala (dibalavs@yandex.ru)
 * @brief Network management functions
 * @version 0.1
 * @date 2023-05-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "esp_netif_types.h"
#include "lwip/ip4_addr.h"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "network.h"
#include "esp_err.h"
#include "esp_wifi_types.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>

#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/api.h>
#include <lwip/tcp.h>
#include <lwip/dns.h>

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <string.h>

static const char *TAG = "net";

const int CONNECTED_BIT = 0x00000001;
const int CONNECTED_AP  = 0x00000010;

#define SSIDLEN		32
#define PASSLEN		64
#define HOSTLEN		24

static esp_netif_t *net;
static EventGroupHandle_t wifi_event_group;
static netword_cb_t *wifi_connected_cb;
static netword_cb_t *wifi_disconnected_cb;
static bool is_connected = false;
static char ip_str[20];

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
							   int32_t event_id, void *event_data)
{
	esp_netif_ip_info_t ip;
	wifi_event_sta_disconnected_t *dis;
	ip_event_got_ip_t *got_ip;
	const bool is_ap = arg != NULL;

	if (event_base == WIFI_EVENT) {
		switch (event_id)
		{
		case WIFI_EVENT_AP_STACONNECTED:
			ESP_ERROR_CHECK(esp_netif_get_ip_info(net, &ip));
			sprintf(ip_str, IPSTR, IP2STR(&ip.ip));
			ESP_LOGI(TAG, "AP set IP address: %s", ip_str);
			is_connected = true;
			if (wifi_connected_cb)
				wifi_connected_cb(is_ap);
			break;
		case WIFI_EVENT_AP_STADISCONNECTED:
			ESP_LOGE(TAG, "AP was disconnected");
			is_connected = false;
			if (wifi_disconnected_cb)
				wifi_disconnected_cb(is_ap);
			break;
		case WIFI_EVENT_STA_START:
			ESP_LOGI(TAG, "WIFI started");
			ESP_ERROR_CHECK(esp_wifi_connect());
			break;
		case WIFI_EVENT_STA_CONNECTED:
			xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
			ESP_LOGI(TAG, "wifi connected");
			break;
		case WIFI_EVENT_STA_DISCONNECTED:
			xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
			dis = (wifi_event_sta_disconnected_t *)event_data;
			ESP_LOGE(TAG, "Wifi Disconnected. reason:%d", (int)dis->reason);
			is_connected = false;
			if (wifi_disconnected_cb)
				wifi_disconnected_cb(is_ap);
			ESP_ERROR_CHECK(esp_wifi_connect());
			break;
		case WIFI_EVENT_AP_START:
			xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
			ESP_LOGI(TAG, "AP started.");
			break;
		default:
			break;
		}
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        got_ip = (ip_event_got_ip_t *)event_data;
        sprintf(ip_str, IPSTR, IP2STR(&got_ip->ip_info.ip));
        ESP_LOGI(TAG, "Wifi connected. got IP address: %s", ip_str);
		is_connected = true;
        if (wifi_connected_cb)
            wifi_connected_cb(is_ap);
	}
}

void network_set_cb(netword_cb_t *on_connected, netword_cb_t *on_disconnected)
{
	wifi_connected_cb = on_connected;
	wifi_disconnected_cb = on_disconnected;
}

// private function.
void adc_power_acquire(void);

void network_init(const wifi_ssid_t *cfg_sta, const wifi_ssid_t *cfg_ap, bool is_ap)
{
	wifi_mode_t mode;
	wifi_interface_t iface;

    ESP_LOGI(TAG, "Starting wifi");

	// hack to remove interrupt glitches on GPIO36.
	// see:
	// - https://github.com/espressif/esp-idf/issues/4585
	// - https://github.com/espressif/esp-idf/commit/d890a516a1097f0a07788e203fdb1a82bb83520e
	adc_power_acquire();

    if (!cfg_sta || cfg_sta->ssid[0] == '\0')
        is_ap = true;

    esp_netif_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    if (is_ap) {
        net = esp_netif_create_default_wifi_ap();
    } else {
        net = esp_netif_create_default_wifi_sta();
    }
    assert(net);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	void *arg = is_ap ? (void *)1 : NULL;
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, arg));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, arg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	wifi_config_t wifi_config = {};

	if (is_ap) {
		mode = WIFI_MODE_AP;
		iface = WIFI_IF_AP;
		wifi_config.ap =  (wifi_ap_config_t) {
			.authmode = WIFI_AUTH_OPEN,
			.max_connection = 2,
			.beacon_interval = 100
		};

		strcpy((char *)wifi_config.ap.ssid, (char *)cfg_ap->ssid);

		ESP_LOGI(TAG, "WiFi AP is open. SSID:%s", wifi_config.ap.ssid);
	} else {
		mode = WIFI_MODE_STA;
		iface = WIFI_IF_STA;
		wifi_config.sta =  (wifi_sta_config_t) {
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,
			.sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
		};

		strcpy((char *)wifi_config.sta.ssid, (char *)cfg_sta->ssid);
		strcpy((char *)wifi_config.sta.password, (char *)cfg_sta->password);

		ESP_LOGI(TAG, "Trying to connect SSID:%s", cfg_sta->ssid);
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(mode)) ;
	ESP_ERROR_CHECK(esp_wifi_set_config(iface, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "WiFi init done!");
}

void network_deinit(void)
{
	ESP_ERROR_CHECK(esp_wifi_deinit());
}

const char* app_get_ip()
{
	return ip_str;
}

void network_wait(void)
{
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,false, true, 2000);
}

bool network_is_connected(void)
{
	return is_connected;
}