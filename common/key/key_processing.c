/**
 * @file common/utils/key/key_processing.c
 *
 * Copyright (C) 2022
 *
 * key_processing.c is free software: you can redistribute it and/or modify
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

/*---------- includes ----------*/
#include "key_processing.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
#define KEY_LONG_PRESS_TIME                 (500)       /*<< 500ms */
#define KEY_PRESSING_READ_PERIOD            (200)       /*<< 200ms */
#define TAG                                 "KEYPROCESSING"

/*---------- type define ----------*/
struct key_proc {
    bool pressed;
    bool released;
    bool last_key_value;
    uint64_t pressed_time;
    uint64_t pressing_time;
    uint32_t state;
    const void *user_data;
    uint32_t long_pressing_time;
    uint32_t pressing_read_period;
    key_evt_cb_t evt_cb;
    key_input_cb_t input_cb;
};

enum key_state {
    KEY_STATE_RELEASED,
    KEY_STATE_PRESSED = (1 << KEY_EVENT_PRESSED),
    KEY_STATE_PRESSING = (1 << KEY_EVENT_PRESSING),
    KEY_STATE_LONG_PRESSED = (1 << KEY_EVENT_LONG_PRESSED)
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
/*---------- function ----------*/
static inline bool _is_pressed(const key_proc_t key, bool key_value)
{
    bool retval = false;

    if(key_value && !key->pressed) {
        retval = true;
    }

    return retval;
}

static inline bool _is_pressing(const key_proc_t key, bool key_value)
{
    bool retval = false;

    if(key->pressed && key_value) {
        retval = true;
    }

    return retval;
}

static inline bool _is_released(const key_proc_t key, bool key_value)
{
    bool retval = false;

    if(!key_value && !key->released) {
        retval = true;
    }

    return retval;
}

static inline bool _is_key_stable(const key_proc_t key, bool key_value)
{
    return (key->last_key_value == key_value);
}

static void _key_event_cb_default(enum key_event event)
{
    /* do nothing */
}

static bool _key_input_default(const void *user_data)
{
    return false;
}

key_proc_t key_processing_create(const void *user_data, key_input_cb_t input_cb, key_evt_cb_t evt_cb)
{
    key_proc_t key = NULL;

    do {
        key = __malloc(sizeof(struct key_proc));
        if(key == NULL) {
            xlog_tag_error(TAG, "No memory for alloc key processing\n");
            break;
        }
        xlog_tag_message(TAG, "alloc 0x%p for new key\n", key);
        memset(key, 0, sizeof(*key));
        key->pressed = false;
        key->released = true;
        key->last_key_value = false;
        key->user_data = user_data;
        key->long_pressing_time = KEY_LONG_PRESS_TIME;
        key->pressing_read_period = KEY_PRESSING_READ_PERIOD;
        if(input_cb) {
            key->input_cb = input_cb;
        } else {
            key->input_cb = _key_input_default;
        }
        if(evt_cb) {
            key->evt_cb = evt_cb;
        } else {
            key->evt_cb = _key_event_cb_default;
        }
    } while(0);

    return key;
}

void key_processing_destroy(const key_proc_t key)
{
    if(key) {
        __free(key);
    }
}

void key_processing_set_event_cb(const key_proc_t key, key_evt_cb_t evt_cb)
{
    if(key) {
        if(evt_cb != NULL) {
            key->evt_cb = evt_cb;
        } else {
            key->evt_cb = _key_event_cb_default;
        }
    }
}

void key_processing_set_input_cb(const key_proc_t key, key_input_cb_t input_cb)
{
    if(key) {
        if(input_cb != NULL) {
            key->input_cb = input_cb;
        } else {
            key->input_cb = _key_input_default;
        }
    }
}

void key_processing_set_long_pressing_time(const key_proc_t key, uint32_t long_pressing_time)
{
    if(key) {
        if(long_pressing_time) {
            key->long_pressing_time = long_pressing_time;
        } else {
            key->long_pressing_time = KEY_LONG_PRESS_TIME;
        }
    }
}

void key_processing_set_pressing_read_period(const key_proc_t key, uint32_t pressing_read_period)
{
    if(key) {
        if(pressing_read_period) {
            key->pressing_read_period = pressing_read_period;
        } else {
            key->pressing_read_period = KEY_PRESSING_READ_PERIOD;
        }
    }
}

void key_processing(const key_proc_t key)
{
    bool key_value = false;
    uint64_t cur_ticks = __get_ticks();
    bool update = false;
    uint64_t pressing_time = 0;

    do {
        if(key == NULL) {
            break;
        }
        /* get key value */
        key_value = key->input_cb(key->user_data);
        if(_is_key_stable(key, key_value) != true) {
            key->last_key_value = key_value;
            break;
        }
        update = true;
        if(_is_pressed(key, key_value)) {
            /* key pressed */
            key->state |= KEY_STATE_PRESSED;
            key->pressed_time = __ticks2ms(cur_ticks);
            key->pressing_time = 0;
            /* tigger pressed event */
            key->evt_cb(KEY_EVENT_PRESSED);
            break;
        }
        pressing_time = __ticks2ms(cur_ticks) - key->pressed_time;
        if(_is_pressing(key, key_value)) {
            if((pressing_time - key->pressing_time) >= key->pressing_read_period) {
                key->pressing_time = pressing_time;
                key->state |= KEY_STATE_PRESSING;
                /* tigger pressing event */
                key->evt_cb(KEY_EVENT_PRESSING);
            }
            if(pressing_time >= key->long_pressing_time &&
               (key->state & KEY_STATE_LONG_PRESSED) == 0) {
                key->state |= KEY_STATE_LONG_PRESSED;
                /* tigger long pressed event */
                key->evt_cb(KEY_EVENT_LONG_PRESSED);
            }
            break;
        }
        if(_is_released(key, key_value)) {
            if(pressing_time < key->long_pressing_time) {
                /* tigger short clicked event */
                key->evt_cb(KEY_EVENT_SHORT_CLICKED);
            } else {
                /* tigger clicked event */
                key->evt_cb(KEY_EVENT_CLICKED);
            }
            /* tigger released event */
            key->evt_cb(KEY_EVENT_RELEASED);
            key->pressed_time = 0;
            key->pressing_time = 0;
            key->state = KEY_STATE_RELEASED;
        }
    } while(0);
    if(update) {
        key->pressed = key_value;
        key->released = !key_value;
    }
}
