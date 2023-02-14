/**
 * @file common/utils/resource_manager/resource_manager.c
 *
 * Copyright (C) 2022
 *
 * resource_manager.c is free software: you can redistribute it and/or modify
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
#include "resource_manager.h"
#include "lists.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
#define TAG                                         "ResourceManager"

/*---------- type define ----------*/
typedef struct resource_manager *resource_manager_t;
struct resource_manager {
    struct resource_manager_base base;
    void *default_ptr;
    struct list_head head;
};

typedef struct resource_node *resource_node_t;
struct resource_node {
    const char *name;
    void *ptr;
    struct list_head node;
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
/*---------- function ----------*/
static bool inline __match_by_name(const char *src, const char *dst)
{
    return (strcmp(src, dst) == 0);
}

static bool _search_node(const resource_manager_t manager, const char *name, resource_node_t node)
{
    bool retval = false;
    resource_node_t p = NULL;

    list_for_each_entry(p, struct resource_node, &manager->head, node) {
        if(__match_by_name(p->name, name)) {
            *node = *p;
            retval = true;
        }
    }

    return retval;
}

static bool _add_resource(const resource_manager_base_t base, const char *name, void *ptr)
{
    bool retval = false;
    resource_manager_t manager = (resource_manager_t)base;
    resource_node_t node = NULL;

    do {
        if(manager == NULL) {
            xlog_tag_error(TAG, "base is invalid, add resource failed\n");
            break;
        }
        node = __malloc(sizeof(struct resource_node));
        if(node == NULL) {
            xlog_tag_error(TAG, "No enough memory to add resource\n");
            break;
        }
        xlog_tag_message(TAG, "Alloc 0x%p for new resource\n", node);
        memset(node, 0, sizeof(struct resource_node));
        if(_search_node(manager, name, node)) {
            xlog_tag_warn(TAG, "%s was registered\n", name);
            break;
        }
        node->name = name;
        node->ptr = ptr;
        list_add_tail(&node->node, &manager->head);
        xlog_tag_message(TAG, "%s[0x%p] add success\n", node->name, node->ptr);
        retval = true;
    } while(0);

    return retval;
}

static bool _remove_resource(const resource_manager_base_t base, const char *name)
{
    bool retval = false;
    struct resource_node node = {0};
    struct list_head *list = NULL;
    resource_manager_t manager = (resource_manager_t)base;

    do {
        if(manager == NULL) {
            xlog_tag_error(TAG, "base is invalid, remove resource failed\n");
            break;
        }
        if(_search_node(manager, name, &node) != true) {
            xlog_tag_error(TAG, "%s was not found\n", name);
            break;
        }
        list = node.node.next->prev;
        list_del(list);
        __free(list_entry(list, struct resource_node, node));
        xlog_tag_message(TAG, "%s remove success\n", name);
        retval = true;
    } while(0);

    return retval;
}

static void *_get_resource(const resource_manager_base_t base, const char *name)
{
    struct resource_node node = {0};
    resource_manager_t manager = (resource_manager_t)base;
    void *ptr = NULL;

    do {
        if(manager == NULL) {
            xlog_tag_error(TAG, "base is invalid, get resource failed\n");
            break;
        }
        ptr = manager->default_ptr;
        if(_search_node(manager, name, &node) != true) {
            xlog_tag_warn(TAG, "%s was not found, return default[0x%p]\n", name, manager->default_ptr);
            break;
        }
        ptr = node.ptr;
        xlog_tag_message(TAG, "%s[0x%p] was found\n", name, ptr);
    } while(0);

    return ptr;
}

static void *_get_resource_careful(const resource_manager_base_t base, const char *name)
{
    struct resource_node node = {0};
    resource_manager_t manager = (resource_manager_t)base;
    void *ptr = NULL;

    do {
        if(manager == NULL) {
            break;
        }
        ptr = manager->default_ptr;
        if(_search_node(manager, name, &node) != true) {
            break;
        }
        ptr = node.ptr;
    } while(0);

    return ptr;
}

static void _set_default(resource_manager_base_t base, void *ptr)
{
    resource_manager_t manager = (resource_manager_t)base;

    do {
        if(manager == NULL) {
            xlog_tag_error(TAG, "base is invalid args, set default failed\n");
            break;
        }
        manager->default_ptr = ptr;
        xlog_tag_message(TAG, "set [0x%p] to default\n", manager->default_ptr);
    } while(0);
}

resource_manager_base_t resource_manager_create(void)
{
    resource_manager_t manager = NULL;

    do {
        manager = __malloc(sizeof(struct resource_manager));
        if(manager == NULL) {
            xlog_tag_error(TAG, "No enough memory to create resource manager\n");
            break;
        }
        memset(manager, 0, sizeof(struct resource_manager));
        manager->base.add_resource = _add_resource;
        manager->base.remove_resource = _remove_resource;
        manager->base.get_resource = _get_resource;
        manager->base.get_resource_careful = _get_resource_careful;
        manager->base.set_default = _set_default;
        INIT_LIST_HEAD(&manager->head);
    } while(0);

    return (resource_manager_base_t)manager;
}

void resource_manager_destroy(resource_manager_base_t base)
{
    resource_manager_t manager = (resource_manager_t)base;
    resource_node_t p = NULL;
    resource_node_t n = NULL;

    do {
        if(manager == NULL) {
            xlog_tag_error(TAG, "base is invalid, destroy resource manager failed\n");
            break;
        }
        if(list_empty(&manager->head) != true) {
            list_for_each_entry_safe(p, n, struct resource_node, &manager->head, node) {
                list_del(&p->node);
                __free(p);
            }
        }
        __free(manager);
        xlog_tag_message(TAG, "destroy resource manager success\n");
    } while(0);
}
