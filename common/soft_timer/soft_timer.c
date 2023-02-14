/**
 * @file common/utils/soft_timer/soft_timer.c
 *
 * Copyright (C) 2022
 *
 * soft_timer.c is free software: you can redistribute it and/or modify
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
#include "soft_timer.h"
#include "lists.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
#define TAG                                 "SoftTimer"

/*---------- type define ----------*/
struct timer_tcb {
    struct list_head node;
    soft_timer_mode_t mode;
    const char *name;
    uint32_t remaining;
    uint32_t period;
    void *user_data;
    struct {
        void (*cb)(timer_handle_t tcb);
        void (*insert)(timer_handle_t tcb);
        void (*remove)(timer_handle_t tcb);
    } ops;
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
static void _remove_from_ready_list(timer_handle_t tcb);
static void _remove_from_active_list(timer_handle_t tcb);
static void _insert_to_ready_list(timer_handle_t tcb);
static void _insert_to_active_list(timer_handle_t tcb);

/*---------- variable ----------*/
static LIST_HEAD(_timer_ready_list);        /*<< Mount all timers that due now */
static LIST_HEAD(_timer_active_list);       /*<< Mount all timers that are running */
static uint32_t _timer_count;               /*<< The expiry time of the first timer on the active list */

/*---------- function ----------*/
static inline void _lock(void)
{
    __enter_critical();
}

static inline void _unlock(void)
{
    __exit_critical();
}

static void _remove_from_ready_list(timer_handle_t tcb)
{
    _lock();
    list_del(&tcb->node);
    _unlock();
    tcb->ops.remove = NULL;
    tcb->ops.insert = _insert_to_active_list;
}

static void _remove_from_active_list(timer_handle_t tcb)
{
    _lock();
    /* update the first active tcb's remaining value */
    list_first_entry(&_timer_active_list, struct timer_tcb, node)->remaining = _timer_count;
    /* if tcb is not the last active tcb, update the next tcb's remaining value */
    if(list_last_entry(&_timer_active_list, struct timer_tcb, node) != tcb) {
        list_next_entry(tcb, struct timer_tcb, node)->remaining += tcb->remaining;
    }
    /* remove tcb from active list */
    list_del(&tcb->node);
    /* update timer count */
    _timer_count = list_first_entry(&_timer_active_list, struct timer_tcb, node)->remaining;
    _unlock();
    tcb->ops.remove = NULL;
    tcb->ops.insert = _insert_to_ready_list;
}

static void _insert_to_ready_list(timer_handle_t tcb)
{
    _lock();
    list_add_tail(&tcb->node, &_timer_ready_list);
    _unlock();
    tcb->ops.remove = _remove_from_ready_list;
    tcb->ops.insert = NULL;
}

static void _insert_to_active_list(timer_handle_t tcb)
{
    timer_handle_t iter = NULL, n = NULL;
    uint32_t remaining_total = 0;
    bool insertion_point_found = false;

    _lock();
    if(list_empty_careful(&_timer_active_list)) {
        list_add(&tcb->node, &_timer_active_list);
        tcb->remaining = tcb->period;
        _timer_count = tcb->remaining;
    } else {
        /* update the first active tcb's remaining value */
        list_first_entry(&_timer_active_list, struct timer_tcb, node)->remaining = _timer_count;
        list_for_each_entry(iter, struct timer_tcb, &_timer_active_list, node) {
            remaining_total += iter->remaining;
            if(remaining_total > tcb->period) {
                insertion_point_found = true;
                break;
            }
        }
        if(insertion_point_found) {
            /* insert to the insertion point */
            n = list_prev_entry(iter, struct timer_tcb, node);
            list_add(&tcb->node, &n->node);
            /* calc the remaining value */
            remaining_total -= iter->remaining;
            tcb->remaining = tcb->period - remaining_total;
            iter->remaining -= tcb->remaining;
        } else {
            /* insert to the list last */
            tcb->remaining = tcb->period - remaining_total;
            list_add_tail(&tcb->node, &_timer_active_list);
        }
        /* update timer count */
        _timer_count = list_first_entry(&_timer_active_list, struct timer_tcb, node)->remaining;
    }
    _unlock();
    tcb->ops.remove = _remove_from_active_list;
    tcb->ops.insert = NULL;
}

timer_handle_t soft_timer_create(const char *name, soft_timer_mode_t mode, uint32_t period, void *user_data, timer_cb_t cb)
{
    timer_handle_t tcb = NULL;

    do {
        if(!period) {
            xlog_tag_error(TAG, "SoftTimer's period can not be zero\n");
            break;
        }
        if(mode > SFTIM_MODE_REPEAT) {
            xlog_tag_error(TAG, "SoftTimer's mode para format error\n");
            break;
        }
        tcb = __malloc(sizeof(struct timer_tcb));
        if(!tcb) {
            xlog_tag_error(TAG, "No memory to alloc new tiemr tcb\n");
            break;
        }
        memset(tcb, 0, sizeof(struct timer_tcb));
        INIT_LIST_HEAD(&tcb->node);
        tcb->name = name;
        tcb->mode = mode;
        tcb->period = period;
        tcb->remaining = period;
        tcb->user_data = user_data;
        tcb->ops.cb = cb;
        tcb->ops.insert = _insert_to_active_list;
    } while(0);

    return tcb;
}

void soft_timer_destroy(timer_handle_t tcb)
{
    assert(tcb);
    if(tcb->ops.remove) {
        tcb->ops.remove(tcb);
    }
    __free(tcb);
}

void soft_timer_start(timer_handle_t tcb)
{
    assert(tcb);
    if(tcb->ops.remove) {
        tcb->ops.remove(tcb);
    }
    _insert_to_active_list(tcb);
}

void soft_timer_restart(timer_handle_t tcb)
{
    soft_timer_start(tcb);
}

void soft_timer_stop(timer_handle_t tcb)
{
    assert(tcb);
    if(tcb->ops.remove) {
        tcb->ops.remove(tcb);
    }
}

void soft_timer_change_period(timer_handle_t tcb, uint32_t period)
{
    assert(tcb);
    assert(period);
    if(tcb->ops.remove) {
        tcb->ops.remove(tcb);
    }
    tcb->period = period;
    _insert_to_active_list(tcb);
}

void soft_timer_set_reload_mode(timer_handle_t tcb, soft_timer_mode_t mode)
{
    assert(tcb);
    tcb->mode = mode;
    if(tcb->mode > SFTIM_MODE_REPEAT) {
        tcb->mode = SFTIM_MODE_REPEAT;
    }
}

bool soft_timer_is_active(timer_handle_t tcb)
{
    bool retval = false;

    assert(tcb);
    if(tcb->ops.remove == _remove_from_active_list) {
        retval = true;
    }

    return retval;
}

const char *soft_timer_get_name(timer_handle_t tcb)
{
    assert(tcb);

    return tcb->name;
}

soft_timer_mode_t soft_timer_get_reload_mode(timer_handle_t tcb)
{
    assert(tcb);

    return tcb->mode;
}

uint32_t soft_timer_get_period(timer_handle_t tcb)
{
    assert(tcb);

    return tcb->period;
}

void *soft_timer_get_user_data(timer_handle_t tcb)
{
    assert(tcb);

    return tcb->user_data;
}

void soft_timer_poll(void)
{
    timer_handle_t tcb = NULL;

    while(list_empty_careful(&_timer_ready_list) != true) {
        _lock();
        tcb = list_first_entry(&_timer_ready_list, struct timer_tcb, node);
        _unlock();
        /* remove from ready list */
        tcb->ops.remove(tcb);
        /* insert to active list */
        if(tcb->mode == SFTIM_MODE_REPEAT) {
            tcb->ops.insert(tcb);
        }
        if(tcb->ops.cb) {
            tcb->ops.cb(tcb);
        }
    }
}

void soft_timer_tick(void)
{
    timer_handle_t tcb = NULL, next_tcb = NULL;

    _timer_count--;
    if(_timer_count == 0 && list_empty_careful(&_timer_active_list) != true) {
        tcb = list_first_entry(&_timer_active_list, struct timer_tcb, node);
        tcb->remaining = 0;
        list_for_each_entry_safe(tcb, next_tcb, struct timer_tcb, &_timer_active_list, node) {
            if(tcb->remaining == 0) {
                tcb->ops.remove(tcb);
                tcb->ops.insert(tcb);
                continue;
            }
            _timer_count = tcb->remaining;
            break;
        }
    }
}
