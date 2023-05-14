/**
 * @file freertos_err.h
 * @author Vasily Dybala (dibalavs@yandex.ru)
 * @brief Helper macros to check return code of FreeRTOS functions.
 * @version 0.1
 * @date 2023-05-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <freertos/projdefs.h>

#include <esp_err.h>

#define FREERTOS_ERROR_CHECK(x) do {                                    \
        BaseType_t err_rc_ = (x);                                       \
        if (unlikely(err_rc_ != pdTRUE)) {                              \
            _esp_error_check_failed(err_rc_, __FILE__, __LINE__,        \
                                    __ASSERT_FUNC, #x);                 \
        }                                                               \
    } while(0)
