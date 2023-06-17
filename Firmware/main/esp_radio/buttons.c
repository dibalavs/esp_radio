#include "buttons.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <esp_log.h>
#include <ext_gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <sys/time.h>
#include <esp_err.h>

#include "app_main.h" // kalloc
#include "freertos_err.h"

// ----------------------------------------------------------------------------

#define EVENT_QUEUE_LEN 30

struct button_event_queue {
    button_event_t event;
    struct button_event_queue *next;
};

// TODO: make fifo instead of lifo.
static struct button_event_queue *event_queue;
static struct button_event_queue *released_events;
static unsigned long *button_times; //array of [BTN_TYPE_LAST] elements
static unsigned char encoder_state = 0;
static unsigned char encoder_value = 0;
static unsigned long encoder_ccw_ms = 0;
static unsigned long encoder_cw_ms = 0;

static SemaphoreHandle_t lock;
static SemaphoreHandle_t task_notify;
static TaskHandle_t button_task_handle;
static buttons_cb_t *buttons_callback;

#define TAG "button"

// Idea was taken from:
// https://github.com/brianlow/Rotary/blob/master/Rotary.cpp

// current values:
// CW:
// single step turn:    '0' -> '3' -> '1' -> '0'
// multiple steps turn: '0' -> '3' -> '0'

// CCW:
// single step turn:    '0' -> '1' -> '3' -> '0'
// multiple steps turn: '0' -> '1' -> '0'

// Clockwise step.
#define DIR_CW 0x10
// Counter-clockwise step.
#define DIR_CCW 0x20

static const unsigned char ttable[4][4] = {
    // Common start
    {0, 1, 0, 3},

    // CCW direction states
    {DIR_CCW, 1, 1, 1},

    // Unused state.
    {0, 0, 0, 0},

    // CW direction states
    {DIR_CW, 3, 0, 3}
};

