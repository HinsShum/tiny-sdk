/**
 * @file common\access_control\radio\radio_media_access_control.c
 *
 * Copyright (C) 2022
 *
 * radio_media_access_control.c is free software: you can redistribute it and/or modify
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
#include "radio_media_access_control.h"
#include "pingpong_buffer.h"
#include "options.h"
#include <stdlib.h>
#include <string.h>

/*---------- macro ----------*/
#define DISF                                        (__ms2ticks(200))
#define BUS_BUSY_TIMEOUT                            (__ms2ticks(50))

/*---------- type define ----------*/
typedef enum {
    BUS_IDLE = 0,
    BUS_BUSY = !BUS_IDLE
} bus_state_t;

typedef enum {
    TRANS_IDLE,
    TRANS_READY,
    TRANS_BUSY,
    TRANS_WAI_ACK
} trans_state_t;

struct mac_bus {
    bus_state_t state;
    uint8_t disf;
    uint16_t backoff_counter;
    uint32_t bus_busy_timeout;
};

struct mac_receive {
    uint8_t *pbuf;
    uint32_t pos;
    uint32_t capacity;
};

struct mac_transmit {
    uint8_t *pbuf;
    uint32_t pos;
    uint32_t capacity;
    uint16_t retrans_counter;
    uint16_t retrans_max_value;
    trans_state_t state;
};

struct mac_process {
    uint8_t *pbuf;
    uint32_t pos;
    uint32_t capacity;
    struct mac_receive *preceiver;
};

struct mac_ops {
    /* radio callback interface */
    uint32_t (*radio_receive)(uint8_t *pbuf, uint32_t capacity, bool continuing);
    void (*radio_post)(const uint8_t *pbuf, uint32_t length);
    /* event callback interface */
    bool (*event_init)(void);
    void (*event_post)(radio_mac_evt_t evt, bool protected);
    bool (*event_get)(radio_mac_evt_t *pevt);
    /* packet parse callback interface */
    void (*receive_packet_parse)(const uint8_t *recv_buf, uint32_t recv_length, const uint8_t *trans_buf, uint32_t trans_length);
};

struct mac {
    struct mac_bus bus;
    struct mac_receive *preceiver;
    struct mac_transmit transmitter;
    struct mac_process processer;
    struct pingpong_buffer pingpong;
    struct mac_ops ops;
};

typedef struct radio_mac_ops *radio_mac_ops_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
/*---------- function ----------*/
static inline uint32_t _pow(uint16_t x)
{
    uint32_t val = 1;

    for(uint16_t i = 0; i < (x + 5); ++i) {
        val = val * 2;
    }

    return val;
}

static inline uint16_t _get_random_backoff(uint16_t retrans_count)
{
    uint32_t count = 0;

    if(retrans_count > 5) {
        retrans_count = 5;
    }
    srand((uint32_t)__get_ticks_from_isr());
    count = rand() % _pow(retrans_count);

    return (count == 0) ? 1 : count;
}

static inline void _mac_bus_lock(radio_mac_t self)
{
    self->bus.bus_busy_timeout = BUS_BUSY_TIMEOUT;
    self->bus.state = BUS_BUSY;
}

static inline void _mac_bus_unlock(radio_mac_t self)
{
    self->bus.state = BUS_IDLE;
    self->bus.bus_busy_timeout = 0;
}

static inline bool _mac_bus_busy(radio_mac_t self)
{
    return (BUS_BUSY == self->bus.state);
}

static inline void _clear_transmitter(struct mac_transmit *transmitter)
{
    transmitter->pos = 0;
    transmitter->retrans_counter = 0;
    transmitter->retrans_max_value = 0;
    transmitter->state = TRANS_IDLE;
}

static uint32_t _get_recv_data(radio_mac_t self)
{
    uint32_t recv_size = 0;

    self->preceiver->pos += self->ops.radio_receive(&self->preceiver->pbuf[self->preceiver->pos],
            self->preceiver->capacity - self->preceiver->pos, false);
    if(self->preceiver->pos != 0) {
        recv_size = self->preceiver->pos;
        pingpong_buffer_set_write_done(&self->pingpong);
        pingpong_buffer_get_read_buf(&self->pingpong, (void **)&self->processer.preceiver);
        pingpong_buffer_get_write_buf(&self->pingpong, (void **)&self->preceiver);
        self->preceiver->pos = 0;
    }

    return recv_size;
}

static void _get_recv_data_continue(radio_mac_t self)
{
    self->preceiver->pos += self->ops.radio_receive(&self->preceiver->pbuf[self->preceiver->pos],
            self->preceiver->capacity - self->preceiver->pos, true);
}

