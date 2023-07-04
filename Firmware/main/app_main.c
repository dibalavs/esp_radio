/*
  KaRadio 32
  A WiFi webradio player

Copyright (C) 2017  KaraWin

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "action_manager.h"
#include "app_state.h"
#include "freertos/portmacro.h"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <nvs.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_sleep.h"
//#include "esp_heap_trace.h"
#include "nvs_flash.h"
#include "driver/i2s.h"
#include "driver/uart.h"

#include "mdns.h"
#include <merus.h>
#include <rda5807.h>

#include "app_main.h"

//#include "spiram_fifo.h"
#include "audio_renderer.h"
//#include "bt_speaker.h"
#include "bt_config.h"
//#include "mdns_task.h"f
#include "audio_player.h"
#include <u8g2.h>
#include "u8g2_esp32_hal.h"
#include "addon.h"
#include "addonu8g2.h"
#include "buttons.h"
#include "eeprom.h"

/////////////////////////////////////////////////////
///////////////////////////
#include "bt_config.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "nvs_flash.h"
#include "gpio.h"
#include "servers.h"
#include "webclient.h"
#include "webserver.h"
#include "interface.h"
#include "vs1053.h"
#include "addon.h"
#include "esp_idf_version.h"
#include "driver/gptimer.h"
#include "ext_gpio.h"
#include "network.h"
#include "i2s_redirector.h"

#include "debug_task_stats.h"

#include <bus.h>

#define TAG "main"

//Priorities of the reader and the decoder thread. bigger number = higher prio
#define PRIO_READER configMAX_PRIORITIES -3
#define PRIO_MQTT configMAX_PRIORITIES - 3
#define PRIO_CONNECT configMAX_PRIORITIES -1
#define striWATERMARK  "watermark: %d  heap: %d"

void start_network();
void autoPlay(bool is_ap);
/* */
QueueHandle_t event_queue;

//xSemaphoreHandle print_mux;
bool logTel; // true = log also on telnet
player_t *player_config;
// timeout to save volume in flash
//static uint32_t ctimeVol = 0;
static uint32_t ctimeMs = 0;
static bool divide = false;

gptimer_handle_t mstimer = NULL;
gptimer_handle_t sleeptimer = NULL;
gptimer_handle_t waketimer = NULL;

IRAM_ATTR void app_no_interrupt_1ms() {}
// enable 1MS timer interrupt
IRAM_ATTR void app_interrupt_1ms() {}

static bool msCallback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t event_qu = (QueueHandle_t)user_ctx;
    queue_event_t evt;
    evt.type = TIMER_1MS;
    evt.i1 = 0;
    evt.i2 = 0;
    xQueueSendFromISR(event_qu, &evt, NULL);
    return high_task_awoken == pdTRUE;
}

static bool sleepCallback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_awoken = pdFALSE;
    gptimer_stop(timer);
    QueueHandle_t event_qu = (QueueHandle_t)user_ctx;
    queue_event_t evt;
    evt.type = TIMER_SLEEP;
    evt.i1 = 0;
    evt.i2 = 0;
    xQueueSendFromISR(event_qu, &evt, NULL);
    return high_task_awoken == pdTRUE;
}

static bool wakeCallback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_awoken = pdFALSE;
    gptimer_stop(timer);
    QueueHandle_t event_qu = (QueueHandle_t)user_ctx;
    queue_event_t evt;
    evt.type = TIMER_WAKE;
    evt.i1 = 0;
    evt.i2 = 0;
    xQueueSendFromISR(event_qu, &evt, NULL);
    return high_task_awoken == pdTRUE;
}

uint64_t app_get_sleep()
{
    uint64_t ret=0;
    ESP_ERROR_CHECK(gptimer_get_raw_count(sleeptimer,&ret));
    ESP_LOGD(TAG,"getSleep: ret: %lld",ret);
    return ret/10000ll;
}

uint64_t app_get_wake()
{
    uint64_t ret=0;
    ESP_ERROR_CHECK(gptimer_get_raw_count(waketimer,&ret));
    ESP_LOGD(TAG,"getWake: ret: %lld",ret);
    return ret/10000ll;
}

void tsocket(const char* lab, uint32_t cnt)
{
        char* title = kmalloc(strlen(lab)+50);
        sprintf(title,"{\"%s\":\"%d\"}",lab,cnt*60);
        websocket_broadcast(title, strlen(title));
        free(title);
}

