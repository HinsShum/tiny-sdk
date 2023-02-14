/**
 * @file common\access_control\radio\radio_transport_level.c
 *
 * Copyright (C) 2022
 *
 * radio_transport_level.c is free software: you can redistribute it and/or modify
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
#include "radio_transport_level.h"
#include "lists.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
/*---------- type define ----------*/
typedef struct node *node_t;
struct node {
    struct list_head node;
    uint8_t *pbuf;
    uint32_t length;
    uint32_t retrans_max_count;
};

struct transport_ops {
    void (*lock)(void);
    void (*unlock)(void);
};

struct transport {
    uint32_t max_blocked_count;
    uint32_t cur_blocked_count;
    radio_mac_t handle;
    struct list_head head;
    struct transport_ops ops;
};

typedef struct radio_transport_ops *transport_ops_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
/*---------- function ----------*/
static inline void _lock(radio_transport_t self)
{
    if(self->ops.lock) {
        self->ops.lock();
    }
}

static inline void _unlock(radio_transport_t self)
{
    if(self->ops.unlock) {
        self->ops.unlock();
    }
}

radio_transport_t radio_transport_new(uint32_t recv_capacity, uint32_t trans_capacity,
        uint32_t max_blocked_count, transport_ops_t ops)
{
    radio_transport_t self = NULL;

    assert(ops);
    do {
        self = __malloc(sizeof(*self));
        if(!self) {
            break;
        }
        memset(self, 0, sizeof(*self));
        self->max_blocked_count = max_blocked_count;
        INIT_LIST_HEAD(&self->head);
        self->ops.lock = ops->lock;
        self->ops.unlock = ops->unlock;
        self->handle = radio_mac_new(recv_capacity, trans_capacity, &ops->mac_ops);
        if(!self->handle) {
            __free(self);
            self = NULL;
        }
    } while(0);

    return self;
}

void radio_transport_delete(radio_transport_t self)
{
    assert(self);
    assert(self->handle);
    _lock(self);
    while(!list_empty_careful(&self->head)) {
        node_t n = list_first_entry(&self->head, struct node, node);
        list_del(&n->node);
        __free(n);
    }
    self->cur_blocked_count = 0;
    _unlock(self);
    radio_mac_delete(self->handle);
    __free(self);
}

void radio_transport_set_transmitter(radio_transport_t self, const uint8_t *pbuf, uint32_t length)
{
    assert(self);
    assert(self->handle);
    radio_mac_set_transmitter(self->handle, pbuf, length);
}

radio_transport_expection_t radio_transport_set_transmitter_cache(radio_transport_t self, const uint8_t *pbuf,
        uint32_t length, uint16_t retrans_count)
{
    radio_transport_expection_t err = RADIO_TRANSPORT_EX_MEMORY_EMPTY;
    uint32_t wanted_size = 0;
    node_t n = NULL;

    assert(self);
    assert(self->handle);
    do {
        if(self->cur_blocked_count >= self->max_blocked_count) {
            break;
        }
        wanted_size = length + sizeof(*n);
        n = __malloc(wanted_size);
        if(!n) {
            break;
        }
        memset(n, 0, wanted_size);
        n->pbuf = ((uint8_t *)n) + sizeof(*n);
        memcpy(n->pbuf, pbuf, length);
        n->length = length;
        n->retrans_max_count = retrans_count;
        _lock(self);
        list_add_tail(&n->node, &self->head);
        self->cur_blocked_count++;
        _unlock(self);
        err = RADIO_TRANSPORT_EX_NONE;
    } while(0);

    return err;
}

void radio_transport_clear_transmitter(radio_transport_t self)
{
    assert(self);
    assert(self->handle);
    radio_mac_clear_transmitter(self->handle);
}

void radio_transport_event_post(radio_transport_t self, radio_mac_evt_t evt, bool protected)
{
    assert(self);
    assert(self->handle);
    radio_mac_event_post(self->handle, evt, protected);
}

void radio_transport_poll(radio_transport_t self)
{
    assert(self);
    assert(self->handle);
    radio_mac_poll(self->handle);
    if(!list_empty_careful(&self->head)) {
        node_t n = list_first_entry(&self->head, struct node, node);
        if(radio_mac_set_transmitter_cache(self->handle, n->pbuf, n->length, n->retrans_max_count) != RADIO_MAC_EX_TRANS_BUSY) {
            _lock(self);
            list_del(&n->node);
            if(self->cur_blocked_count) {
                self->cur_blocked_count--;
            }
            _unlock(self);
            __free(n);
        }
    }
}

void radio_transport_called_per_tick(radio_transport_t self)
{
    assert(self);
    assert(self->handle);
    radio_mac_called_per_tick(self->handle);
}
