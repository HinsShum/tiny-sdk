/**
 * @file common\simple_fifo\simple_semaphore.c
 *
 * Copyright (C) 2024
 *
 * simple_semaphore.c is free software: you can redistribute it and/or modify
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
#include "simple_semaphore.h"
#include "simple_fifo.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
/*---------- type define ----------*/
struct simple_semaphore {
    simple_fifo_t fifo;
    uint32_t member_size;
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
/*---------- function ----------*/
simple_semaphore_t simple_semaphore_new(uint32_t count)
{
    simple_semaphore_t self = NULL;
    uint32_t wanted_size = sizeof(*self);

    assert(count);
    do {
        self = __malloc(wanted_size);
        if(!self) {
            break;
        }
        memset((void *)self, 0, wanted_size);
        wanted_size = sizeof(uint32_t) * count;
        self->fifo = simple_fifo_new(wanted_size);
        if(!self->fifo) {
            __free(self);
            self = NULL;
            break;
        }
        self->member_size = sizeof(uint32_t);
    } while(0);

    return self;
}

void simple_semaphore_delete(simple_semaphore_t self)
{
    assert(self);
    simple_fifo_delete(self->fifo);
    __free(self);
}

void simple_semaphore_reset(simple_semaphore_t self)
{
    assert(self);
    simple_fifo_reset(self->fifo);
}

bool simple_semaphore_push(simple_semaphore_t self)
{
    bool err = false;
    uint32_t val = 0;

    (void)val;
    assert(self);
    if(simple_fifo_get_remaining_size(self->fifo) >= self->member_size) {
        simple_fifo_push(self->fifo, (const uint8_t *)&val, self->member_size);
        err = true;
    }

    return err;
}

bool simple_semaphore_pop(simple_semaphore_t self)
{
    bool err = false;
    uint32_t val = 0;

    (void)val;
    assert(self);
    if(simple_fifo_get_available_size(self->fifo) >= self->member_size) {
        simple_fifo_pop(self->fifo, (void *)&val, self->member_size);
        err = true;
    }

    return err;
}