void app_stop_sleep(){
    ESP_LOGD(TAG,"stopSleep");
    ESP_ERROR_CHECK(gptimer_stop(sleeptimer));
    tsocket("lsleep",0);
}
gptimer_event_callbacks_t cbss = {
        .on_alarm = sleepCallback, // register user callback
};
gptimer_alarm_config_t  alarm_config = {
    .alarm_count = 0,
    .flags.auto_reload_on_alarm = false, // enable auto-reload
};
void app_start_sleep(uint32_t delay){
    ESP_LOGD(TAG,"startSleep: %d min.",delay );
    ESP_ERROR_CHECK(gptimer_stop(sleeptimer));
    vTaskDelay(1);
    if (delay == 0) return;
    ESP_ERROR_CHECK(gptimer_set_raw_count(sleeptimer,delay*600000));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(sleeptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(sleeptimer, &cbss, event_queue));
    ESP_ERROR_CHECK(gptimer_start(sleeptimer));
    tsocket("lsleep",delay);
}
void app_stop_wake(){
    ESP_LOGD(TAG,"stopWake");
    ESP_ERROR_CHECK(gptimer_stop(waketimer));
    tsocket("lwake",0);
}
gptimer_event_callbacks_t cbsw = {
        .on_alarm = wakeCallback, // register user callback
};
void app_start_wake(uint32_t delay){
    ESP_LOGD(TAG,"startWake: %d min.",delay );
    ESP_ERROR_CHECK(gptimer_stop(waketimer));
    vTaskDelay(1);
    if (delay == 0) return;
    ESP_ERROR_CHECK(gptimer_set_raw_count(waketimer,delay*600000ll));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(waketimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(waketimer, &cbsw, event_queue));
    ESP_ERROR_CHECK(gptimer_start(waketimer));
    tsocket("lwake",delay);
}

void initTimers()
{
    event_queue = xQueueCreate(10, sizeof(queue_event_t));
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_APB,
        .direction = GPTIMER_COUNT_DOWN,
        .resolution_hz = 10 * 1000 , //  1 tick = 100µs
    };
    gptimer_alarm_config_t  alarm_config = {
        .reload_count = 5, // counter will reload with 0 on alarm event
        .alarm_count = 0, // period = 500µs
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };

    /*Configure timer 1MS*/
    //////////////////////
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &mstimer));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(mstimer, &alarm_config));
    gptimer_event_callbacks_t cbs = {
        .on_alarm = msCallback, // register user callback
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(mstimer, &cbs, event_queue));
    ESP_ERROR_CHECK(gptimer_enable(mstimer));
    ESP_ERROR_CHECK(gptimer_start(mstimer));


    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &sleeptimer));
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &waketimer));
}

//////////////////////////////////////////////////////////////////


// Renderer config creation
static renderer_config_t *create_renderer_config()
{
    renderer_config_t *renderer_config = kcalloc(1, sizeof(renderer_config_t));

    if(renderer_config->output_mode == I2S_MERUS) {
        renderer_config->bit_depth = I2S_BITS_PER_SAMPLE_32BIT;
    }

    if(renderer_config->output_mode == DAC_BUILT_IN) {
        renderer_config->bit_depth = I2S_BITS_PER_SAMPLE_16BIT;
    }

    return renderer_config;
}


/******************************************************************************
 * FunctionName : checkUart
 * Description  : Check for a valid uart baudrate
 * Parameters   : baud
 * Returns      : baud
*******************************************************************************/
uint32_t iface_check_uart(uint32_t speed)
{
    uint32_t valid[] = {1200,2400,4800,9600,14400,19200,28800,38400,57600,76880,115200,230400};
    int i ;
    for (i=0;i<12;i++){
        if (speed == valid[i]) return speed;
    }
    return 115200; // default
}


/******************************************************************************
 * FunctionName : init_hardware
 * Description  : Init all hardware, partitions etc
 * Parameters   :
 * Returns      :
*******************************************************************************/
static void init_hardware()
{
    bus_init_gpio();
    bus_init_spi();
    bus_init_i2c();
	bus_init_i2s();

    ext_gpio_init();
    buttons_init();

    i2s_redirector_init();

    ESP_LOGI(TAG, "hardware initialized");
}

//blinking led and timer isr
IRAM_ATTR void timer_task(void* p) {
//	struct device_settings *device;
    queue_event_t evt;
        // queue for events of the sleep / wake and Ms timers

    initTimers();


    while(1) {
        // read and treat the timer queue events
//		int nb = uxQueueMessagesWaiting(event_queue);
//		if (nb >29) printf(" %d\n",nb);
        while (xQueueReceive(event_queue, &evt, portMAX_DELAY))
        {
            if (evt.type != TIMER_1MS) printf("evt.type: %d\n",evt.type);
            switch (evt.type){
                    case TIMER_1MS:
                        //if (serviceAddon != NULL) serviceAddon(); // for the encoders and buttons
                        if (divide)
                            ctimeMs++;	// for led
                        divide = !divide;
                        addon_service_isr();
                    break;
                    case TIMER_SLEEP:
                        action_stop(); // stop the player
                    break;
                    case TIMER_WAKE:
                        webclient_connect(); // start the player
                    break;
                    default:
                    break;
            }
        }

        vTaskDelay(1);
    }
//	printf("t0 end\n");
    vTaskDelete( NULL ); // stop the task (never reached)
}