static radio_mac_expection_t _port_level_init(radio_mac_ops_t ops)
{
    radio_mac_expection_t err = RADIO_MAC_EX_ERROR;

    if(ops->event_init()) {
        err = RADIO_MAC_EX_NONE;
    }

    return err;
}

radio_mac_t radio_mac_new(uint32_t recv_capacity, uint32_t trans_capacity, radio_mac_ops_t ops)
{
    uint8_t *recv_buf0 = NULL, *recv_buf1 = NULL;
    radio_mac_t self = NULL;
    struct mac_receive *preceiver = NULL;
    uint32_t wanted_size = 0;

    assert(recv_capacity);
    assert(trans_capacity);
    assert(ops);
    assert(ops->radio_receive);
    assert(ops->radio_post);
    assert(ops->event_init);
    assert(ops->event_post);
    assert(ops->event_get);
    assert(ops->receive_packet_parse);
    do {
        if(!recv_capacity || !trans_capacity) {
            break;
        }
        if(RADIO_MAC_EX_NONE != _port_level_init(ops)) {
            break;
        }
        wanted_size = sizeof(*self) + (recv_capacity + sizeof(*preceiver)) * 2 + recv_capacity + trans_capacity;
        self = __malloc(wanted_size);
        if(!self) {
            break;
        }
        memset(self, 0, wanted_size);
        /* configure ops callback pointer */
        self->ops.radio_receive = ops->radio_receive;
        self->ops.radio_post = ops->radio_post;
        self->ops.event_init = ops->event_init;
        self->ops.event_post = ops->event_post;
        self->ops.event_get = ops->event_get;
        self->ops.receive_packet_parse = ops->receive_packet_parse;
        /* assign pointer */
        recv_buf0 = ((uint8_t *)self) + sizeof(*self);
        recv_buf1 = recv_buf0 + recv_capacity + sizeof(*preceiver);
        self->transmitter.pbuf = recv_buf1 + recv_capacity + sizeof(*preceiver);
        self->processer.pbuf = self->transmitter.pbuf + trans_capacity;
        /* register receiver to pingpong buffer */
        preceiver = (struct mac_receive *)recv_buf0;
        preceiver->pbuf = recv_buf0 + sizeof(*preceiver);
        preceiver->capacity = recv_capacity;
        preceiver->pos = 0;
        preceiver = (struct mac_receive *)recv_buf1;
        preceiver->pbuf = recv_buf1 + sizeof(*preceiver);
        preceiver->capacity = recv_capacity;
        preceiver->pos = 0;
        pingpong_buffer_init(&self->pingpong, recv_buf0, recv_buf1);
        /* get receiver */
        pingpong_buffer_get_write_buf(&self->pingpong, (void **)&self->preceiver);
        /* configure transmitter */
        self->transmitter.capacity = trans_capacity;
        self->transmitter.pos = 0;
        self->transmitter.retrans_counter = 0;
        self->transmitter.retrans_max_value = 0;
        self->transmitter.state = TRANS_IDLE;
        /* configure processer */
        self->processer.capacity = recv_capacity;
        self->processer.pos = 0;
        self->processer.preceiver = NULL;
        /* configure bus */
        self->bus.disf = DISF;
        self->bus.state = BUS_IDLE;
        self->bus.backoff_counter = 0;
        self->bus.bus_busy_timeout = 0;
    } while(0);

    return self;
}

void radio_mac_delete(radio_mac_t self)
{
    assert(self);
    __free(self);
}

void radio_mac_set_transmitter(radio_mac_t self, const uint8_t *pbuf, uint32_t length)
{
    trans_state_t old_state = TRANS_IDLE;

    assert(self);
    assert(self->ops.radio_receive);
    old_state = self->transmitter.state;
    _mac_bus_lock(self);
    self->transmitter.state = TRANS_BUSY;
    self->ops.radio_post(pbuf, length);
#ifdef CONFIG_RADIO_MAC_DEBUG
    PRINT_BUFFER_CONTENT(COLOR_GREEN, "[Radio]W", pbuf, length);
#endif
    self->bus.disf = DISF;
    self->transmitter.state = old_state;
    _mac_bus_unlock(self);
}

radio_mac_expection_t radio_mac_set_transmitter_cache(radio_mac_t self, const uint8_t *pbuf,
        uint32_t length, uint16_t retrans_count)
{
    radio_mac_expection_t err = RADIO_MAC_EX_TRANS_BUSY;

    assert(self);
    assert(pbuf);
    assert(length);
    do {
        if(self->transmitter.state != TRANS_IDLE) {
            break;
        }
        if(length > self->transmitter.capacity) {
            err = RADIO_MAC_EX_ERROR;
            break;
        }
        memcpy(self->transmitter.pbuf, pbuf, length);
        self->transmitter.pos = length;
        self->transmitter.retrans_counter = 0;
        self->transmitter.retrans_max_value = retrans_count;
        self->transmitter.state = TRANS_READY;
        err = RADIO_MAC_EX_NONE;
    } while(0);

    return err;
}

