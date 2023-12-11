#include "test_i2s_song.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <sys/time.h>
#include <esp_err.h>
#include "esp_log.h"
#include "gpio.h"
#include "math.h"

#define TAG "test_song"

#define EXAMPLE_BUFF_SIZE   8096
#define EXAMPLE_PDM_TX_FREQ_HZ          48000           // I2S PDM TX frequency
#define EXAMPLE_WAVE_AMPLITUDE          (4000.0)        // 1~32767
#define CONST_PI                        (3.1416f)
#define EXAMPLE_SINE_WAVE_LEN(tone)     (uint32_t)((EXAMPLE_PDM_TX_FREQ_HZ / (float)tone) + 0.5) // The sample point number per sine wave to generate the tone
#define EXAMPLE_TONE_LAST_TIME_MS       800
#define EXAMPLE_BYTE_NUM_EVERY_TONE     (EXAMPLE_TONE_LAST_TIME_MS * EXAMPLE_PDM_TX_FREQ_HZ / 1000)

/* The frequency of tones: do, re, mi, fa, so, la, si, in Hz. */
static const uint32_t tone[3][7] = {{262, 294, 330, 349, 392, 440, 494},            // bass
                                    {523, 587, 659, 698, 784, 880, 988},            // alto
                                    {1046, 1175, 1318, 1397, 1568, 1760, 1976}};    // treble

/* Numbered musical notation of 'twinkle twinkle little star' */
static const uint8_t song[28] = {1, 1, 5, 5, 6, 6, 5,
                                 4, 4, 3, 3, 2, 2, 1,
                                 5, 5, 4, 4, 3, 3, 2,
                                 5, 5, 4, 4, 3, 3, 2};
/* Rhythm of 'twinkle twinkle little star', it's repeated in four sections */
static const uint8_t rhythm[7] = {1, 1, 1, 1, 1, 1, 3};

static const char *tone_name[3] = {"bass", "alto", "treble"};

void test_twinkle_twinkle_little_star(void)
{
    int32_t *w_buf = (int32_t *)calloc(1, EXAMPLE_BUFF_SIZE);
    size_t w_bytes = 0;
    uint8_t cnt = 0;            // The current index of the song
    uint8_t tone_select = 0;    // To selecting the tone level
    vTaskDelay(pdMS_TO_TICKS(5000));

    printf("Playing %s `twinkle twinkle little star`\n", tone_name[tone_select]);
    while (1) {
        int tone_point = EXAMPLE_SINE_WAVE_LEN(tone[tone_select][song[cnt]-1]);
        /* Generate the tone buffer */
        for (int i = 0; i < tone_point; i++) {
            w_buf[2 * i] =  (int16_t)((sin(2 * (float)i * CONST_PI / tone_point)) * EXAMPLE_WAVE_AMPLITUDE) << 16;
        }

        for (int tot_bytes = 0; tot_bytes < 2 * EXAMPLE_BYTE_NUM_EVERY_TONE * rhythm[cnt % 7]; tot_bytes += w_bytes) {
            /* Play the tone */
            ESP_ERROR_CHECK(i2s_channel_write(i2s_tx_chan, w_buf, 2 * tone_point * sizeof(int32_t), &w_bytes, portMAX_DELAY));
        }

        cnt++;
        /* If finished playing, switch the tone level */
        if (cnt == sizeof(song)) {
            cnt = 0;
            tone_select++;
            tone_select %= 3;
            printf("Playing %s `twinkle twinkle little star`\n", tone_name[tone_select]);
            vTaskDelay(215);
        }
        /* Gap between the tones */
        vTaskDelay(40);
    }
    free(w_buf);
    vTaskDelete(NULL);
}
