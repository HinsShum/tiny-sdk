/**
 * @file drv\inc\lp5868.h
 *
 * Copyright (C) 2024
 *
 * lp5868.h is free software: you can redistribute it and/or modify
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
#ifndef __LP5868_H
#define __LP5868_H

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
#define IOCTL_LP5868_ENABLE                         (IOCTL_USER_START + 0)
#define IOCTL_LP5868_REFRESH                        (IOCTL_USER_START + 1)
#define IOCTL_LP5868_CLEAR                          (IOCTL_USER_START + 2)

/*---------- type define ----------*/
typedef struct {
    uint16_t row;
    uint16_t column;
} lp5868_resolution_t;

typedef struct {
    bool (*init)(void);
    void (*deinit)(void);
    bool (*power_ctl)(bool ctrl);
} lp5868_ops_t;

typedef struct {
    char *bus_name;
    void *bus;
    uint8_t address;
    lp5868_resolution_t resolution;
    lp5868_ops_t ops;
} lp5868_describe_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t *pdata;
    uint32_t length;
} lp5868_refresh_param_t;

typedef struct {
    uint8_t data;
} lp5868_clear_param_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __LP5868_H */
