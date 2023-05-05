/**
 * @file drv\inc\pat9125el.h
 *
 * Copyright (C) 2023
 *
 * pat9125el.h is free software: you can redistribute it and/or modify
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
#ifndef __PAT9125EL_H
#define __PAT9125EL_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "i2c_bus.h"

/*---------- macro ----------*/
#define IOCTL_PAT9125EL_GET_DATA                    (IOCTL_USER_START + 0x00)
#define IOCTL_PAT9125EL_SET_IRQ_HANDLER             (IOCTL_USER_START + 0x01)

/*---------- type define ----------*/
typedef struct {
    bool (*init)(void);
    void (*deinit)(void);
    bool (*data_valid)(void);
    int32_t (*irq_handler)(uint32_t irq, void *args, uint32_t length);
} pat9125el_ops_t;

typedef struct {
    uint8_t address;
    char *bus_name;
    void *bus;
    pat9125el_ops_t ops;
} pat9125el_describe_t;

union pat9125el_ioctl_param {
    struct {
        int16_t x;
        int16_t y;
    } data;
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __PAT9125EL_H */
