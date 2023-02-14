/**
 * @file common\access_control\serial\serial_media_access_control.c
 *
 * Copyright (C) 2022
 *
 * serial_media_access_control.c is free software: you can redistribute it and/or modify
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
#include "serial_media_access_control.h"
#include "halfduplex_serial_media_access_control.h"
#include "fullduplex_serial_media_access_control.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
/*---------- type define ----------*/
typedef struct serial_mac_ops *serial_mac_ops_t;

struct mac_ops {
    serial_mac_t (*new)(uint32_t baudrate, uint32_t recv_capacity, uint32_t trans_capacity, serial_mac_ops_t ops);
    void (*delete)(serial_mac_t handle);
    void (*set_transmitter)(serial_mac_t handle, const uint8_t *pbuf, uint32_t length);
    serial_mac_expection_t (*set_transmitter_cache)(serial_mac_t handle, const uint8_t *pbuf, 
            uint32_t length, uint16_t retrans_count, uint32_t wait_ack_ticks);
    void (*clear_transmitter)(serial_mac_t handle);
    void (*recv_byte)(serial_mac_t handle, uint8_t byte);
    void (*timer_expired)(serial_mac_t handle);
    void (*poll)(serial_mac_t handle);
    void (*called_per_tick)(serial_mac_t handle);
};

struct mac {
    serial_mac_t handle;
    struct mac_ops ops;
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
/*---------- function ----------*/
serial_mac_t serial_mac_new(serial_mac_type_t type, uint32_t baudrate, uint32_t recv_capacity,
        uint32_t trans_capacity, serial_mac_ops_t ops)
{
    serial_mac_t self = NULL;

    do {
        if(type >= SERIAL_MAC_TYPE_BOUND) {
            break;
        }
        self = __malloc(sizeof(*self));
        if(!self) {
            break;
        }
        memset(self, 0, sizeof(*self));
        if(type == SERIAL_MAC_TYPE_HALFDUPLEX) {
            self->ops.new = halfduplex_serial_media_access_controller_new;
            self->ops.delete = halfduplex_serial_media_access_controller_delete;
            self->ops.set_transmitter = halfduplex_serial_mac_set_transmitter;
            self->ops.set_transmitter_cache = halfduplex_serial_mac_set_transmitter_cache;
            self->ops.clear_transmitter = halfduplex_serial_mac_clear_transmitter;
            self->ops.recv_byte = halfduplex_serial_mac_recv_byte;
            self->ops.timer_expired = halfduplex_serial_mac_timer_expired;
            self->ops.poll = halfduplex_serial_mac_poll;
            self->ops.called_per_tick = halfduplex_serial_mac_called_per_tick;
        } else {
            self->ops.new = fullduplex_serial_media_access_controller_new;
            self->ops.delete = fullduplex_serial_media_access_control_delete;
            self->ops.set_transmitter = fullduplex_serial_mac_set_transmitter;
            self->ops.set_transmitter_cache = fullduplex_serial_mac_set_transmitter_cache;
            self->ops.clear_transmitter = fullduplex_serial_mac_clear_transmitter;
            self->ops.recv_byte = fullduplex_serial_mac_recv_byte;
            self->ops.timer_expired = fullduplex_serial_mac_timer_expired;
            self->ops.poll = fullduplex_serial_mac_poll;
            self->ops.called_per_tick = fullduplex_serial_mac_called_per_tick;
        }
        self->handle = self->ops.new(baudrate, recv_capacity, trans_capacity, ops);
        if(!self->handle) {
            __free(self);
            self = NULL;
        }
    } while(0);

    return self;
}

void serial_mac_delete(serial_mac_t self)
{
    assert(self);
    assert(self->ops.delete);
    self->ops.delete(self->handle);
    __free(self);
}

void serial_mac_set_transmitter(serial_mac_t self, const uint8_t *pbuf, uint32_t length)
{
    assert(self);
    assert(self->ops.set_transmitter);
    self->ops.set_transmitter(self->handle, pbuf, length);
}

serial_mac_expection_t serial_mac_set_transmitter_cache(serial_mac_t self, const uint8_t *pbuf, uint32_t length,
        uint16_t retrans_count, uint32_t wait_ack_tick)
{
    assert(self);
    assert(self->ops.set_transmitter_cache);
    return self->ops.set_transmitter_cache(self->handle, pbuf, length, retrans_count, wait_ack_tick);
}

void serial_mac_clear_transmitter(serial_mac_t self)
{
    assert(self);
    assert(self->ops.clear_transmitter);
    self->ops.clear_transmitter(self->handle);
}

void serial_mac_recv_byte(serial_mac_t self, uint8_t byte)
{
    assert(self);
    assert(self->ops.recv_byte);
    self->ops.recv_byte(self->handle, byte);
}

void serial_mac_timer_expired(serial_mac_t self)
{
    assert(self);
    assert(self->ops.timer_expired);
    self->ops.timer_expired(self->handle);
}

void serial_mac_poll(serial_mac_t self)
{
    assert(self);
    assert(self->ops.poll);
    self->ops.poll(self->handle);
}

void serial_mac_called_per_tick(serial_mac_t self)
{
    assert(self);
    assert(self->ops.called_per_tick);
    self->ops.called_per_tick(self->handle);
}
