/**
 * @file common\access_control\serial\fullduplex_serial_media_access_contorl.c
 *
 * Copyright (C) 2022
 *
 * fullduplex_serial_media_access_contorl.c is free software: you can redistribute it and/or modify
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
#include "fullduplex_serial_media_access_control.h"
#include "pingpong_buffer.h"
#include "options.h"
#include <stdlib.h>
#include <string.h>

/*---------- macro ----------*/
/*---------- type define ----------*/
typedef enum {
    RECV_IDLE,
    RECV_BUSY,
    RECV_ERROR
} recv_state_t;

typedef enum {
    TRANS_IDLE,
    TRANS_READY,
    TRANS_BUSY,
    TRANS_WAIT_ACK
} trans_state_t;

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
    uint32_t wait_ack_ticks;
    uint32_t cur_wait_ack_ticks;
    trans_state_t state;
};

struct mac_process {
    uint8_t *pbuf;
    uint32_t pos;
    uint32_t capacity;
    struct mac_receive *preceiver;
};

struct mac_ops {
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
    uint32_t disf;
    uint32_t disf_default;
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
static inline void _clear_transmitter(struct mac_transmit *transmitter)
{
    transmitter->pos = 0;
    transmitter->retrans_counter = 0;
    transmitter->retrans_max_value = 0;
    transmitter->wait_ack_ticks = 0;
    transmitter->cur_wait_ack_ticks = 0;
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

static uint32_t _get_disf(uint32_t baudrate)
{
    uint32_t us = 0;
    uint32_t disf = 0;

    if(baudrate > 19200) {
        us = 1800;
    } else {
        us = ((7UL * 220000UL) / (2UL * baudrate)) * 50;
    }
    disf = __ms2ticks((us / 1000)) + 1;

    return disf;
}

serial_mac_t fullduplex_serial_media_access_controller_new(uint32_t baudrate, uint32_t recv_capacity,
        uint32_t trans_capacity, serial_mac_ops_t ops)
{
    uint8_t *recv_buf0 = NULL, *recv_buf1 = NULL;
    serial_mac_t self = NULL;
    struct mac_receive *preceiver = NULL;
    uint32_t alloc_length = 0;

    assert(ops);
    assert(ops->fullduplex.serial_init);
    assert(ops->fullduplex.serial_post);
    assert(ops->fullduplex.timer_init);
    assert(ops->fullduplex.timer_ctrl);
    assert(ops->fullduplex.event_init);
    assert(ops->fullduplex.event_post);
    assert(ops->fullduplex.event_get);
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
        self->ops.serial_init = ops->fullduplex.serial_init;
        self->ops.serial_post = ops->fullduplex.serial_post;
        self->ops.timer_init = ops->fullduplex.timer_init;
        self->ops.timer_ctrl = ops->fullduplex.timer_ctrl;
        self->ops.event_init = ops->fullduplex.event_init;
        self->ops.event_post = ops->fullduplex.event_post;
        self->ops.event_get = ops->fullduplex.event_get;
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
        self->transmitter.wait_ack_ticks = 0;
        self->transmitter.cur_wait_ack_ticks = 0;
        self->transmitter.state = TRANS_IDLE;
        /* configure processer */
        self->processer.capacity = recv_capacity;
        self->processer.pos = 0;
        self->processer.preceiver = NULL;
        /* configure other */
        self->disf_default = _get_disf(baudrate);
        self->disf = self->disf_default;
    } while(0);

    return self;
}

void fullduplex_serial_media_access_control_delete(serial_mac_t self)
{
    assert(self);
    assert(self->ops.timer_ctrl);
    self->ops.timer_ctrl(false);
    __free(self);
}

void fullduplex_serial_mac_set_transmitter(serial_mac_t self, const uint8_t *pbuf, uint32_t length)
{
    trans_state_t old_state = self->transmitter.state;

    assert(self);
    assert(self->ops.serial_post);
    self->transmitter.state = TRANS_BUSY;
    self->ops.serial_post(pbuf, length);
#ifdef CONFIG_SERIAL_MAC_DEBUG
    PRINT_BUFFER_CONTENT(COLOR_YELLOW, "[Serial]W", pbuf, length);
#endif
    self->transmitter.state = old_state;
    self->disf = self->disf_default;
}

serial_mac_expection_t fullduplex_serial_mac_set_transmitter_cache(serial_mac_t self, const uint8_t *pbuf,
        uint32_t length, uint16_t retrans_count, uint32_t wait_ack_ticks)
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
        self->transmitter.wait_ack_ticks = wait_ack_ticks;
        self->transmitter.cur_wait_ack_ticks = 0;
        self->transmitter.state = TRANS_READY;
        err = SERIAL_MAC_EX_NONE;
    } while(0);

    return err;
}

void fullduplex_serial_mac_clear_transmitter(serial_mac_t self)
{
    assert(self);
    _clear_transmitter(&self->transmitter);
}

void fullduplex_serial_mac_recv_byte(serial_mac_t self, uint8_t byte)
{
    assert(self);
    assert(self->ops.timer_ctrl);
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
            /* drop recv data */
            self->ops.timer_ctrl(true);
            break;
        default:
            break;
    }
}

void fullduplex_serial_mac_timer_expired(serial_mac_t self)
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
    self->processer.preceiver->state = RECV_IDLE;
}

void fullduplex_serial_mac_poll(serial_mac_t self)
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
                if(self->transmitter.state == TRANS_WAIT_ACK) {
                    self->ops.receive_packet_parse(self->processer.pbuf, self->processer.pos,
                            self->transmitter.pbuf, self->transmitter.pos);
                } else {
                    self->ops.receive_packet_parse(self->processer.pbuf, self->processer.pos, NULL, 0);
                }
                break;
            case SERIAL_MAC_EVT_TRANSMITTER_READY:
                self->transmitter.state = TRANS_BUSY;
                self->ops.serial_post(self->transmitter.pbuf, self->transmitter.pos);
#ifdef CONFIG_SERIAL_MAC_DEBUG
                PRINT_BUFFER_CONTENT(COLOR_YELLOW, "[Serial]W", self->transmitter.pbuf, self->transmitter.pos);
#endif
                self->transmitter.state = TRANS_WAIT_ACK;
                self->disf = self->disf_default;
                break;
            default:
                break;
        }
    }
}

void fullduplex_serial_mac_called_per_tick(serial_mac_t self)
{
    assert(self);
    assert(self->ops.event_post);
    do {
        if(self->transmitter.state == TRANS_IDLE || self->transmitter.state == TRANS_BUSY) {
            /* supend transport fsm */
            break;
        }
        if(self->disf) {
            self->disf--;
            break;
        }
        if(self->transmitter.state == TRANS_READY) {
            self->transmitter.state = TRANS_BUSY;
            self->ops.event_post(SERIAL_MAC_EVT_TRANSMITTER_READY);
            break;
        }
        if(self->transmitter.cur_wait_ack_ticks < self->transmitter.wait_ack_ticks) {
            self->transmitter.cur_wait_ack_ticks++;
            if(self->transmitter.cur_wait_ack_ticks < self->transmitter.wait_ack_ticks) {
                break;
            }
        }
        if(self->transmitter.retrans_max_value == 0 || self->transmitter.retrans_counter >= self->transmitter.retrans_max_value) {
            _clear_transmitter(&self->transmitter);
            break;
        }
        self->transmitter.retrans_counter++;
        self->transmitter.cur_wait_ack_ticks = 0;
        self->transmitter.state = TRANS_READY;
    } while(0);
}