void radio_mac_clear_transmitter(radio_mac_t self)
{
    assert(self);
    _clear_transmitter(&self->transmitter);
}

void radio_mac_event_post(radio_mac_t self, radio_mac_evt_t evt, bool protected)
{
    assert(self);
    assert(self->ops.event_post);
    self->ops.event_post(evt, protected);
}

void radio_mac_poll(radio_mac_t self)
{
    radio_mac_evt_t evt;

    assert(self);
    assert(self->ops.event_get);
    assert(self->ops.radio_receive);
    assert(self->ops.receive_packet_parse);
    assert(self->ops.radio_post);
    if(self->ops.event_get(&evt)) {
        switch(evt) {
            case RADIO_MAC_EVT_RECEIVED:
                _mac_bus_unlock(self);
                if(_get_recv_data(self)) {
#ifdef CONFIG_RADIO_MAC_DEBUG
                    PRINT_BUFFER_CONTENT(COLOR_GREEN, "[Radio]R",
                            self->processer.preceiver->pbuf, self->processer.preceiver->pos);
#endif
                    /* copy received packet from receiver to processer */
                    memcpy(self->processer.pbuf, self->processer.preceiver->pbuf, self->processer.preceiver->pos);
                    self->processer.pos = self->processer.preceiver->pos;
                    pingpong_buffer_set_read_done(&self->pingpong);
                    if(self->transmitter.state == TRANS_WAI_ACK) {
                        self->ops.receive_packet_parse(self->processer.pbuf, self->processer.pos,
                                self->transmitter.pbuf, self->transmitter.pos);
                    } else {
                        self->ops.receive_packet_parse(self->processer.pbuf, self->processer.pos, NULL, 0);
                    }
                }
                break;
            case RADIO_MAC_EVT_RECEIVING:
                _mac_bus_lock(self);
                _get_recv_data_continue(self);
                break;
            case RADIO_MAC_EVT_TRANSMITTER_READY:
                self->bus.disf = DISF;
                self->transmitter.state = TRANS_BUSY;
                self->ops.radio_post(self->transmitter.pbuf, self->transmitter.pos);
#ifdef CONFIG_RADIO_MAC_DEBUG
                PRINT_BUFFER_CONTENT(COLOR_GREEN, "[Radio]W", self->transmitter.pbuf, self->transmitter.pos);
#endif
                self->transmitter.state = TRANS_WAI_ACK;
                break;
            case RADIO_MAC_EVT_BUS_TRY_LOCK:
                _mac_bus_lock(self);
                break;
            case RADIO_MAC_EVT_BUS_TRY_UNLOCK:
                _mac_bus_unlock(self);
                break;
            default:
                break;
        }
    }
}

void radio_mac_called_per_tick(radio_mac_t self)
{
    assert(self);
    assert(self->ops.event_post);
    do {
        if(_mac_bus_busy(self)) {
            /* supend bus transport fsm */
            self->bus.bus_busy_timeout--;
            if(self->bus.bus_busy_timeout == 0) {
                _mac_bus_unlock(self);
            }
            break;
        }
        if(self->transmitter.state == TRANS_IDLE || self->transmitter.state == TRANS_BUSY) {
            /* supend bus transport fsm */
            break;
        }
        /* silence bus for DISF ticks */
        if(self->bus.disf) {
            self->bus.disf--;
            if(self->bus.disf) {
                break;
            }
        }
        /* update transmitter */
        if(self->transmitter.state == TRANS_WAI_ACK) {
            if(self->transmitter.retrans_max_value == 0 || 
                    self->transmitter.retrans_counter >= self->transmitter.retrans_max_value) {
                _clear_transmitter(&self->transmitter);
                break;
            }
            self->transmitter.retrans_counter++;
            self->transmitter.state = TRANS_READY;
        }
        /* random backoff */
        if(!self->bus.backoff_counter) {
            self->bus.backoff_counter = _get_random_backoff(self->transmitter.retrans_counter);
            break;
        }
        self->bus.backoff_counter--;
        if(!self->bus.backoff_counter) {
            self->ops.event_post(RADIO_MAC_EVT_TRANSMITTER_READY, true);
            _mac_bus_lock(self);
        }
    } while(0);
}
