/**
 * @file common\access_control\serial\halfduplex_serial_media_access_control.c
 *
 * Copyright (C) 2022
 *
 * halfduplex_serial_media_access_control.c is free software: you can redistribute it and/or modify
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
#include "halfduplex_serial_media_access_control.h"
#include "pingpong_buffer.h"
#include "options.h"
#include <stdlib.h>
#include <string.h>

/*---------- macro ----------*/
#define DISF                                        __ms2ticks(50)

/*---------- type define ----------*/
typedef enum {
    BUS_IDLE = 0,
    BUS_BUSY = !BUS_IDLE
} bus_state_t;

typedef enum {
    RECV_IDLE,
    RECV_BUSY,
    RECV_ERROR
} recv_state_t;

typedef enum {
    TRANS_IDLE,
    TRANS_READY,
    TRANS_BUSY,
    TRANS_WAI_ACK
} trans_state_t;

struct mac_bus {
    bus_state_t state;
    uint32_t disf;
    uint16_t backoff_counter;
};

struct mac_receive {
    uint8_t *pbuf;
    uint32_t pos;
    recv_state_t state;
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
    uint32_t disf;                  /*<< bus silence time */
    /* serial callback interface */
    bool (*serial_init)(uint32_t baudrate);
    void (*serial_post)(const uint8_t *pbuf, uint32_t length);
    /* timer callback interface */
    bool (*timer_init)(uint32_t t35_50us);
    void (*timer_ctrl)(bool enable);
    /* event callback interface */
    bool (*event_init)(void);
    bool (*event_post)(serial_mac_evt_t evt);
    bool (*event_get)(serial_mac_evt_t *pevt);
    /* receive callback interface */
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

typedef struct serial_mac_ops *serial_mac_ops_t;

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

    if(retrans_count > 8) {
        retrans_count = 8;
    }
    srand((uint32_t)__get_ticks_from_isr());
    count = rand() % _pow(retrans_count);

    return (count == 0) ? 1 : count;
}

static inline bool _mac_bus_busy(serial_mac_t self)
{
    return (BUS_BUSY == self->bus.state);
}

static void _mac_bus_lock(serial_mac_t self)
{
    self->bus.state = BUS_BUSY;
}

static void _mac_bus_unlock(serial_mac_t self)
{
    self->bus.state = BUS_IDLE;
}

static inline void _clear_transmitter(struct mac_transmit *transmitter)
{
    transmitter->pos = 0;
    transmitter->retrans_counter = 0;
    transmitter->retrans_max_value = 0;
    transmitter->state = TRANS_IDLE;
}

static serial_mac_expection_t _port_level_init(uint32_t baudrate, serial_mac_ops_t ops)
{
    serial_mac_expection_t err = SERIAL_MAC_EX_ERROR;
    uint32_t t35_50us = 0;

    do {
        if(!ops->halfduplex.serial_init(baudrate)) {
            break;
        }
        if(!ops->halfduplex.event_init()) {
            break;
        }
        if(baudrate > 19200) {
            /* 36 * 50 = 1800us */
            t35_50us = 36;
        } else {
            /* The timer reload value for a character is given by:
             *
             * ChTimeValue = Ticks_per_1s / ( Baudrate / 11 )
             *             = 11 * Ticks_per_1s / Baudrate
             *             = 220000 / Baudrate
             * The reload for t3.5 is 1.5 times this value and similary
             * for t3.5.
             */
            t35_50us = (7UL * 220000UL) / (2UL * baudrate);
        }
        if(!ops->halfduplex.timer_init(t35_50us)) {
            break;
        }
        err = SERIAL_MAC_EX_NONE;
    } while(0);

    return err;
}

