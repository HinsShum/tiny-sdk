/**
 * @file common\utils\simple_fifo\inc\simple_fifo.h
 *
 * Copyright (C) 2022
 *
 * simple_fifo.h is free software: you can redistribute it and/or modify
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
#ifndef __SIMPLE_FIFO_H
#define __SIMPLE_FIFO_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*---------- macro ----------*/
/*---------- type define ----------*/
typedef struct simple_fifo *simple_fifo_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/**
 * @brief Create a simple fifo instance, and return a handle by which the
 * created fifo can be referenced.
 * @param capacity Simple fifo will create a buffer with a size of capacity bytes.
 * 
 * @retval If the simple fifo is successfully created then a handle to the newly fifo
 * is returned. If the fifo cannot be created because there is insufficient heap remaining
 * to allocate the fifo structure then NULL is returned.
 */
extern simple_fifo_t simple_fifo_new(uint32_t capacity);

/**
 * @brief Delete a fifo that was previously created using the simple_fifo_new() API function.
 * @param self The handle of the fifo being deleted.
 * 
 * @retval None
 */
extern void simple_fifo_delete(simple_fifo_t self);

/**
 * @brief Query the fifo remaining size.
 * @param self The handle of the fifo being queried.
 * 
 * @retval The remaining size of the fifo.
 */
extern uint32_t simple_fifo_get_remaining_size(simple_fifo_t self);

/**
 * @brief Query the fifo available size.
 * @param self The handle of the fifo being queried.
 * 
 * @retval The available size of the fifo.
 */
extern uint32_t simple_fifo_get_available_size(simple_fifo_t self);

/**
 * @brief Reset the fifo and drop the all bytes which has been received.
 * @param self The handle of the fifo being reset.
 * 
 * @retval None
 */
extern void simple_fifo_reset(simple_fifo_t self);

/**
 * @brief Push serial bytes to the fifo.
 * @param self The handle of the fifo being pushed.
 * @param pbuf Serial bytes.
 * @param length The length of the bytes.
 * 
 * @retval The bytes length which has been pushed is returned.
 */
extern uint32_t simple_fifo_push(simple_fifo_t self, const uint8_t *pbuf, uint32_t length);

/**
 * @brief Pop a serial bytes from the fifo.
 * @param self The handle of the fifo being poped.
 * @param pbuf The container for storing serial bytes.
 * 
 * @retval The bytes length which has been poped is returned.
 */
extern uint32_t simple_fifo_pop(simple_fifo_t self, uint8_t *pbuf, uint32_t capacity);

#ifdef __cplusplus
}
#endif
#endif /* __SIMPLE_FIFO_H */
