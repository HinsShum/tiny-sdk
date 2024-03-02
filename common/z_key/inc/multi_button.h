/**
 * @file drv\inc\multi_button.h
 *
 * Copyright (C) 2021
 *
 * gpio.h is free software: you can redistribute it and/or modify
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
 * @author zhuodada
 *
 * @encoding utf-8
 */

#ifndef _MULTI_BUTTON_H_
#define _MULTI_BUTTON_H_
#include "stdint.h"
#include "string.h"

// According to your need to modify the constants.
#define TICKS_INTERVAL 5 // ms
#define DEBOUNCE_TICKS 3 // MAX 8
#define SHORT_TICKS    (100 / TICKS_INTERVAL)
#define LONG_TICKS     (500 / TICKS_INTERVAL)

typedef void (*BtnCallback)(void *);
typedef enum
{
    PRESS_DOWN = 0,
    PRESS_UP,
    PRESS_REPEAT,
    SINGLE_CLICK,
    DOUBLE_CLICK,
    LONG_PRESS_START,
    LONG_PRESS_HOLD,
    number_of_event,
    NONE_PRESS
} PressEvent;

typedef struct Button
{
    uint16_t ticks;
    uint8_t  key_id;
    uint8_t  repeat : 4;
    uint8_t  event : 4;
    uint8_t  state : 3;
    uint8_t  debounce_cnt : 3;
    uint8_t  active_level : 1;
    uint8_t  button_level : 1;
    uint8_t (*hal_button_Level)(void);
    BtnCallback    cb[number_of_event];
    struct Button *next;
} Button;

#ifdef __cplusplus
extern "C"
{
#endif

    void          button_init(struct Button *handle, uint8_t (*pin_level)(), uint8_t active_level, uint8_t keyid);
    void          button_attach(struct Button *handle, PressEvent event, BtnCallback cb);
    PressEvent    get_button_event(struct Button *handle);
    int           button_start(struct Button *handle);
    void          button_stop(struct Button *handle);
    void          button_ticks(void);
    unsigned char get_button_keyid(struct Button *handle);

#ifdef __cplusplus
}
#endif

#endif