serial_mac_t halfduplex_serial_media_access_controller_new(uint32_t baudrate, uint32_t recv_capacity,
        uint32_t trans_capacity, serial_mac_ops_t ops)
{
    uint8_t *recv_buf0 = NULL, *recv_buf1 = NULL;
    serial_mac_t self = NULL;
    struct mac_receive *preceiver = NULL;
    uint32_t alloc_length = 0;

    assert(ops);
    assert(ops->halfduplex.serial_init);
    assert(ops->halfduplex.serial_post);
    assert(ops->halfduplex.timer_init);
    assert(ops->halfduplex.timer_ctrl);
    assert(ops->halfduplex.event_init);
    assert(ops->halfduplex.event_post);
    assert(ops->halfduplex.event_get);
    assert(ops->receive_packet_parse);
    do {
        if(!recv_capacity || !trans_capacity) {
            break;
        }
        if(SERIAL_MAC_EX_NONE != _port_level_init(baudrate, ops)) {
            break;
        }
        alloc_length = sizeof(*self) + (recv_capacity + sizeof(*preceiver)) * 2 + recv_capacity + trans_capacity;
        self = __malloc(alloc_length);
        if(!self) {
            break;
        }
        memset(self, 0, alloc_length);
        /* configure ops callback pointer */
        self->ops.serial_init = ops->halfduplex.serial_init;
        self->ops.serial_post = ops->halfduplex.serial_post;
        self->ops.timer_init = ops->halfduplex.timer_init;
        self->ops.timer_ctrl = ops->halfduplex.timer_ctrl;
        self->ops.event_init = ops->halfduplex.event_init;
        self->ops.event_post = ops->halfduplex.event_post;
        self->ops.event_get = ops->halfduplex.event_get;
        self->ops.receive_packet_parse = ops->receive_packet_parse;
        self->ops.disf = ops->halfduplex.disf ? ops->halfduplex.disf : DISF;
        /* assign pointer */
        recv_buf0 = ((uint8_t *)self) + sizeof(*self);
        recv_buf1 = recv_buf0 + recv_capacity + sizeof(*preceiver);
        self->transmitter.pbuf = recv_buf1 + recv_capacity + sizeof(*preceiver);
        self->processer.pbuf = self->transmitter.pbuf + trans_capacity;
        /* register receiver to pingpong buffer */
        preceiver = (struct mac_receive *)recv_buf0;
        preceiver->pbuf = recv_buf0 + sizeof(*preceiver);
        preceiver->capacity = recv_capacity;
        preceiver->state = RECV_IDLE;
        preceiver->pos = 0;
        preceiver = (struct mac_receive *)recv_buf1;
        preceiver->pbuf = recv_buf1 + sizeof(*preceiver);
        preceiver->capacity = recv_capacity;
        preceiver->state = RECV_IDLE;
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
        self->bus.state = BUS_IDLE;
        self->bus.backoff_counter = 0;
        self->bus.disf = self->ops.disf;
    } while(0);

    return self;
}

void halfduplex_serial_media_access_controller_delete(serial_mac_t self)
{
    assert(self);
    assert(self->ops.timer_ctrl);
    self->ops.timer_ctrl(false);
    __free(self);
}

void halfduplex_serial_mac_set_transmitter(serial_mac_t self, const uint8_t *pbuf, uint32_t length)
{
    trans_state_t old_state = self->transmitter.state;

    assert(self);
    assert(self->ops.serial_post);
    _mac_bus_lock(self);
    self->transmitter.state = TRANS_BUSY;
    self->ops.serial_post(pbuf, length);
#ifdef CONFIG_SERIAL_MAC_DEBUG
    PRINT_BUFFER_CONTENT(COLOR_YELLOW, "[Serial]W", pbuf, length);
#endif
    self->bus.disf = self->ops.disf;
    self->transmitter.state = old_state;
    _mac_bus_unlock(self);
}

serial_mac_expection_t halfduplex_serial_mac_set_transmitter_cache(serial_mac_t self, const uint8_t *pbuf, 
        uint32_t length, uint16_t retrans_count, uint32_t no_use)
{
    serial_mac_expection_t err = SERIAL_MAC_EX_TRANS_BUSY;

    assert(self);
    do {
        if(self->transmitter.state != TRANS_IDLE) {
            break;
        }
        if(length > self->transmitter.capacity) {
            err = SERIAL_MAC_EX_ERROR;
            break;
        }
        memcpy(self->transmitter.pbuf, pbuf, length);
        self->transmitter.pos = length;
        self->transmitter.retrans_counter = 0;
        self->transmitter.retrans_max_value = retrans_count;
        self->transmitter.state = TRANS_READY;
        err = SERIAL_MAC_EX_NONE;
    } while(0);

    return err;
}

