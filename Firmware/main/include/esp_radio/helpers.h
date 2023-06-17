/**
 * @file helpers.h
 * @author Vasily Dybala
 * @brief Helper functions
 * @version 0.1
 * @date 2023-06-15
 *
 * @copyright Copyright (c) 2023
 *
 */

 #define CLIP(val, min, max) ({ \
    typeof(val) res = (val); \
    res < (min) ? (min) : \
    res > (max) ? (max) : res; \
 })

 #define CLIP_VOLUME(val) CLIP(val, 0, 255)