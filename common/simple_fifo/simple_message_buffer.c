/**
 * @file common\utils\simple_fifo\simple_message_buffer.c
 *
 * Copyright (C) 2022
 *
 * simple_message_buffer.c is free software: you can redistribute it and/or modify
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
#include "simple_message_buffer.h"
#include "simple_fifo.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
/*---------- type define ----------*/
struct simple_message {
    simple_fifo_t fifo;
    uint32_t capacity;
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
/*---------- function ----------*/
simple_message_t simple_message_new(uint32_t size)
{
    simple_message_t self = NULL;

    assert(size);
    do {
        self = __malloc(sizeof(*self));
        if(!self) {
            break;
        }
        memset((void *)self, 0, sizeof(*self));
        self->fifo = simple_fifo_new(size);
        if(!self->fifo) {
            __free(self);
            self = NULL;
            break;
        }
        self->capacity = size;
    } while(0);

    return self;
}

void simple_message_delete(simple_message_t self)
{
    assert(self);
    simple_fifo_delete(self->fifo);
    __free(self);
}

void simple_message_reset(simple_message_t self)
{
    assert(self);
    simple_fifo_reset(self->fifo);
}

bool simple_message_push(simple_message_t self, const void *member, uint32_t length)
{
    bool retval = false;
    uint32_t remaining_size = 0;

    assert(self);
    assert(member);
    remaining_size = simple_fifo_get_remaining_size(self->fifo);
    if(remaining_size >= (length + sizeof(length))) {
        simple_fifo_push(self->fifo, (const uint8_t *)&length, sizeof(length));
        simple_fifo_push(self->fifo, (const uint8_t *)member, length);
        retval = true;
    }

    return retval;
}

static void _drop_message(simple_message_t self, uint32_t drop_length)
{
    uint8_t drop_byte = 0;

    for(uint32_t i = 0; i < drop_length; ++i) {
        simple_fifo_pop(self->fifo, &drop_byte, sizeof(drop_byte));
    }
}

bool simple_message_pop(simple_message_t self, void *member, uint32_t capacity)
{
    bool retval = false;
    uint32_t available_size = 0;
    uint32_t length = 0;

    assert(self);
    assert(member);
    do {
        available_size = simple_fifo_get_available_size(self->fifo);
        if(available_size <= 4) {
            break;
        }
        simple_fifo_pop(self->fifo, (uint8_t *)&length, sizeof(length));
        if(length > capacity) {
            _drop_message(self, length);
            break;
        }
        simple_fifo_pop(self->fifo, (uint8_t *)member, length);
        retval = true;
    } while(0);

    return retval;
}