void halfduplex_serial_mac_clear_transmitter(serial_mac_t self)
{
    assert(self);
    _clear_transmitter(&self->transmitter);
}

void halfduplex_serial_mac_recv_byte(serial_mac_t self, uint8_t byte)
{
    assert(self);
    assert(self->ops.timer_ctrl);
    /* if halfduplex serial is transmitting, skip recv any data */
    if(self->transmitter.state != TRANS_BUSY) {
        _mac_bus_lock(self);
        switch(self->preceiver->state) {
            case RECV_IDLE:
                /* start recv */
                self->preceiver->pos = 0;
                self->preceiver->pbuf[self->preceiver->pos++] = byte;
                self->preceiver->state = RECV_BUSY;
                self->ops.timer_ctrl(true);
                break;
            case RECV_BUSY:
                if(self->preceiver->pos < self->preceiver->capacity) {
                    self->preceiver->pbuf[self->preceiver->pos++] = byte;
                } else {
                    /* overflow */
                    self->preceiver->state = RECV_ERROR;
                }
                self->ops.timer_ctrl(true);
                break;
            case RECV_ERROR:
                /* drop recv byte */
                self->ops.timer_ctrl(true);
                break;
            default:
                break;
        }
    }
}

void halfduplex_serial_mac_timer_expired(serial_mac_t self)
{
    assert(self);
    assert(self->ops.timer_ctrl);
    assert(self->ops.event_post);
    self->ops.timer_ctrl(false);
    /* choose a clean buffer ready to receive new data */
    pingpong_buffer_set_write_done(&self->pingpong);
    pingpong_buffer_get_read_buf(&self->pingpong, (void **)&self->processer.preceiver);
    pingpong_buffer_get_write_buf(&self->pingpong, (void **)&self->preceiver);
    /* post received event if no error occured during receiving bytes */
    if(self->processer.preceiver->state == RECV_BUSY) {
        self->ops.event_post(SERIAL_MAC_EVT_RECEIVED);
    }
    /* recover bus */
    self->processer.preceiver->state = RECV_IDLE;
    self->bus.disf = self->ops.disf;
    _mac_bus_unlock(self);
}

void halfduplex_serial_mac_poll(serial_mac_t self)
{
    serial_mac_evt_t evt;

    assert(self);
    assert(self->ops.event_get);
    assert(self->ops.serial_post);
    assert(self->ops.receive_packet_parse);
    if(self->ops.event_get(&evt)) {
        switch(evt) {
            case SERIAL_MAC_EVT_RECEIVED:
#ifdef CONFIG_SERIAL_MAC_DEBUG
                PRINT_BUFFER_CONTENT(COLOR_YELLOW, "[Serial]R", 
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
                break;
            case SERIAL_MAC_EVT_TRANSMITTER_READY:
                self->bus.disf = self->ops.disf;
                self->transmitter.state = TRANS_BUSY;
                self->ops.serial_post(self->transmitter.pbuf, self->transmitter.pos);
#ifdef CONFIG_SERIAL_MAC_DEBUG
                PRINT_BUFFER_CONTENT(COLOR_YELLOW, "[Serial]W", self->transmitter.pbuf, self->transmitter.pos);
#endif
                self->transmitter.state = TRANS_WAI_ACK;
                _mac_bus_unlock(self);
                break;
            default:
                break;
        }
    }
}

void halfduplex_serial_mac_called_per_tick(serial_mac_t self)
{
    assert(self);
    assert(self->ops.event_post);
    do {
        if(_mac_bus_busy(self)) {
            /* supend bus transport fsm */
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
            uint16_t retrans_count = self->transmitter.retrans_counter + 1;
            if(self->transmitter.retrans_max_value == 0 || retrans_count > self->transmitter.retrans_max_value) {
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
            self->ops.event_post(SERIAL_MAC_EVT_TRANSMITTER_READY);
            _mac_bus_lock(self);
        }
    } while(0);
}
