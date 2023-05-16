/**
 * @file kmalloc.h
 * @author Vasily Dybala (dibalavs@yandex.ru)
 * @brief SPI based allocator
 * @version 0.1
 * @date 2023-05-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <stdint.h>

void* kmalloc(size_t size);
void* kcalloc(size_t nmemb, size_t size);
