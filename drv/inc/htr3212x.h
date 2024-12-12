/**
 * @file drv\inc\htr3212x.h
 *
 * Copyright (C) 2024
 *
 * htr3212x.h is free software: you can redistribute it and/or modify
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
#ifndef __HTR3212X_H
#define __HTR3212X_H

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
#define IOCTL_HTR3212X_GLOBAL_ON                        (IOCTL_USER_START + 0x00)
#define IOCTL_HTR3212X_GLOBAL_OFF                       (IOCTL_USER_START + 0x01)
#define IOCTL_HTR3212X_CHANNEL_ON                       (IOCTL_USER_START + 0x02)
#define IOCTL_HTR3212X_CHANNEL_OFF                      (IOCTL_USER_START + 0x03)

/*---------- type define ----------*/
typedef enum {
    HTR3212X_CHANNEL1,
    HTR3212X_CHANNEL2,
    HTR3212X_CHANNEL3,
    HTR3212X_CHANNEL4,
    HTR3212X_CHANNEL5,
    HTR3212X_CHANNEL6,
    HTR3212X_CHANNEL7,
    HTR3212X_CHANNEL8,
    HTR3212X_CHANNEL9,
    HTR3212X_CHANNEL10,
    HTR3212X_CHANNEL11,
    HTR3212X_CHANNEL12,
} htr3212x_channel_t;

union htr3212x_ioctl_param {
    htr3212x_channel_t channel;
};

typedef struct {
    char *bus_name;
    void *bus;
    uint8_t address;
    struct {
        bool (*init)(void);
        void (*deinit)(void);
        bool (*power)(bool on);
    } ops;
} htr3212x_describe_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __HTR3212X_H */
