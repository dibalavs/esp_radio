#include "buttons.h"

#include <esp_log.h>
#include <ext_gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>

#include "app_main.h" // kalloc
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

#define TAG "button"

// Rotary encoder table was taken from:
// https://github.com/brianlow/Rotary/blob/master/Rotary.cpp

#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Counter-clockwise step.
#define DIR_CCW 0x20
#define R_START 0x0

#ifdef HALF_STEP
// Use the half-step state table (emits a code at 00 and 11)
#define R_CCW_BEGIN 0x1
#define R_CW_BEGIN 0x2
#define R_START_M 0x3
#define R_CW_BEGIN_M 0x4
#define R_CCW_BEGIN_M 0x5
static const unsigned char ttable[6][4] = {
  // R_START (00)
  {R_START_M,            R_CW_BEGIN,     R_CCW_BEGIN,  R_START},
  // R_CCW_BEGIN
  {R_START_M | DIR_CCW, R_START,        R_CCW_BEGIN,  R_START},
  // R_CW_BEGIN
  {R_START_M | DIR_CW,  R_CW_BEGIN,     R_START,      R_START},
  // R_START_M (11)
  {R_START_M,            R_CCW_BEGIN_M,  R_CW_BEGIN_M, R_START},
  // R_CW_BEGIN_M
  {R_START_M,            R_START_M,      R_CW_BEGIN_M, R_START | DIR_CW},
  // R_CCW_BEGIN_M
  {R_START_M,            R_CCW_BEGIN_M,  R_START_M,    R_START | DIR_CCW},
};
#else
// Use the full-step state table (emits a code at 00 only)
#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6

static const unsigned char ttable[7][4] = {
  // R_START
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  // R_CW_FINAL
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  // R_CW_BEGIN
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  // R_CW_NEXT
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  // R_CCW_BEGIN
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  // R_CCW_FINAL
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  // R_CCW_NEXT
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};
#endif

static void enqueue_event(enum button_type button, enum button_state state)
{
    if (released_events == NULL) {
        ESP_LOGE(TAG, "Unable to enqueue kbd event. Queue is empty.");
        return;
    }

    struct button_event_queue *new_ev = released_events;
    released_events = released_events->next;

    new_ev->event.button = button;
    new_ev->event.state = state;

    new_ev->next = event_queue;
    event_queue = new_ev;
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
    encoder_state = R_START;
    encoder_value = 0;
}

static void process_button(enum button_type button, bool is_pressed, unsigned long now_ms)
{
    const unsigned long tm = button_times[button];

    // Pressed
    if (button && tm == 0) {
        button_times[button] = now_ms;
        enqueue_event(button, BTN_STATE_PRESSED);
    }

    // Released
    if (!button && tm != 0) {
        enqueue_event(button, BTN_STATE_RELEASED);
        if (tm - now_ms > BTN_HOLDTIME_MS)
            enqueue_event(button, BTN_STATE_HOLD);
        else
            enqueue_event(button, BTN_STATE_CLICKED);
        button_times[button] = 0;
    }
}

// ----------------------------------------------------------------------------
// call this every 1 millisecond via timer ISR
//
IRAM_ATTR void buttons_service(void)
{
    const unsigned long now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // Process buttons
    process_button(BTN_TYPE_PREV, ext_gpio_get_button_prev(), now_ms);
    process_button(BTN_TYPE_PLAY, ext_gpio_get_button_play(), now_ms);
    process_button(BTN_TYPE_NEXT, ext_gpio_get_button_next(), now_ms);
    process_button(BTN_TYPE_ENC_BTN, ext_gpio_get_enc_button(), now_ms);

    // Process rotary encoder
    const uint8_t pins = (ext_gpio_get_enca() << 1) | ext_gpio_get_encb();
    encoder_state = ttable[encoder_state & 0xf][pins];
    switch (encoder_state & 0x30) {
    case DIR_CCW:
        if (encoder_value > 0) {
            encoder_value--;
            enqueue_event(BTN_TYPE_ENC_LESS, BTN_STATE_CLICKED);
        }
        break;
    case DIR_CW:
        if (encoder_value < 255) {
            encoder_value++;
            enqueue_event(BTN_TYPE_ENC_MORE, BTN_STATE_CLICKED);
        }
        break;
    }
}

button_event_t * buttons_get_event()
{
    if (event_queue == NULL)
        return NULL;

    button_event_t *res = &event_queue->event;
    event_queue = event_queue->next;

    return res;
}

void buttons_release_event(button_event_t *evt)
{
    // TODO: check that offset_of(button_event_queue::event) == 0;
    struct button_event_queue *queue = (struct button_event_queue *)evt;
    queue->next = released_events;
    released_events = queue;
}

uint8_t buttons_get_encoder_value(void)
{
    return encoder_value;
}