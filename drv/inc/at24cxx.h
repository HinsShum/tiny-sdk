/**
 * @file drv\inc\at24cxx.h
 *
 * Copyright (C) 2021
 *
 * at24cxx.h is free software: you can redistribute it and/or modify
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
#ifndef __AT24CXX_H
#define __AT24CXX_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "flash.h"
#include "i2c_bus.h"

/*---------- macro ----------*/
#define IOCTL_AT24CXX_SET_EVENT_CALLBACK                    IOCTL_FLASH_INHERIT_START

/*---------- type define ----------*/
struct at24cxx_event {
    enum {
        AT24CXX_EVT_NONE,
        AT24CXX_EVT_WRITE_FAILURE,
        AT24CXX_EVT_READ_FAILURE,
        AT24CXX_EVT_ERASE_FAILURE,
    } type;
    uint32_t offset;
};

typedef struct {
    bool (*init)(void);
    void (*deinit)(void);
    bool (*power)(bool on);
    void (*write_protect_set)(bool enable);
    bool (*write_protect_get)(void);
    void (*write_cycle_time)(void);
    void (*cb)(void);
    void (*on_event)(struct at24cxx_event *evt);
} at24cxx_ops_t;

typedef struct {
    char *bus_name;
    void *bus;
    uint8_t address;
    uint8_t mem_addr_counts;
    uint8_t *blk_buf;
    flash_info_t info;
    at24cxx_ops_t ops;
} at24cxx_describe_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __AT24CXX_H */
