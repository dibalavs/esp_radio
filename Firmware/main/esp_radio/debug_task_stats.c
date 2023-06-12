/**
 * @file debug_task_stats.c
 * @author Vasily Dybala (dibalavs@yandex.ru)
 * @brief Show task cpu consuptions. (like 'top' tool).
 * @version 0.1
 * @date 2023-05-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "debug_task_stats.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_log.h>

#include "freertos/portmacro.h"
#include "freertos_err.h"
#include "kmalloc.h"

#define STATS_TICKS pdMS_TO_TICKS(5000)
#define MAX_TASKS   30

static const char *TAG = "stat";

static void print_real_time_stats(const TaskStatus_t *prev, UBaseType_t prev_num, TaskStatus_t *curr, UBaseType_t *curr_num, uint32_t *prev_timestamp)
{
    uint32_t curr_timestamp = 0;

    *curr_num = uxTaskGetNumberOfTasks();
    if (*curr_num > MAX_TASKS) {
        ESP_LOGE(TAG, "TOO many tasks (%d). truncate to :%d\n", *curr_num, MAX_TASKS);
        *curr_num = MAX_TASKS;
    }

    *curr_num = uxTaskGetSystemState(curr, *curr_num, &curr_timestamp);
    if (*curr_num == 0) {
        ESP_LOGE(TAG, "No active tasks.");
        return;
    }

    uint32_t total_elapsed_time = (curr_timestamp - *prev_timestamp);
    *prev_timestamp = curr_timestamp;
    if (total_elapsed_time == 0) {
        ESP_LOGE(TAG, "No elapsed time. curr time:%d", curr_timestamp);
        return;
    }

    uint32_t all = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    uint32_t iram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    uint32_t psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    ESP_LOGE(TAG, "");
    ESP_LOGE(TAG, "Total free memory/ IRAM / PSRAM = %u / %u / %u", all, iram, psram);
    ESP_LOGE(TAG, "| %-18s | Core |  Run Time  | Percentage | Stack usage\n", "Task");
    for (int c = 0; c < *curr_num; c++) {
        const TaskStatus_t *found = NULL;

        for (int p = 0; p < prev_num; p++) {
            if (curr[c].xHandle == prev[p].xHandle) {
                found = &prev[p];
                break;
            }
        }

        if (found) {
            uint32_t task_elapsed_time = curr[c].ulRunTimeCounter - found->ulRunTimeCounter;
            uint32_t percentage_time = (task_elapsed_time * 100UL) / (total_elapsed_time * portNUM_PROCESSORS);
            uint32_t stack = curr[c].usStackHighWaterMark;
            ESP_LOGE(TAG, "| %-18s |    %d | %10u | %9u%% | %8u\n", curr[c].pcTaskName, (int)curr[c].xCoreID, task_elapsed_time, percentage_time, stack);
        }
    }
}

#define SWAP(a, b) \
{ \
  typeof(a) tmp = a ; \
  a = b; \
  b = tmp; \
}

static void stats_task(void *arg)
{
    TaskStatus_t *stat1 = kcalloc(MAX_TASKS, sizeof(TaskStatus_t));
    TaskStatus_t *stat2 = kcalloc(MAX_TASKS, sizeof(TaskStatus_t));
    UBaseType_t stat1_numtasks = 0;
    UBaseType_t stat2_numtasks = 0;
    uint32_t timestamp = 0;
    assert(stat1);
    assert(stat2);

    while (1) {
        vTaskDelay(STATS_TICKS);
        print_real_time_stats(stat1, stat1_numtasks, stat2, &stat2_numtasks, &timestamp);
        SWAP(stat1, stat2);
        SWAP(stat1_numtasks, stat2_numtasks);
    }
}

void debug_task_stat_init(void)
{
    FREERTOS_ERROR_CHECK(xTaskCreatePinnedToCore(stats_task, "stats", 2096, NULL, 1, NULL, tskNO_AFFINITY));
}