/**
 * @file common\utils\simple_fifo\simple_fifo.c
 *
 * Copyright (C) 2022
 *
 * simple_fifo.c is free software: you can redistribute it and/or modify
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
#include "simple_fifo.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
/*---------- type define ----------*/
struct simple_fifo {
    uint32_t capacity;
    uint8_t *pbuf;
    uint32_t head;
    uint32_t tail;
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
/*---------- function ----------*/
simple_fifo_t simple_fifo_new(uint32_t capacity)
{
    simple_fifo_t self = NULL;
    uint32_t wanted_size = sizeof(*self) + capacity + 1;

    assert(capacity);
    self = (simple_fifo_t)__malloc(wanted_size);
    if(self) {
        memset((void *)self, 0, wanted_size);
        self->pbuf = (uint8_t *)self + sizeof(*self);
        self->capacity = capacity + 1;
    }

    return self;
}

void simple_fifo_delete(simple_fifo_t self)
{
    assert(self);
    __free(self);
}

uint32_t simple_fifo_get_remaining_size(simple_fifo_t self)
{
    uint32_t full_tail = 0, available = 0;

    assert(self);
    full_tail = (self->tail + self->capacity - 1) % self->capacity;
    available = (full_tail + self->capacity - self->head) % self->capacity;

    return available;
}

uint32_t simple_fifo_get_available_size(simple_fifo_t self)
{
    assert(self);

    return ((self->head + self->capacity - self->tail) % self->capacity);
}

void simple_fifo_reset(simple_fifo_t self)
{
    assert(self);
    self->head = 0;
    self->tail = 0;
}

uint32_t simple_fifo_push(simple_fifo_t self, const uint8_t *pbuf, uint32_t length)
{
    uint32_t full_tail = 0, len = 0;

    assert(self);
    assert(pbuf);
    full_tail = (self->tail + self->capacity - 1) % self->capacity;
    while(self->head != full_tail && len < length) {
        self->pbuf[self->head] = pbuf[len++];
        self->head = (self->head + 1) % self->capacity;
    }

    return len;
}

uint32_t simple_fifo_pop(simple_fifo_t self, uint8_t *pbuf, uint32_t capacity)
{
    uint32_t len = 0;

    assert(self);
    assert(pbuf);
    while(self->head != self->tail && len < capacity) {
        pbuf[len++] = self->pbuf[self->tail];
        self->tail = (self->tail + 1) % self->capacity;
    }

    return len;
}
