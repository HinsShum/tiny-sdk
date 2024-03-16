/**
 * @file common\simple_fifo\inc\simple_semaphore.h
 *
 * Copyright (C) 2024
 *
 * simple_semaphore.h is free software: you can redistribute it and/or modify
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
#ifndef __SIMPLE_SEMAPHORE_H
#define __SIMPLE_SEMAPHORE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*---------- macro ----------*/
/**
 * @brief Create a simple binary semaphore instance and return a handle by which the
 * created semaphore can be referenced.
 * 
 * @return If the simple semaphore is successfully created then a handle to the newly
 * semaphore is returned. If the semaphore cannot be created because there is insufficient
 * heap remaining to allocate the semaphore structure then NULL is returned.
 */
#define simple_semaphore_binary_new()               simple_semaphore_new(1)

/*---------- type define ----------*/
typedef struct simple_semaphore *simple_semaphore_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/**
 * @brief Create a simple semaphore instance and return a handle by which the
 * created semaphore can be referenced.
 * @param count Number of semaphore member. 
 * 
 * @return If the simple semaphore is successfully created then a handle to the newly
 * semaphore is returned. If the semaphore cannot be created because there is insufficient
 * heap remaining to allocate the semaphore structure then NULL is returned.
 */
extern simple_semaphore_t simple_semaphore_new(uint32_t count);

/**
 * @brief Delete a semaphore that was previously created using the simple_semaphore_new() API
 * function.
 * @param self The handle of the semaphore being deleted.
 * 
 * @retval None 
 */
extern void simple_semaphore_delete(simple_semaphore_t self);

/**
 * @brief Reset the semaphore and drop the all semaphore member which has been pushed.
 * @param self The handle of the sempahore being reset.
 * 
 * @retval None
 */
extern void simple_semaphore_reset(simple_semaphore_t self);

/**
 * @brief Push a semaphore member to the semaphore.
 * @param self The handle of the semaphore being pushed.
 * 
 * @return If member was pushed successfully the true is returned.
 * If member was pushed failure then false is returned.
 */
extern bool simple_semaphore_push(simple_semaphore_t self);

/**
 * @brief Pop a semaphore member from the semaphore.
 * @param self The handle of the semaphore being poped.
 * 
 * @return If member was poped successfully then true is returned.
 * If member was poped failure then false is returned.
 */
extern bool simple_semaphore_pop(simple_semaphore_t self);

#ifdef __cplusplus
}
#endif
#endif /* __SIMPLE_SEMAPHORE_H */
