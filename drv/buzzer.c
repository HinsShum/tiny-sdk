/**
 * @file drv\buzzer.c
 *
 * Copyright (C) 2021
 *
 * buzzer.c is free software: you can redistribute it and/or modify
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
#include "buzzer.h"
#include "driver.h"
#include "errorno.h"
#include "options.h"

/*---------- macro ----------*/
#define TAG                                         "Buzzer"

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
static int32_t buzzer_open(driver_t **pdrv);
static void buzzer_close(driver_t **pdrv);
static int32_t buzzer_ioctl(driver_t **pdrv, uint32_t cmd, void *args);

static int32_t __ioctl_turn_on(buzzer_describe_t *pdesc, void *args);
static int32_t __ioctl_turn_off(buzzer_describe_t *pdesc, void *args);
static int32_t __ioctl_toggle(buzzer_describe_t *pdesc, void *args);
static int32_t __ioctl_toggle_once(buzzer_describe_t *pdesc, void *args);
static int32_t __ioctl_set_toggle(buzzer_describe_t *pdesc, void *args);
static int32_t __ioctl_get_toggle(buzzer_describe_t *pdesc, void *args);
static int32_t __ioctl_get_status(buzzer_describe_t *pdesc, void *args);
static int32_t __ioctl_set_freq(buzzer_describe_t *pdesc, void *args);
static int32_t __ioctl_get_freq(buzzer_describe_t *pdesc, void *args);
static int32_t __ioctl_set_duty(buzzer_describe_t *pdesc, void *args);
static int32_t __ioctl_get_duty(buzzer_describe_t *pdesc, void *args);

/*---------- type define ----------*/
/*---------- variable ----------*/
DRIVER_DEFINED(buzzer, buzzer_open, buzzer_close, NULL, NULL, buzzer_ioctl, NULL);

static struct protocol_callback ioctl_cb_array[] = {
    {IOCTL_BUZZER_ON, __ioctl_turn_on},
    {IOCTL_BUZZER_OFF, __ioctl_turn_off},
    {IOCTL_BUZZER_TOGGLE, __ioctl_toggle},
    {IOCTL_BUZZER_TOGGLE_ONCE, __ioctl_toggle_once},
    {IOCTL_BUZZER_SET_TOGGLE, __ioctl_set_toggle},
    {IOCTL_BUZZER_GET_TOGGLE, __ioctl_get_toggle},
    {IOCTL_BUZZER_GET_STATUS, __ioctl_get_status},
    {IOCTL_BUZZER_SET_FREQ, __ioctl_set_freq},
    {IOCTL_BUZZER_GET_FREQ, __ioctl_get_freq},
    {IOCTL_BUZZER_SET_DUTY, __ioctl_set_duty},
    {IOCTL_BUZZER_GET_DUTY, __ioctl_get_duty},
};

/*---------- function ----------*/
static int32_t buzzer_open(driver_t **pdrv)
{
    buzzer_describe_t *pdesc = NULL;
    int32_t retval = CY_EOK;

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
        if(pdesc->ops.ctrl) {
            pdesc->ops.ctrl(false);
        }
    } while(0);

    return retval;
}

static void buzzer_close(driver_t **pdrv)
{
    buzzer_describe_t *pdesc = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc) {
        if(pdesc->ops.ctrl) {
            pdesc->ops.ctrl(false);
        }
        if(pdesc->ops.deinit) {
            pdesc->ops.deinit();
        }
    }
}

static int32_t __ioctl_turn_on(buzzer_describe_t *pdesc, void *args)
{
    int32_t err = CY_EOK;

    if(pdesc->ops.ctrl) {
        if(!pdesc->ops.ctrl(true)) {
            err = CY_ERROR;
            xlog_tag_error(TAG, "Turn on buzzer failure\n");
        }
        pdesc->toggle.millisecond = 0;
        pdesc->toggle.count = 0;
    }

    return err;
}

static int32_t __ioctl_turn_off(buzzer_describe_t *pdesc, void *args)
{
    int32_t err = CY_EOK;

    if(pdesc->ops.ctrl) {
        if(!pdesc->ops.ctrl(false)) {
            err = CY_ERROR;
            xlog_tag_error(TAG, "Turn off buzzer failure\n");
        }
        pdesc->toggle.millisecond = 0;
        pdesc->toggle.count = 0;
    }

    return err;
}

static int32_t __ioctl_toggle_once(buzzer_describe_t *pdesc, void *args)
{
    int32_t err = CY_EOK;

    if(pdesc->ops.toggle) {
        if(!pdesc->ops.toggle()) {
            err = CY_ERROR;
            xlog_tag_error(TAG, "Toggle buzzer failure\n");
        }
        pdesc->toggle.millisecond = 0;
        pdesc->toggle.count = 0;
    }

    return err;
}