void uartInterfaceTask(void *pvParameters) {
    char tmp[255];
    int d;
    uint8_t c;
    int t ;
    esp_err_t err;
//	struct device_settings *device;
    uint32_t uspeed;
    int uxHighWaterMark;

    uspeed = g_device->uartspeed;
   uart_config_t uart_config0 = {
        .baud_rate = uspeed,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,    //UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 0,
    };
    err = uart_param_config(UART_NUM_0, &uart_config0);
    if (err!=ESP_OK) ESP_LOGE("uartInterfaceTask","uart_param_config err: %d",err);

    err = uart_driver_install(UART_NUM_0, 1024 , 0, 0, NULL, 0);
    if (err!=ESP_OK)
    {
        ESP_LOGE("uartInterfaceTask","uart_driver_install err: %d",err);
        vTaskDelete(NULL);
    }

    for(t = 0; t<sizeof(tmp); t++) tmp[t] = 0;
    t = 0;

    while(1) {
        while(1) {
            d= uart_read_bytes(UART_NUM_0, &c, 1, 100);
            if (d>0)
            {
                if((char)c == '\r') break;
                if((char)c == '\n') break;
                tmp[t] = (char)c;
                t++;
                if(t == sizeof(tmp)-1) t = 0;
            }
            //else printf("uart d: %d, T= %d\n",d,t);
            //switchCommand() ;  // hardware panel of command
        }
        iface_check_command(t, tmp);
        uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
        ESP_LOGD("uartInterfaceTask",striWATERMARK,uxHighWaterMark,xPortGetFreeHeapSize( ));

        for(t = 0; t<sizeof(tmp); t++) tmp[t] = 0;
        t = 0;
    }
}

// In STA mode start a station or start in pause mode.
// Show ip on AP mode.
void autoPlay(bool is_ap)
{
    char apmode[50];
    sprintf(apmode,"at IP %s",app_get_ip());
    if (is_ap) {
        webclient_save_one_header("Configure the AP with the web page",34,METANAME);
        webclient_save_one_header(apmode,strlen(apmode),METAGENRE);
        return;
    }

    webclient_save_one_header(apmode,strlen(apmode),METANAME);

    if (g_device->autostart && action_getstation() != NO_STATION) {
        kprintf("autostart: playing:%d, currentstation:%d\n", g_device->autostart, action_getstation());
        action_switch(0);
    } else
        webclient_save_one_header("Ready",5,METANAME);
}

static bool is_mdns_init = false;
static void on_wifi_connected(bool is_ap)
{
    static TaskHandle_t task_webclient;
    static TaskHandle_t task_webserver;

    if (!is_mdns_init) {
        is_mdns_init = true;
        ESP_ERROR_CHECK(mdns_init());
        ESP_LOGI(TAG,"mDNS Hostname: %s",g_device->hostname );
        ESP_ERROR_CHECK(mdns_hostname_set(g_device->hostname));
        ESP_ERROR_CHECK(mdns_instance_name_set(g_device->hostname));
        ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
        ESP_ERROR_CHECK(mdns_service_add(NULL, "_telnet", "_tcp", 23, NULL, 0));
    }

    if (!task_webclient) {
        xTaskCreatePinnedToCore(webclient_task, "clientTask", 3700, NULL, PRIO_CLIENT, &task_webclient,CPU_CLIENT);
        ESP_LOGI(TAG, "%s task: %p","clientTask",(void *)task_webclient);
    }

    if (!task_webserver) {
        xTaskCreatePinnedToCore(servers_task, "serversTask", 3100, NULL, PRIO_SERVER, &task_webserver,CPU_SERVER);
        ESP_LOGI(TAG, "%s task: %p","serversTask",(void *)task_webserver);
    }

    webclient_save_one_header("Wifi Connected.",18,METANAME);
    autoPlay(is_ap);
}

static void on_wifi_disconnected(bool is_ap)
{
    if (is_mdns_init)
        mdns_free();
    is_mdns_init = false;
    webclient_silent_disconnect();
    webclient_save_one_header("Wifi Disconnected.",18,METANAME);
}

/**
 * Main entry point
 */
