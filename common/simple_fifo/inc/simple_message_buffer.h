/**
 * @file common\utils\simple_fifo\inc\simple_message_buffer.h
 *
 * Copyright (C) 2022
 *
 * simple_message_buffer.h is free software: you can redistribute it and/or modify
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
#ifndef __SIMPLE_MESSAGE_BUFFER_H
#define __SIMPLE_MESSAGE_BUFFER_H

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
typedef struct simple_message *simple_message_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/**
 * @brief Create a simple message instance and return a handle by which
 * the created message can be reference.
 * @param size Capacity of message.
 * 
 * @retval If the simple message is successfully created then a handle to
 * the newly message is returned. If the message cannot be created because there
 * is insufficient heap remaining to allocate the message structure then NULL is
 * returned.
 */
extern simple_message_t simple_message_new(uint32_t size);

/**
 * @brief Delete a message that was previously created using the simple_message_new()
 * API function.
 * @param self The handle of the message being deleted.
 * 
 * @retval None
 */
extern void simple_message_delete(simple_message_t self);

/**
 * @brief Reset the message and drop the all message which has been pushed.
 * @param self The handle of the message being reset.
 * 
 * @retval None
 */
extern void simple_message_reset(simple_message_t self);

/**
 * @brief Push a message to the message handle.
 * @param self The handle of message.
 * @param member The message member pointer.
 * @param length The length of message content.
 * 
 * @retval If message was pushed successfully then true is returned.
 * If not, false is returned.
 */
extern bool simple_message_push(simple_message_t self, const void *member, uint32_t length);

/**
 * @brief Pop a message content from message handle.
 * @param self The handle of message.
 * @param member The message member pointer.
 * @param capacity The message member max size.
 * 
 * @retval If member was poped successfully the true is returned.
 * If not, false is retrurned.
 */
extern bool simple_message_pop(simple_message_t self, void *member, uint32_t capacity);

#ifdef __cplusplus
}
#endif
#endif /* __SIMPLE_MESSAGE_BUFFER_H */
