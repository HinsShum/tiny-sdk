/**
 * @file media_access_control\mia\mia_transport_level.c
 *
 * Copyright (C) 2023
 *
 * mia_transport_level.c is free software: you can redistribute it and/or modify
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
#include "mia_transport_level.h"
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

struct mia_transport {
    uint32_t max_blocked_count;
    uint32_t cur_blocked_count;
    mia_mac_t mac;
    struct list_head head;
    struct transport_ops ops;
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
/*---------- function ----------*/
static inline void _lock(mia_transport_t self)
{
    if(self->ops.lock) {
        self->ops.lock();
    }
}

static inline void _unlock(mia_transport_t self)
{
    if(self->ops.unlock) {
        self->ops.unlock();
    }
}

mia_transport_t mia_transport_new(uint32_t baudrate, uint32_t recv_capacity, uint32_t trans_capacity,
        uint32_t max_blocked_count, mia_transport_ops_t ops)
{
    mia_transport_t self = NULL;

    assert(ops);
    self = __malloc(sizeof(*self));
    if(self) {
        memset(self, 0, sizeof(*self));
        self->max_blocked_count = max_blocked_count;
        INIT_LIST_HEAD(&self->head);
        self->ops.lock = ops->lock;
        self->ops.unlock = ops->unlock;
        self->mac = mia_mac_new(baudrate, recv_capacity, trans_capacity, &ops->mac_ops);
        if(!self->mac) {
            __free(self);
            self = NULL;
        }
    }

    return self;
}

void mia_transport_delete(mia_transport_t self)
{
    assert(self);
    assert(self->mac);
    _lock(self);
    while(!list_empty_careful(&self->head)) {
        node_t n = list_first_entry(&self->head, struct node, node);
        list_del(&n->node);
        __free(n);
    }
    self->cur_blocked_count = 0;
    _unlock(self);
    mia_mac_delete(self->mac);
    __free(self);
}

void mia_transport_set_transmitter(mia_transport_t self, const uint8_t *pbuf, uint32_t length)
{
    assert(self);
    assert(self->mac);
    mia_mac_set_transmitter(self->mac, pbuf, length);
}

mia_transport_expection_t mia_transport_set_transmitter_cache_low(mia_transport_t self,
        const uint8_t *pbuf, uint32_t length, uint16_t retrans_count)
{
    mia_transport_expection_t err = MIA_TRANSPORT_EX_MEMORY_EMPTY;
    uint32_t wanted_size = 0;
    node_t n = NULL;

    assert(self);
    assert(self->mac);
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
        err = MIA_TRANSPORT_EX_NONE;
    } while(0);

    return err;
}

mia_transport_expection_t mia_transport_set_transmitter_cache_high(mia_transport_t self,
        const uint8_t *pbuf, uint32_t length, uint16_t retrans_count)
{
    mia_transport_expection_t err = MIA_TRANSPORT_EX_MEMORY_EMPTY;
    uint32_t wanted_size = 0;
    node_t n = NULL;

    assert(self);
    assert(self->mac);
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
        list_add(&n->node, &self->head);
        self->cur_blocked_count++;
        _unlock(self);
        err = MIA_TRANSPORT_EX_NONE;
    } while(0);

    return err;
}

void mia_transport_clear_transmitter(mia_transport_t self)
{
    assert(self);
    assert(self->mac);
    mia_mac_clear_transmitter(self->mac);
}

void mia_transport_start_bit_detected(mia_transport_t self)
{
    assert(self);
    assert(self->mac);
    mia_mac_start_bit_detected(self->mac);
}

void mia_transport_timer_expired(mia_transport_t self)
{
    assert(self);
    assert(self->mac);
    mia_mac_timer_expired(self->mac);
}

void mia_transport_called_per_tick(mia_transport_t self)
{
    assert(self);
    assert(self->mac);
    mia_mac_called_per_tick(self->mac);
}

void mia_transport_polling(mia_transport_t self)
{
    assert(self);
    assert(self->mac);
    mia_mac_polling(self->mac);
    if(!list_empty_careful(&self->head)) {
        node_t n = list_first_entry(&self->head, struct node, node);
        if(mia_mac_set_transmitter_cache(self->mac, n->pbuf, n->length,
                n->retrans_max_count) != MIA_MAC_EX_TRANS_BUSY) {
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
