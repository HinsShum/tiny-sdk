/**
 * @file components\tiny-sdk\drv\inc\analog.h
 *
 * Copyright (C) 2023
 *
 * analog.h is free software: you can redistribute it and/or modify
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
#ifndef __ANALOG_H
#define __ANALOG_H

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
#define IOCTL_ANALOG_ENABLE                             (IOCTL_USER_START + 0x00)
#define IOCTL_ANALOG_DISABLE                            (IOCTL_USER_START + 0x01)
#define IOCTL_ANALOG_GET                                (IOCTL_USER_START + 0x02)

/*---------- type define ----------*/
typedef struct {
    uint32_t number_of_channels;
    struct {
        bool (*init)(void);
        void (*deinit)(void);
        bool (*enable)(bool ctrl);
        uint32_t (*get)(uint32_t channel);
        int32_t (*irq_handler)(uint32_t irq, void *args, uint32_t length);
    } ops;
} analog_describe_t;

union analog_ioctl_param {
    struct {
        uint32_t channel;
        uint32_t data;
    } get;
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __ANALOG_H */