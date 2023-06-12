/**
 * @file debug_task_stats.h
 * @author Vasily Dybala (dibalavs@yandex.ru)
 * @brief Show task cpu consuptions. (like 'top' tool).
 * @version 0.1
 * @date 2023-05-14
 *
 * @copyright Copyright (c) 2023
 *
 */

 #pragma once

// Need to enable in menuconfig:
// - configUSE_TRACE_FACILITY
// - CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
// - configTASKLIST_INCLUDE_COREID
void debug_task_stat_init(void);