void app_main()
{
    uint32_t uspeed;
    TaskHandle_t pxCreatedTask;
    esp_err_t err;

    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI(TAG, "starting app_main()");
    ESP_LOGI(TAG, "RAM left: %u", esp_get_free_heap_size());

    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);
    // Initialize NVS.
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    app_state_init();
    g_device = app_state_get_settings();

    // log on telnet
    if (g_device->options & T_LOGTEL)
        logTel = true; //
    else
        logTel = false; //

    // init softwares
    telnet_init();
    websocket_init();

    //time display
    uint8_t ddmm;
    option_get_ddmm(&ddmm);
    iface_set_ddmm(ddmm?1:0);

    init_hardware();

    // log level
    //iface_set_log_level(g_device->trace_level);

    // output mode
    //I2S, I2S_MERUS, DAC_BUILT_IN, PDM, VS1053
    ESP_LOGI(TAG, "audio_output_mode %d\nOne of I2S=0, I2S_MERUS, DAC_BUILT_IN, PDM, VS1053, SPDIF",app_state_get_audio_output_mode());

    //uart speed
    uspeed = g_device->uartspeed;
    uspeed = iface_check_uart(uspeed);
    uart_set_baudrate(UART_NUM_0, uspeed);
    ESP_LOGI(TAG, "Set baudrate at %d",uspeed);

    // Version infos
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, "Project name: %s",esp_app_get_description()->project_name);
    ESP_LOGI(TAG, "Version: %s",esp_app_get_description()->version);
    ESP_LOGI(TAG, "Release %s, Revision %s",RELEASE,REVISION);
//	ESP_LOGI(TAG, "Date: %s,  Time: %s",esp_app_get_description()->date,esp_app_get_description()->time);
    ESP_LOGI(TAG, "SDK %s",esp_get_idf_version());
    ESP_LOGI(TAG, " Date %s, Time: %s\n", __DATE__,__TIME__ );
    ESP_LOGI(TAG, "Heap size: %d",xPortGetFreeHeapSize());

    // lcd init
    uint8_t rt;
    option_get_lcd_info(&g_device->lcd_type,&rt);
    ESP_LOGI(TAG,"LCD Type %d",g_device->lcd_type);
    //lcd rotation
    iface_set_rotat(rt) ;
    addon_lcd_init(g_device->lcd_type);
    ESP_LOGI(TAG, "Hardware init done...");

    addon_lcd_welcome("","");
    addon_lcd_welcome("","STARTING");

    ESP_LOGI(TAG, "Volume set to %d",app_state_get_ivol());

    xTaskCreatePinnedToCore(timer_task, "timerTask",2100, NULL, PRIO_TIMER, &pxCreatedTask,CPU_TIMER);
    ESP_LOGI(TAG, "%s task: %x","t0",(unsigned int)pxCreatedTask);

    //xTaskCreatePinnedToCore(uartInterfaceTask, "uartInterfaceTask", 2500, NULL, PRIO_UART, &pxCreatedTask,CPU_UART);
    //ESP_LOGI(TAG, "%s task: %x","uartInterfaceTask",(unsigned int)pxCreatedTask);

    webclient_init();
    // init player config
    player_config = (player_t*)kcalloc(1, sizeof(player_t));
    player_config->command = CMD_NONE;
    player_config->decoder_status = UNINITIALIZED;
    player_config->decoder_command = CMD_NONE;
    player_config->buffer_pref = BUF_PREF_SAFE;
    player_config->media_stream = kcalloc(1, sizeof(media_stream_t));
    audio_player_init(player_config);
    renderer_init(create_renderer_config());

//-----------------------------
// start the network
//-----------------------------
    /* Force enable AP if 'play' button is pressed during boot. */
    bool is_ap = ext_gpio_get_button_play();
    ESP_LOGE(TAG, "is_ap: %d, ssid:%s", is_ap, g_device->ssid1);

    if (g_device->ssid1[0] == '\0')
        is_ap = true;

    network_set_cb(on_wifi_connected, on_wifi_disconnected);
    wifi_ssid_t ssid;
    if (is_ap) {
        strcpy((char *)ssid.ssid, "esp_radio");
        ssid.password[0] = '\0';
        network_init(NULL, &ssid, true);
    } else {
        strcpy((char *)ssid.ssid, g_device->ssid1);
        strcpy((char *)ssid.password, g_device->pass1);
        network_init(&ssid, NULL, false);
    }

    if (g_device->autostart && action_getstation() != NO_STATION)
        action_power_on();

//-----------------------------------------------------
//init softwares
//-----------------------------------------------------

    // LCD Display infos
    addon_lcd_welcome(app_get_ip(),"STARTED");
    //start tasks of KaRadio32
    xTaskCreatePinnedToCore (addon_task, "task_addon", 2200, NULL, PRIO_ADDON, &pxCreatedTask,CPU_ADDON);
    ESP_LOGI(TAG, "%s task: %x","task_addon",(unsigned int)pxCreatedTask);
    ESP_LOGI(TAG," Init Done");

    kprintf("READY. Type help for a list of commands\n");
    // error log on telnet
    esp_log_set_vprintf( (vprintf_like_t)lkprintf);

    // Uncomment it to enable printing task stats.
    // Need configUSE_TRACE_FACILITY and CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS option in menuconfig
    // debug_task_stat_init();

    // All done.
}