static int32_t __ioctl_toggle(buzzer_describe_t *pdesc, void *args)
{
    int32_t err = CY_EOK;

    do {
        if(!pdesc->ops.toggle) {
            break;
        }
        if(!pdesc->toggle.millisecond || !pdesc->toggle.count) {
            break;
        }
        if(!(__get_ticks_from_isr() % pdesc->toggle.millisecond)) {
            if(!pdesc->ops.toggle()) {
                err = CY_ERROR;
                xlog_tag_error(TAG, "Toggle buzzer failure\n");
                break;
            }
            if(pdesc->toggle.count && pdesc->toggle.count != BUZZER_TOGGLE_COUNT_MAX) {
                pdesc->toggle.count--;
            }
        }
    } while(0);

    return err;
}

static int32_t __ioctl_set_toggle(buzzer_describe_t *pdesc, void *args)
{
    int32_t err = CY_E_WRONG_ARGS;
    buzzer_toggle_t *toggle = (buzzer_toggle_t *)args;

    if(!args) {
        xlog_tag_error(TAG, "Args format error, can not set buzzer toggle\n");
    } else {
        pdesc->toggle = *toggle;
        err = CY_EOK;
    }

    return err;
}

static int32_t __ioctl_get_toggle(buzzer_describe_t *pdesc, void *args)
{
    int32_t err = CY_E_WRONG_ARGS;
    buzzer_toggle_t *toggle = (buzzer_toggle_t *)args;

    if(!args) {
        xlog_tag_error(TAG, "Args format error, can not get buzzer cycle\n");
    } else {
        *toggle = pdesc->toggle;
        err = CY_EOK;
    }

    return err;
}

static int32_t __ioctl_get_status(buzzer_describe_t *pdesc, void *args)
{
    int32_t err = CY_E_WRONG_ARGS;
    bool *pstatus = (bool *)args;

    do {
        if(!args) {
            xlog_tag_error(TAG, "Args is NULL, no memory to store the buzzer status\n");
            break;
        }
        if(!pdesc->ops.get) {
            xlog_tag_error(TAG, "Driver has no get ops\n");
            break;
        }
        *pstatus = pdesc->ops.get();
        err = CY_EOK;
    } while(0);

    return err;
}

static int32_t __ioctl_set_freq(buzzer_describe_t *pdesc, void *args)
{
    int32_t err = CY_E_WRONG_ARGS;
    uint32_t *freq = (uint32_t *)args;

    if(!args) {
        xlog_tag_error(TAG, "Args format error, can not set buzzer freq\n");
    } else {
        if(pdesc->freq != *freq) {
            pdesc->freq = *freq;
            if(pdesc->ops.deinit) {
                pdesc->ops.deinit();
            }
            if(pdesc->ops.init) {
                pdesc->ops.init();
            }
        }
        err = CY_EOK;
    }

    return err;
}

static int32_t __ioctl_get_freq(buzzer_describe_t *pdesc, void *args)
{
    int32_t err = CY_E_WRONG_ARGS;
    uint32_t *freq = (uint32_t *)args;

    if(!args) {
        xlog_tag_error(TAG, "Args format error, can not get buzzer freq\n");
    } else {
        *freq = pdesc->freq;
        err = CY_EOK;
    }

    return err;
}

static int32_t __ioctl_set_duty(buzzer_describe_t *pdesc, void *args)
{
    int32_t err = CY_EOK;
    float *duty = (float *)args;

    if(!args) {
        err = CY_E_WRONG_ARGS;
        xlog_tag_error(TAG, "Args format error, can not set buzzer duty\n");
    } else if(pdesc->ops.set_duty) {
        if(!pdesc->ops.set_duty(*duty)) {
            xlog_tag_error(TAG, "Set buzzer duty failure\n");
            err = CY_ERROR;
        }
    }

    return err;
}

static int32_t __ioctl_get_duty(buzzer_describe_t *pdesc, void *args)
{
    int32_t err = CY_EOK;
    float *duty = (float *)args;

    if(!args) {
        err = CY_E_WRONG_ARGS;
        xlog_tag_error(TAG, "Args format error, can not get buzzer duty\n");
    } else {
        *duty = pdesc->duty;
    }

    return err;
}

static int32_t buzzer_ioctl(driver_t **pdrv, uint32_t cmd, void *args)
{
    buzzer_describe_t *pdesc = NULL;
    int32_t retval = CY_E_WRONG_ARGS;
    int32_t (*cb)(buzzer_describe_t *, void *) = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(!pdesc) {
            xlog_tag_error(TAG, "driver has not bind describe field\n");
            break;
        }
        if(NULL == (cb = protocol_callback_find(cmd, ioctl_cb_array, ARRAY_SIZE(ioctl_cb_array)))) {
            xlog_tag_error(TAG, "driver not support command(%08X)\n", cmd);
            break;
        }
        retval = cb(pdesc, args);
    } while(0);

    return retval;
}
