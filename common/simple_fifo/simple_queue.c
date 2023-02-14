/**
 * @file common\utils\simple_fifo\simple_queue.c
 *
 * Copyright (C) 2022
 *
 * simple_queue.c is free software: you can redistribute it and/or modify
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
#include "simple_queue.h"
#include "simple_fifo.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
/*---------- type define ----------*/
struct simple_queue {
    simple_fifo_t fifo;
    uint32_t member_size;
    uint32_t member_count;
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
/*---------- function ----------*/
simple_queue_t simple_queue_new(uint32_t member_count, uint32_t member_size)
{
    simple_queue_t self = NULL;
    uint32_t wanted_size = sizeof(*self);

    assert(member_count);
    assert(member_size);
    do {
        self = __malloc(wanted_size);
        if(!self) {
            break;
        }
        memset((void *)self, 0, wanted_size);
        wanted_size = member_count * member_size;
        self->fifo = simple_fifo_new(wanted_size);
        if(!self->fifo) {
            __free(self);
            self = NULL;
            break;
        }
        self->member_count = member_count;
        self->member_size = member_size;
    } while(0);

    return self;
}

void simple_queue_delete(simple_queue_t self)
{
    assert(self);
    simple_fifo_delete(self->fifo);
    __free(self);
}

void simple_queue_reset(simple_queue_t self)
{
    assert(self);
    simple_fifo_reset(self->fifo);
}

bool simple_queue_push(simple_queue_t self, const void *member)
{
    bool retval = false;

    assert(self);
    assert(member);
    if(simple_fifo_get_remaining_size(self->fifo) >= self->member_size) {
        simple_fifo_push(self->fifo, (const uint8_t *)member, self->member_size);
        retval = true;
    }

    return retval;
}

bool simple_queue_pop(simple_queue_t self, void *member)
{
    bool retval = false;

    assert(self);
    assert(member);
    if(simple_fifo_get_available_size(self->fifo) >= self->member_size) {
        simple_fifo_pop(self->fifo, member, self->member_size);
        retval = true;
    }

    return retval;
}
