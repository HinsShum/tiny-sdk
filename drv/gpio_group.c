/**
 * @file drv\gpio.c
 *
 * Copyright (C) 2021
 *
 * gpio.c is free software: you can redistribute it and/or modify
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
#include "gpio_group.h"
#include "driver.h"
#include "errorno.h"
#include "options.h"

/*---------- macro ----------*/
#define TAG                                         "GPIO_GROUP"

/*---------- variable prototype ----------*/
static int32_t gpio_open(driver_t **pdrv);
static void gpio_close(driver_t **pdrv);
static int32_t gpio_ioctl(driver_t **pdrv, uint32_t cmd, void *args);

static int32_t __ioctl_get(gpio_group_describe_t *pdesc,unsigned char num, void *args);
static int32_t __ioctl_set(gpio_group_describe_t *pdesc,unsigned char num, void *args);

/*---------- function prototype ----------*/
/*---------- type define ----------*/
typedef int32_t (*ioctl_cb_func_t)(gpio_group_describe_t *pdesc,unsigned char num,void *args);
typedef struct {
    uint32_t ioctl_cmd;
    ioctl_cb_func_t cb;
} ioctl_cb_t;

/*---------- variable ----------*/
DRIVER_DEFINED(gpio_group, gpio_open, gpio_close, NULL, NULL, gpio_ioctl, NULL);

static ioctl_cb_t ioctl_cb_array[] = {
    {IOCTL_GROUP_GET, __ioctl_get},
    {IOCTL_GROUP_SET, __ioctl_set},
};

/*---------- function ----------*/
static int32_t gpio_open(driver_t **pdrv)
{
    gpio_group_describe_t *pdesc = NULL;
    int32_t retval = CY_E_WRONG_ARGS;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(!pdesc) {
            xlog_tag_error(TAG, "device has not bind describe field\n");
            break;
        }
        retval = CY_EOK;
        if(pdesc->ops.init) {
            if(!pdesc->ops.init()) {
                retval = CY_ERROR;
                xlog_tag_warn(TAG, "initialize failed\n");
            }
        }
    } while(0);

    return retval;
}

static void gpio_close(driver_t **pdrv)
{
    gpio_group_describe_t *pdesc = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc && pdesc->ops.deinit) {
        pdesc->ops.deinit();
    }
}

static int32_t __ioctl_get(gpio_group_describe_t *pdesc,unsigned char num, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    unsigned char *val = (unsigned char *)args;

    if(!args) {
        xlog_tag_error(TAG, "Args format error, can not get gpio value\n");
    } else {
        if(pdesc->ops.get) {
            *val = pdesc->ops.get(*val);
            retval = CY_EOK;
        } else {
            retval = CY_ERROR;
        }
    }

    return retval;
}

static int32_t __ioctl_set(gpio_group_describe_t *pdesc,unsigned char num, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    unsigned char *val = (unsigned char *)args;

    if(!args) {
        xlog_tag_error(TAG, "Args format error, can not set gpio value\n");
    } else {
        if(pdesc->ops.set) {
            pdesc->ops.set(num,*val);
            retval = CY_EOK;
        } else {
            retval = CY_ERROR;
        }
    }

    return retval;
}
static ioctl_cb_func_t __ioctl_cb_func_find(uint32_t ioctl_cmd)
{
    ioctl_cb_func_t cb = NULL;

    for(uint32_t i = 0; i < ARRAY_SIZE(ioctl_cb_array); ++i) {
        if(ioctl_cb_array[i].ioctl_cmd == ioctl_cmd) {
            cb = ioctl_cb_array[i].cb;
            break;
        }
    }

    return cb;
}
//cmd 高八位 序列号、  低八位 实际cmd 
static int32_t gpio_ioctl(driver_t **pdrv, uint32_t cmd, void *args)
{
    gpio_group_describe_t *pdesc = NULL;
    int32_t retval = CY_E_WRONG_ARGS;
    ioctl_cb_func_t cb = NULL;
	uint8_t gpio_group_index = (cmd>>8) & 0xFF;
    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(!pdesc) {
            xlog_tag_error(TAG, "driver has not bind describe field\n");
            break;
        }
        if(NULL == (cb = (__ioctl_cb_func_find(cmd)))) {
            xlog_tag_error(TAG, "driver not support cmd(%08X)\n", cmd);
            break;
        }
        retval = cb(pdesc,gpio_group_index,args);
    } while(0);

    return retval;
}

