/**
 * @file common/utils/resource_manager/inc/resource_manager.h
 *
 * Copyright (C) 2022
 *
 * resource_manager.h is free software: you can redistribute it and/or modify
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
#ifndef __RESOURCE_MANAGER_H
#define __RESOURCE_MANAGER_H

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
typedef struct resource_manager_base *resource_manager_base_t;
struct resource_manager_base {
    bool (*add_resource)(const resource_manager_base_t base, const char *name, void *ptr);
    bool (*remove_resource)(const resource_manager_base_t base, const char *name);
    void *(*get_resource)(const resource_manager_base_t base, const char *name);
    void *(*get_resource_careful)(const resource_manager_base_t base, const char *name);
    void (*set_default)(resource_manager_base_t base, void *ptr);
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/**
 * @brief Create a container for resources.
 * 
 * @retval If the container is successfully created then a handle to the newly created container
 * is returned. If the container cannot be created because there is insufficient heap remaining
 * to allocate the container structers then NULL is returned.
 */
extern resource_manager_base_t resource_manager_create(void);

/**
 * @brief Destroy a container that was previously created using the resource_manager_create() API
 * function.
 * @param base The handle of the container being destroyed.
 * 
 * @retval None
 */
extern void resource_manager_destroy(resource_manager_base_t base);

#ifdef __cplusplus
}
#endif
#endif /* __RESOURCE_MANAGER_H */