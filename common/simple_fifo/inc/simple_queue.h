/**
 * @file common\utils\simple_fifo\inc\simple_queue.h
 *
 * Copyright (C) 2022
 *
 * simple_queue.h is free software: you can redistribute it and/or modify
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
#ifndef __SIMPLE_QUEUE_H
#define __SIMPLE_QUEUE_H

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
typedef struct simple_queue *simple_queue_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/**
 * @brief Create a simple queue instance, and return ad handle by which the 
 * created queue can be referenced.
 * @param member_count Number of queue member.
 * @param member_size Size of queue member.
 * 
 * @retval If the simple quueue is successfully created then a handle to the newly
 * queue is returned. If the queue cannot be created because there is insufficient
 * heap remaining to allocate the queue structure then NULL is returned.
 */
extern simple_queue_t simple_queue_new(uint32_t member_count, uint32_t member_size);

/**
 * @brief Delete a quque that was previously created using the simple_queue_new() API
 * function.
 * @param self The handle of the queue being deleted.
 * 
 * @retval None
 */
extern void simple_queue_delete(simple_queue_t self);

/**
 * @brief Reset the queue and drop the all queue member which has been pushed.
 * @param self The handle of the queue being reset.
 * 
 * @retval None
 */
extern void simple_queue_reset(simple_queue_t self);

/**
 * @brief Push a queue member to the queue.
 * @param self The handle of the queue being pushed.
 * @param member Queue member pointer.
 * 
 * @retval If member was pushed successfully then true is returned.
 * If member was pushed failure then false is returned.
 */
extern bool simple_queue_push(simple_queue_t self, const void *member);

/**
 * @brief Pop a queue member from the queue.
 * @param self The handle of the queue being poped.
 * @param member Queue member pointer.
 * 
 * @retval If member was poped successfully then true is returned.
 * If member was poped failure then false is returned.
 */
extern bool simple_queue_pop(simple_queue_t self, void *member);

#ifdef __cplusplus
}
#endif
#endif /* __SIMPLE_QUEUE_H */
