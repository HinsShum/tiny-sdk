/**
 * @file drv\ws281x.c
 *
 * Copyright (C) 2024
 *
 * ws281x.c is free software: you can redistribute it and/or modify
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
#include "ws281x.h"
#include "driver.h"
#include "errorno.h"
#include "options.h"

/*---------- macro ----------*/
#define TAG                                         "WS281X"

/*---------- type define ----------*/
/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
static int32_t ws281x_open(driver_t **pdrv);
static void ws281x_close(driver_t **pdrv);
static int32_t ws281x_write(driver_t **pdrv, void *pbuf, uint32_t off, uint32_t numbers);
static int32_t ws281x_read(driver_t **pdrv, void *pbuf, uint32_t off, uint32_t numbers);
static int32_t ws281x_ioctl(driver_t **pdrv, uint32_t cmd, void *args);
static int32_t ws281x_irq_handler(driver_t **pdrv, uint32_t irq_handler, void *args, uint32_t length);
static int32_t _ioctl_refresh(ws281x_describe_t *pdesc, void *args);
static int32_t _ioctl_get_info(ws281x_describe_t *pdesc, void *args);
static int32_t _ioctl_clear_data(ws281x_describe_t *pdesc, void *args);

/*---------- variable ----------*/
DRIVER_DEFINED(ws281x, ws281x_open, ws281x_close, ws281x_write, ws281x_read, ws281x_ioctl, ws281x_irq_handler);
static struct protocol_callback ioctl_cbs[] = {
    {IOCTL_WS281X_REFRESH, _ioctl_refresh},
    {IOCTL_WS281X_GET_INFO, _ioctl_get_info},
    {IOCTL_WS281X_CLEAR_DATA, _ioctl_clear_data},
};

/*---------- function ----------*/
static int32_t ws281x_open(driver_t **pdrv)
{
    int32_t err = CY_E_WRONG_ARGS;
    ws281x_describe_t *pdesc = NULL;

    do {
        assert(pdrv);
        pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
        assert(pdesc);
        if(!pdesc) {
            xlog_tag_error(TAG, "No desc found\n");
            break;
        }
        if(pdesc->ops.init) {
            if(pdesc->ops.init() != true) {
                xlog_tag_error(TAG, "Initialize low level failure\n");
                err = CY_ERROR;
                break;
            }
        }
        err = CY_EOK;
    } while(0);

    return err;
}

static void ws281x_close(driver_t **pdrv)
{
    ws281x_describe_t *pdesc = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc && pdesc->ops.deinit) {
        pdesc->ops.deinit();
    }
}

static int32_t ws281x_write(driver_t **pdrv, void *pbuf, uint32_t off, uint32_t numbers)
{
    int32_t err = CY_E_WRONG_ARGS;
    ws281x_describe_t *pdesc = NULL;
    union ws281x_data *data = (union ws281x_data *)pbuf;

    do {
        assert(pdrv);
        pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
        if(!pdesc) {
            break;
        }
        if(!pbuf || !numbers || off >= pdesc->info.numbers) {
            break;
        }
        numbers = ((off + numbers) > pdesc->info.numbers ? (pdesc->info.numbers - off) : numbers);
        for(uint32_t i = 0; i < numbers; ++i) {
            pdesc->info.data[off + i] = data[i];
        }
        err = (int32_t)numbers;
    } while(0);

    return err;
}

static int32_t ws281x_read(driver_t **pdrv, void *pbuf, uint32_t off, uint32_t numbers)
{
    int32_t err = CY_E_WRONG_ARGS;
    ws281x_describe_t *pdesc = NULL;
    union ws281x_data *data = (union ws281x_data *)pbuf;

    do {
        assert(pdrv);
        pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
        if(!pdesc) {
            break;
        }
        if(!pbuf || !numbers || off >= pdesc->info.numbers) {
            break;
        }
        numbers = ((off + numbers) > pdesc->info.numbers ? (pdesc->info.numbers - off) : numbers);
        for(uint32_t i = 0; i < numbers; ++i) {
            data[i] = pdesc->info.data[off + i];
        }
        err = (int32_t)numbers;
    } while(0);

    return err;
}

static int32_t _ioctl_refresh(ws281x_describe_t *pdesc, void *args)
{
    if(pdesc->ops.refresh) {
        pdesc->ops.refresh();
    }

    return CY_EOK;
}

static int32_t _ioctl_get_info(ws281x_describe_t *pdesc, void *args)
{
    int32_t err = CY_E_WRONG_ARGS;
    ws281x_info_t *info = (ws281x_info_t *)args;

    if(args) {
        *info = pdesc->info;
        err = CY_EOK;
    }

    return err;
}

static int32_t _ioctl_clear_data(ws281x_describe_t *pdesc, void *args)
{
    memset(pdesc->info.data, 0x00, sizeof(union ws281x_data) * pdesc->info.numbers);

    return CY_EOK;
}

static int32_t ws281x_ioctl(driver_t **pdrv, uint32_t cmd, void *args)
{
    int32_t err = CY_E_WRONG_ARGS;
    ws281x_describe_t *pdesc = NULL;
    int32_t (*cb)(ws281x_describe_t *pdesc, void *args) = NULL;

    do {
        assert(pdrv);
        pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
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

static int32_t ws281x_irq_handler(driver_t **pdrv, uint32_t irq_handler, void *args, uint32_t length)
{
    ws281x_describe_t *pdesc = NULL;
    int32_t err = CY_E_WRONG_ARGS;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc && pdesc->ops.irq_handler) {
        err = pdesc->ops.irq_handler(irq_handler, args, length);
    }

    return err;
}
