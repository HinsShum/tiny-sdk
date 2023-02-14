/**
 * @file common\access_control\serial\serial_transport_level.c
 *
 * Copyright (C) 2022
 *
 * serial_transport_level.c is free software: you can redistribute it and/or modify
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
#include "serial_transport_level.h"
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
    uint32_t wait_ack_ticks;
};

struct transport_ops {
    void (*lock)(void);
    void (*unlock)(void);
};

struct transport {
    uint32_t max_blocked_count;
    uint32_t cur_blocked_count;
    serial_mac_t handle;
    struct list_head head;
    struct transport_ops ops;
};

typedef struct serial_transport_ops *transport_ops_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
/*---------- function ----------*/
static inline void _lock(serial_transport_t self)
{
    if(self->ops.lock) {
        self->ops.lock();
    }
}

static inline void _unlock(serial_transport_t self)
{
    if(self->ops.unlock) {
        self->ops.unlock();
    }
}

serial_transport_t serial_transport_new(serial_mac_type_t type, uint32_t baudrate, uint32_t recv_capacity,
        uint32_t trans_capacity, uint32_t max_blocked_count, transport_ops_t ops)
{
    serial_transport_t self = NULL;

    assert(ops);
    do {
        if(type >= SERIAL_MAC_TYPE_BOUND) {
            break;
        }
        self = __malloc(sizeof(*self));
        if(!self) {
            break;
        }
        memset(self, 0, sizeof(*self));
        self->max_blocked_count = max_blocked_count;
        INIT_LIST_HEAD(&self->head);
        self->ops.lock = ops->lock;
        self->ops.unlock = ops->unlock;
        self->handle = serial_mac_new((serial_mac_type_t)type, baudrate, 
                recv_capacity, trans_capacity, &ops->mac_ops);
        if(!self->handle) {
            __free(self);
            self = NULL;
        }
    } while(0);

    return self;
}

void serial_transport_delete(serial_transport_t self)
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
    serial_mac_delete(self->handle);
    __free(self);
}

void serial_transport_set_transmitter(serial_transport_t self, const uint8_t *pbuf, uint32_t length)
{
    assert(self);
    assert(self->handle);
    serial_mac_set_transmitter(self->handle, pbuf, length);
}

serial_transport_expection_t serial_transport_set_transmitter_cache(serial_transport_t self, const uint8_t *pbuf,
        uint32_t length, uint16_t retrans_count, uint32_t wait_ack_ticks)
{
    serial_transport_expection_t err = SERIAL_TRANSPORT_EX_MEMORY_EMPTY;
    uint32_t alloc_length = 0;
    node_t n = NULL;

    assert(self);
    assert(self->handle);
    do {
        if(self->cur_blocked_count >= self->max_blocked_count) {
            break;
        }
        alloc_length = length + sizeof(*n);
        n = __malloc(alloc_length);
        if(!n) {
            break;
        }
        memset(n, 0, alloc_length);
        n->pbuf = ((uint8_t *)n) + sizeof(*n);
        memcpy(n->pbuf, pbuf, length);
        n->length = length;
        n->retrans_max_count = retrans_count;
        n->wait_ack_ticks = wait_ack_ticks;
        _lock(self);
        list_add_tail(&n->node, &self->head);
        self->cur_blocked_count++;
        _unlock(self);
        err = SERIAL_TRANSPORT_EX_NONE;
    } while(0);

    return err;
}

void serial_transport_clear_transmitter(serial_transport_t self)
{
    assert(self);
    assert(self->handle);
    serial_mac_clear_transmitter(self->handle);
}

void serial_transport_recv_byte(serial_transport_t self, uint8_t byte)
{
    assert(self);
    assert(self->handle);
    serial_mac_recv_byte(self->handle, byte);
}

void serial_transport_timer_expired(serial_transport_t self)
{
    assert(self);
    assert(self->handle);
    serial_mac_timer_expired(self->handle);
}

void serial_transport_poll(serial_transport_t self)
{
    assert(self);
    assert(self->handle);
    serial_mac_poll(self->handle);
    if(!list_empty_careful(&self->head)) {
        node_t n = list_first_entry(&self->head, struct node, node);
        if(serial_mac_set_transmitter_cache(self->handle, n->pbuf, n->length, 
                n->retrans_max_count, n->wait_ack_ticks) != SERIAL_MAC_EX_TRANS_BUSY) {
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

void serial_transport_called_per_tick(serial_transport_t self)
{
    assert(self);
    assert(self->handle);
    serial_mac_called_per_tick(self->handle);
}