IRAM_ATTR static void ext_gpio_int_callback_isr(void)
{
    portBASE_TYPE woken = pdFALSE;
    vTaskNotifyGiveFromISR(button_task_handle, &woken);
    if (woken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void enqueue_event(enum button_type button, enum button_state state, uint8_t inc)
{
    struct button_event_queue *new_ev;

    if (released_events == NULL) {
        ESP_LOGE(TAG, "Unable to enqueue kbd event. Queue full.");
        return;
    }

    FREERTOS_ERROR_CHECK(xSemaphoreTake(lock, portMAX_DELAY));
    new_ev = released_events;
    released_events = released_events->next;

    new_ev->event.button = button;
    if (button == BTN_TYPE_ENC_LESS || button == BTN_TYPE_ENC_MORE)
        new_ev->event.increment = inc;
    else
        new_ev->event.state = state;

    new_ev->next = event_queue;
    event_queue = new_ev;

    FREERTOS_ERROR_CHECK(xSemaphoreGive(lock));

    if (buttons_callback)
        buttons_callback();
}

static void process_button(enum button_type button, bool is_pressed, unsigned long now_ms)
{
    const unsigned long press_time = button_times[button];

    // Pressed
    if (is_pressed && press_time == 0) {
        button_times[button] = now_ms;
        enqueue_event(button, BTN_STATE_PRESSED, 0);
    }

    // Released
    if (!is_pressed && press_time != 0) {
        enqueue_event(button, BTN_STATE_RELEASED, 0);
        if (now_ms - press_time > BTN_HOLDTIME_MS)
            enqueue_event(button, BTN_STATE_HOLD, 0);
        else
            enqueue_event(button, BTN_STATE_CLICKED, 0);
        button_times[button] = 0;
    }
}

static uint8_t calc_acceleration(unsigned long time_diff_ms)
{
    if (time_diff_ms < 30)
        return 5;

    if (time_diff_ms < 60)
        return 4;

    if (time_diff_ms < 150)
        return 3;

    if (time_diff_ms < 300)
        return 2;

    return 1;
}

static void process_encoder(uint8_t pins, unsigned long now_ms)
{
    int adder = 0;

    encoder_state = ttable[encoder_state & 0xf][pins];
    if ((encoder_state & (DIR_CW | DIR_CCW)) == 0) {
        return;
    }

    if (encoder_state == DIR_CCW ) {
        adder = -calc_acceleration(now_ms - encoder_ccw_ms);
        enqueue_event(BTN_TYPE_ENC_LESS, BTN_STATE_CLICKED, -adder);
        encoder_ccw_ms = now_ms;
    }
    else if (encoder_state == DIR_CW) {
        adder = calc_acceleration(now_ms - encoder_cw_ms);
        enqueue_event(BTN_TYPE_ENC_MORE, BTN_STATE_CLICKED, adder);
        encoder_cw_ms = now_ms;
    }

    if (encoder_value + adder > 0 && encoder_value + adder < 255) {
        encoder_value += adder;
    }
}

static void buttons_task(void* p)
{
    (void)p;
    unsigned long now_ms;
    TickType_t delay = portMAX_DELAY;

    while(1)
    {
        ulTaskNotifyTake(0, delay);
        ext_gpio_fetch_int_captured();

        now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

        // Process rotary encoder
        const uint8_t enc_pins = (ext_gpio_get_enca() << 1) | ext_gpio_get_encb();
        process_encoder(enc_pins, now_ms);

        // For checking encoder, we need to poll encoder pins changes as fast as we can,
        // because interrupts can be lost.
        delay = enc_pins ? 0 : portMAX_DELAY;

        // Process buttons
        if (!enc_pins) {
            process_button(BTN_TYPE_PREV, ext_gpio_get_button_prev(), now_ms);
            process_button(BTN_TYPE_PLAY, ext_gpio_get_button_play(), now_ms);
            process_button(BTN_TYPE_NEXT, ext_gpio_get_button_next(), now_ms);
            process_button(BTN_TYPE_ENC_BTN, ext_gpio_get_enc_button(), now_ms);
        }
    }
}

void buttons_init(void)
{
    struct button_event_queue *events = kcalloc(EVENT_QUEUE_LEN, sizeof(struct button_event_queue));
    assert(events);
    for (int i = 1; i < EVENT_QUEUE_LEN; i++) {
        events[i].next = &events[i - 1];
    }

    released_events = &events[EVENT_QUEUE_LEN - 1];
    button_times = kcalloc(BTN_TYPE_LAST, sizeof(unsigned long));
    assert(button_times);
    encoder_state = 0;
    encoder_value = 0;
    lock = xSemaphoreCreateMutex();
    assert(lock);

    task_notify = xSemaphoreCreateBinary();
    assert(task_notify);
    xSemaphoreTake(task_notify, 0);
    ext_gpio_set_int_callback(&ext_gpio_int_callback_isr);
    FREERTOS_ERROR_CHECK(xTaskCreatePinnedToCore(buttons_task, "buttonstask", configMINIMAL_STACK_SIZE + 100, NULL, 9, &button_task_handle, 0));
}

button_event_t * buttons_get_event(void)
{
    if (event_queue == NULL)
        return NULL;

    button_event_t *res;

    FREERTOS_ERROR_CHECK(xSemaphoreTake(lock, portMAX_DELAY));
    res = &event_queue->event;
    event_queue = event_queue->next;
    FREERTOS_ERROR_CHECK(xSemaphoreGive(lock));

    return res;
}

void buttons_release_event(button_event_t *evt)
{
    // TODO: check that offset_of(button_event_queue::event) == 0;
    struct button_event_queue *queue = (struct button_event_queue *)evt;

    FREERTOS_ERROR_CHECK(xSemaphoreTake(lock, portMAX_DELAY));
    queue->next = released_events;
    released_events = queue;
    FREERTOS_ERROR_CHECK(xSemaphoreGive(lock));
}

uint8_t buttons_get_encoder_value(void)
{
    return encoder_value;
}

void buttons_set_callback(buttons_cb_t *cb)
{
    buttons_callback = cb;
}