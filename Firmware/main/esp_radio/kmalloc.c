#include "esp_radio/kmalloc.h"

#include <esp_heap_caps.h>

static enum {
    STATE_UNINIT = 0,
    STATE_BIGRAM = 1,
    STATE_LOWRAM = 2
} state;

static void do_init(void)
{
    size_t spi = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    state = (spi > 0) ? STATE_BIGRAM : STATE_LOWRAM;
}

void* kmalloc(size_t bytes)
{
    if (state == STATE_BIGRAM)
        return heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (state == STATE_LOWRAM)
        return heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL  | MALLOC_CAP_8BIT);

    do_init();
    return kmalloc(bytes);
}

void* kcalloc(size_t ncnt, size_t nsize)
{
    if (state == STATE_BIGRAM)
        return heap_caps_calloc(ncnt,nsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (state == STATE_LOWRAM)
        return heap_caps_calloc(ncnt,nsize, MALLOC_CAP_INTERNAL  | MALLOC_CAP_8BIT);

    do_init();
    return kcalloc(ncnt, nsize);
}

bool app_big_sram(void)
{
    return state == STATE_BIGRAM;
}