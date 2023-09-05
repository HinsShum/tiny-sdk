/**
 * @file media_access_control\mia\mia_mac.c
 *
 * Copyright (C) 2023
 *
 * mia_mac.c is free software: you can redistribute it and/or modify
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
#include "mia_mac.h"
#include "pingpong_buffer.h"
#include "options.h"
#include <string.h>
#include <stdlib.h>

/*---------- macro ----------*/
#define DISF                                        __ms2ticks(100)

/*---------- type define ----------*/
typedef enum {
    BUS_IDLE = 0,
    BUS_BUSY = !BUS_IDLE
} bus_state_t;

typedef enum {
    TRANS_IDLE,
    TRANS_READY,
    TRANS_BUSY,
    TRANS_WAIT_ACK,
} trans_state_t;

struct mac_bus {
    bool fault;
    bus_state_t state;
    uint32_t disf;
    uint16_t backoff_counter;
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

struct mac {
    mia_phy_t phy;
    uint8_t resp[24];
    struct mac_bus bus;
    struct mac_receive *preceiver;
    struct mac_transmit transmitter;
    struct mac_process processer;
    struct pingpong_buffer pingpong;
    struct {
        uint32_t disf;
        bool (*io_init)(void);
        bool (*timer_init)(uint32_t us);
        void (*timer_ctrl)(bool enable);
        bool (*event_init)(void);
        bool (*event_post)(mia_mac_evt_t evt, bool protected);
        bool (*event_get)(mia_mac_evt_t *pevt);
        void (*bus_fault)(bool fault);
        void (*receive_packet_parse)(const uint8_t *recv_buf, uint32_t recv_length, const uint8_t *trans_buf, uint32_t trans_length);
    } ops;
};

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

static inline bool _mac_bus_busy(mia_mac_t self)
{
    return (BUS_BUSY == self->bus.state);
}

static void _mac_bus_lock(mia_mac_t self)
{
    self->bus.state = BUS_BUSY;
}

static void _mac_bus_unlock(mia_mac_t self)
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

static mia_mac_expection_t _port_level_init(uint32_t baudrate, mia_mac_ops_t ops)
{
    mia_mac_expection_t err = MIA_MAC_EX_ERROR;
    uint32_t us = 0;

    do {
        if(!ops->io_init()) {
            break;
        }
        if(!ops->event_init()) {
            break;
        }
        us = (1000000UL / baudrate) / 8;
        if(!ops->timer_init(us)) {
            break;
        }
        ops->timer_ctrl(true);
        err = MIA_MAC_EX_NONE;
    } while(0);

    return err;
}

mia_mac_t mia_mac_new(uint32_t baudrate, uint32_t recv_capacity, uint32_t trans_capacity, mia_mac_ops_t ops)
{
    uint8_t *recv_buf0 = NULL, *recv_buf1 = NULL;
    mia_mac_t self = NULL;
    struct mac_receive *preceiver = NULL;
    uint32_t wanted_size = 0;
    struct mia_phy_ops phy_ops = {0};

    assert(recv_capacity);
    assert(trans_capacity);
    assert(ops);
    assert(ops->set_tx_bit);
    assert(ops->get_rx_bit);
    assert(ops->monitor_start_bit);
    assert(ops->io_init);
    assert(ops->timer_init);
    assert(ops->timer_ctrl);
    assert(ops->event_init);
    assert(ops->event_get);
    assert(ops->event_post);
    assert(ops->bus_fault);
    assert(ops->receive_packet_parse);
    do {
        if(!recv_capacity || !trans_capacity) {
            break;
        }
        if(MIA_MAC_EX_NONE != _port_level_init(baudrate, ops)) {
            break;
        }
        wanted_size = sizeof(*self) + (recv_capacity + sizeof(*preceiver)) * 2 + recv_capacity + trans_capacity;
        self = __malloc(wanted_size);
        if(!self) {
            break;
        }
        memset(self, 0, wanted_size);
        /* new mia phy */
        phy_ops.get_bit = ops->get_rx_bit;
        phy_ops.set_bit = ops->set_tx_bit;
        phy_ops.monitor_start_bit = ops->monitor_start_bit;
        self->phy = mia_phy_new(baudrate, &phy_ops);
        assert(self->phy);
        /* configure ops callback pointer */
        self->ops.io_init = ops->io_init;
        self->ops.timer_init = ops->timer_init;
        self->ops.timer_ctrl = ops->timer_ctrl;
        self->ops.event_init = ops->event_init;
        self->ops.event_get = ops->event_get;
        self->ops.event_post = ops->event_post;
        self->ops.bus_fault = ops->bus_fault;
        self->ops.receive_packet_parse = ops->receive_packet_parse;
        self->ops.disf = (ops->disf ? ops->disf : DISF);
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
        mia_phy_set_recv_buf(self->phy, self->preceiver->pbuf, self->preceiver->capacity);
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

void mia_mac_delete(mia_mac_t self)
{
    assert(self);
    assert(self->ops.timer_ctrl);
    assert(self->phy);
    mia_phy_delete(self->phy);
    self->ops.timer_ctrl(false);
    __free(self);
}

void mia_mac_set_transmitter(mia_mac_t self, const uint8_t *pbuf, uint32_t length)
{
    trans_state_t old_state = TRANS_IDLE;

    assert(self);
    assert(self->phy);
    if(!self->bus.fault && length <= ARRAY_SIZE(self->resp)) {
        old_state = self->transmitter.state;
        _mac_bus_lock(self);
        self->transmitter.state = TRANS_BUSY;
        memcpy(self->resp, pbuf, length);
        mia_phy_start_sending(self->phy, self->resp, (uint16_t)length);
        self->bus.disf = self->ops.disf;
        self->transmitter.state = old_state;
#ifdef CONFIG_MIA_MAC_DEBUG
        PRINT_BUFFER_CONTENT(COLOR_WHITE, "[MIA]W", pbuf, length);
#endif
    }
}

mia_mac_expection_t mia_mac_set_transmitter_cache(mia_mac_t self, const uint8_t *pbuf, uint32_t length, uint16_t retrans_count)
{
    mia_mac_expection_t err = MIA_MAC_EX_TRANS_BUSY;

    assert(self);
    assert(pbuf);
    assert(length);
    do {
        if(self->transmitter.state != TRANS_IDLE) {
            break;
        }
        if(length > self->transmitter.capacity) {
            err = MIA_MAC_EX_ERROR;
            break;
        }
        memcpy(self->transmitter.pbuf, pbuf, length);
        self->transmitter.pos = length;
        self->transmitter.retrans_counter = 0;
        self->transmitter.retrans_max_value = retrans_count;
        self->transmitter.state = TRANS_READY;
        err = MIA_MAC_EX_NONE;
    } while(0);

    return err;
}

void mia_mac_clear_transmitter(mia_mac_t self)
{
    assert(self);
    _clear_transmitter(&self->transmitter);
}

void mia_mac_start_bit_detected(mia_mac_t self)
{
    assert(self);
    assert(self->phy);
    _mac_bus_lock(self);
    mia_phy_start_recving(self->phy);
}

void mia_mac_timer_expired(mia_mac_t self)
{
    mia_phy_expection_t err = MIA_PHY_EX_NONE;

    assert(self);
    assert(self->phy);
    assert(self->ops.event_post);
    do {
        err = mia_phy_recv_polling(self->phy);
        if(err == MIA_PHY_EX_RECV_OK) {
            /* choose a clean buffer ready to receive new data */
            pingpong_buffer_set_write_done(&self->pingpong);
            pingpong_buffer_get_read_buf(&self->pingpong, (void **)&self->processer.preceiver);
            pingpong_buffer_get_write_buf(&self->pingpong, (void **)&self->preceiver);
            mia_phy_get_recv_buf(self->phy, (uint16_t *)&self->processer.preceiver->pos);
            mia_phy_set_recv_buf(self->phy, self->preceiver->pbuf, self->preceiver->capacity);
            /* post received event */
            self->ops.event_post(MIA_MAC_EVT_RECEIVED, true);
            /* recover bus */
            self->bus.disf = self->ops.disf;
            _mac_bus_unlock(self);
            break;
        }
        if(err == MIA_PHY_EX_RECV_FAULT) {
            /* recover bus */
            self->bus.disf = self->ops.disf;
            _mac_bus_unlock(self);
            break;
        }
        err = mia_phy_send_polling(self->phy);
        if(err != MIA_PHY_EX_NONE) {
            /* recover bus */
            self->bus.disf = self->ops.disf;
            _mac_bus_unlock(self);
            /* set transmitter state */
            self->transmitter.state = TRANS_WAIT_ACK;
            break;
        }
        err = mia_phy_monitor_polling(self->phy);
        if(err == MIA_PHY_EX_BUS_FAULT && !self->bus.fault) {
            self->bus.fault = true;
            /* lock bus */
            _mac_bus_lock(self);
            /* post bus fault event */
            self->ops.event_post(MIA_MAC_EVT_BUS_FAULT, true);
        } else if(err == MIA_PHY_EX_NONE && self->bus.fault) {
            self->bus.fault = false;
            /* post bus recover event */
            self->ops.event_post(MIA_MAC_EVT_BUS_RECOVER, true);
            /* recover bus */
            self->bus.disf = self->ops.disf;
            _mac_bus_unlock(self);
        }
    } while(0);
}

void mia_mac_polling(mia_mac_t self)
{
    mia_mac_evt_t evt = MIA_MAC_EVT_BUS_FAULT;

    assert(self);
    assert(self->ops.event_get);
    assert(self->ops.receive_packet_parse);
    assert(self->phy);
    if(self->ops.event_get(&evt)) {
        switch(evt) {
            case MIA_MAC_EVT_RECEIVED:
#ifdef CONFIG_MIA_MAC_DEBUG
                PRINT_BUFFER_CONTENT(COLOR_WHITE, "[MIA]R", self->processer.preceiver->pbuf, self->processer.preceiver->pos);
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
            case MIA_MAC_EVT_TRANSMITTER_READY:
                if(!self->bus.fault) {
                    self->bus.disf = self->ops.disf;
                    self->transmitter.state = TRANS_BUSY;
                    mia_phy_start_sending(self->phy, self->transmitter.pbuf, self->transmitter.pos);
#ifdef CONFIG_MIA_MAC_DEBUG
                    PRINT_BUFFER_CONTENT(COLOR_WHITE, "[MIA]W", self->transmitter.pbuf, self->transmitter.pos);
#endif
                }
                break;
            case MIA_MAC_EVT_BUS_FAULT:
                self->ops.bus_fault(true);
                break;
            case MIA_MAC_EVT_BUS_RECOVER:
                self->ops.bus_fault(false);
                break;
            default:
                break;
        }
    }
}

void mia_mac_called_per_tick(mia_mac_t self)
{
    assert(self);
    assert(self->ops.event_post);
    do {
        if(self->bus.fault) {
            break;
        }
        if(_mac_bus_busy(self)) {
            break;
        }
        if(self->transmitter.state == TRANS_IDLE || self->transmitter.state == TRANS_BUSY) {
            break;
        }
        if(self->bus.disf) {
            self->bus.disf--;
            if(self->bus.disf) {
                break;
            }
        }
        if(self->transmitter.state == TRANS_WAIT_ACK) {
            uint16_t retrans_count = self->transmitter.retrans_counter + 1;
            if(self->transmitter.retrans_max_value == 0 || retrans_count > self->transmitter.retrans_max_value) {
                _clear_transmitter(&self->transmitter);
                break;
            }
            self->transmitter.retrans_counter++;
            self->transmitter.state = TRANS_READY;
        }
        if(!self->bus.backoff_counter) {
            self->bus.backoff_counter = _get_random_backoff(self->transmitter.retrans_counter);
            break;
        }
        self->bus.backoff_counter--;
        if(!self->bus.backoff_counter) {
            self->ops.event_post(MIA_MAC_EVT_TRANSMITTER_READY, true);
            _mac_bus_lock(self);
        }
    } while(0);
}
