/**
 * @file common/utils/key/inc/key_processing.h
 *
 * Copyright (C) 2022
 *
 * key_processing.h is free software: you can redistribute it and/or modify
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
#ifndef __KEY_PROCESSING_H
#define __KEY_PROCESSING_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*---------- macro ----------*/
/*---------- type define ----------*/
enum key_event {
    KEY_EVENT_PRESSED,              /*<< The key has been pressed */
    KEY_EVENT_PRESSING,             /*<< The key is being pressed(called continuously while pressing) */
    KEY_EVENT_LONG_PRESSED,         /*<< The key has been pressed for at least `LONG PRESS TIME` ms */
    KEY_EVENT_SHORT_CLICKED,        /*<< User pressed the key for a short period of time, then released it */
    KEY_EVENT_CLICKED,              /*<< Called on release */
    KEY_EVENT_RELEASED              /*<< Called in every cases when the key has been released */
};

typedef struct key_proc *key_proc_t;

typedef bool (*key_input_cb_t)(const void *user_data);

typedef void (*key_evt_cb_t)(enum key_event evt);

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
extern key_proc_t key_processing_create(const void *user_data, key_input_cb_t input_cb, key_evt_cb_t evt_cb);
extern void key_processing_destroy(const key_proc_t key);
extern void key_processing_set_event_cb(const key_proc_t key, key_evt_cb_t evt_cb);
extern void key_processing_set_input_cb(const key_proc_t key, key_input_cb_t input_cb);
extern void key_processing_set_long_pressing_time(const key_proc_t key, uint32_t long_pressing_time);
extern void key_processing_set_pressing_read_period(const key_proc_t key, uint32_t pressing_read_period);
extern void key_processing(const key_proc_t key);

#ifdef __cplusplus
}
#endif
#endif /* __KEY_PROCESSING_H */
