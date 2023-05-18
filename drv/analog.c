/**
 * @file drv\analog.c
 *
 * Copyright (C) 2023
 *
 * analog.c is free software: you can redistribute it and/or modify
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
#include "analog.h"
#include "driver.h"
#include "options.h"

/*---------- macro ----------*/
/*---------- type define ----------*/
/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
static int32_t analog_open(driver_t **pdrv);
static void analog_close(driver_t **pdrv);
static int32_t analog_irq_handler(driver_t **pdrv, uint32_t irq, void *args, uint32_t length);
static int32_t analog_ioctl(driver_t **pdrv, uint32_t cmd, void *args);
static int32_t _ioctl_enable(analog_describe_t *pdesc, void *args);
static int32_t _ioctl_disable(analog_describe_t *pdesc, void *args);
static int32_t _ioctl_get(analog_describe_t *pdesc, void *args);

/*---------- variable ----------*/
DRIVER_DEFINED(analog, analog_open, analog_close, NULL, NULL, analog_ioctl, analog_irq_handler);
static struct protocol_callback ioctl_cbs[] = {
    {IOCTL_ANALOG_ENABLE, _ioctl_enable},
    {IOCTL_ANALOG_DISABLE, _ioctl_disable},
    {IOCTL_ANALOG_GET, _ioctl_get},
};

/*---------- function ----------*/
static int32_t analog_open(driver_t **pdrv)
{
    analog_describe_t *pdesc = NULL;
    int32_t err = CY_E_WRONG_ARGS;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc) {
        err = CY_EOK;
        if(pdesc->ops.init) {
            err = (pdesc->ops.init() ? CY_EOK : CY_ERROR);
        }
    }

    return err;
}

static void analog_close(driver_t **pdrv)
{
    analog_describe_t *pdesc = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc && pdesc->ops.deinit) {
        pdesc->ops.deinit();
    }
}

static int32_t analog_irq_handler(driver_t **pdrv, uint32_t irq, void *args, uint32_t length)
{
    analog_describe_t *pdesc = NULL;
    int32_t err = CY_EOK;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc && pdesc->ops.irq_handler) {
        err = pdesc->ops.irq_handler(irq, args, length);
    }

    return err;
}

static int32_t _ioctl_enable(analog_describe_t *pdesc, void *args)
{
    int32_t err = CY_ERROR;

    if(pdesc->ops.enable) {
        err = (pdesc->ops.enable(true) ? CY_EOK : CY_ERROR);
    }

    return err;
}

static int32_t _ioctl_disable(analog_describe_t *pdesc, void *args)
{
    int32_t err = CY_ERROR;

    if(pdesc->ops.enable) {
        err = (pdesc->ops.enable(false) ? CY_EOK : CY_ERROR);
    }

    return err;
}

static int32_t _ioctl_get(analog_describe_t *pdesc, void *args)
{
    int32_t err = CY_ERROR;
    union analog_ioctl_param *param = (union analog_ioctl_param *)args;

    do {
        if(!param) {
            break;
        }
        if(param->get.channel >= pdesc->number_of_channels) {
            break;
        }
        if(!pdesc->ops.get) {
            break;
        }
        param->get.data = pdesc->ops.get(param->get.channel);
        err = CY_EOK;
    } while(0);

    return err;
}

static int32_t analog_ioctl(driver_t **pdrv, uint32_t cmd, void *args)
{
    analog_describe_t *pdesc = NULL;
    int32_t err = CY_E_WRONG_ARGS;
    int32_t (*cb)(analog_describe_t *, void *) = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(!pdesc) {
            break;
        }
        cb = protocol_callback_find(cmd, ioctl_cbs, ARRAY_SIZE(ioctl_cbs));
        if(!cb) {
            break;
        }
        err = cb(pdesc, args);
    } while(0);

    return err;
}
