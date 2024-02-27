/**
 * @file drv\inc\ws281x.h
 *
 * Copyright (C) 2024
 *
 * ws281x.h is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author HinsShum hinsshum@qq.com
 *
 * @encoding utf-8
 */
#ifndef __WS281X_H
#define __WS281X_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "device.h"

/*---------- macro ----------*/
#define IOCTL_WS281X_REFRESH                            (IOCTL_USER_START + 0x00)
#define IOCTL_WS281X_GET_INFO                           (IOCTL_USER_START + 0x01)
#define IOCTL_WS281X_CLEAR_DATA                         (IOCTL_USER_START + 0x02)

/*---------- type define ----------*/
union ws281x_data {
    uint8_t level[3];
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } rgb;
    struct {
        uint8_t r;
        uint8_t b;
        uint8_t g;
    } rbg;
    struct {
        uint8_t g;
        uint8_t r;
        uint8_t b;
    } grb;
    struct {
        uint8_t g;
        uint8_t b;
        uint8_t r;
    } gbr;
    struct {
        uint8_t b;
        uint8_t r;
        uint8_t g;
    } brg;
    struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
    } bgr;
};

typedef struct {
    union ws281x_data *data;
    uint32_t numbers;               /*<< numbers of ws281x */
} ws281x_info_t;

typedef struct {
    /**
     * @brief Initialize the board support package code(some peripherals,
     *         e.g. timer and dma).
     * @retval Return the initialize result, success is ture, failure is false.
     */
    bool (*init)(void);
    /**
     * @brief Deinitialize the board support package code.
     * @retval None.
     */
    void (*deinit)(void);
    /**
     * @brief Start DMA to transfer channel compare value for timer.
     * @retval None.
     */
    void (*refresh)(void);
    /**
     * @brief Interrupt handler for DMA.
     * @param irq irq handler pointer.
     * @param args not used.
     * @param length not used.
     * @retval CY_EOK.
     */
    int32_t (*irq_handler)(uint32_t irq, void *args, uint32_t length);
} ws281x_ops_t;

typedef struct {
    enum {
        WS281X_STATE_IDLE,
        WS281X_STATE_BUSY,
    } state;
    ws281x_info_t info;
    ws281x_ops_t ops;
} ws281x_describe_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __WS281X_H */
