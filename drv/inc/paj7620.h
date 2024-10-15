/**
 * @file drv\inc\paj7620.h
 *
 * Copyright (C) 2024
 *
 * paj7620.h is free software: you can redistribute it and/or modify
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
#ifndef __PAJ7620_H
#define __PAJ7620_H

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
#define IOCTL_PAJ7620_SET_IRQ_HANDLER                       (IOCTL_USER_START + 0x00)
#define IOCTL_PAJ7620_SET_EVENT_CALLBACK                    (IOCTL_USER_START + 0x01)
#define IOCTL_PAJ7620_READ_IRQ_PIN                          (IOCTL_USER_START + 0x02)
#define IOCTL_PAJ7620_INTERRUPT_HANDLING                    (IOCTL_USER_START + 0x03)

/*---------- type define ----------*/
typedef enum {
	PAJ7620_EVENT_GESTURE_NONE,
	PAJ7620_EVENT_GESTURE_UP,
	PAJ7620_EVENT_GESTURE_DOWN,
	PAJ7620_EVENT_GESTURE_LEFT,
	PAJ7620_EVENT_GESTURE_RIGHT,
	PAJ7620_EVENT_GESTURE_FORWARD,
	PAJ7620_EVENT_GESTURE_BACKWARD,
	PAJ7620_EVENT_GESTURE_CIRCLE_CLOCKWISE,
	PAJ7620_EVENT_GESTURE_CIRCLE_COUNTER_CLOCKWISE,
	PAJ7620_EVENT_GESTURE_WAVE
} paj7620_event_gesture_t;

typedef struct {
    const uint8_t *data;
} paj7620_configure_t;

typedef struct {
    bool (*init)(void);
    void (*deinit)(void);
    bool (*get_irq_pin)(void);
    bool (*irq_ctrl)(bool en);
    void (*on_event)(paj7620_event_gesture_t event);
    int32_t (*irq_handler)(uint32_t irq, void *args, uint32_t length);
} paj7620_ops_t;

typedef struct {
    char *bus_name;
    void *bus;
    uint8_t address;
    paj7620_configure_t configure;
    paj7620_ops_t ops;
} paj7620_describe_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __PAJ7620_H */
