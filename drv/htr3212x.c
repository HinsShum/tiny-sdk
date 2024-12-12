/**
 * @file drv\htr3212x.c
 *
 * Copyright (C) 2024
 *
 * htr3212x.c is free software: you can redistribute it and/or modify
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
#include "htr3212x.h"
#include "i2c_bus.h"
#include "driver.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
#define TAG                                         "HTR3212x"

/*---------- type define ----------*/
typedef enum {
    HTR3212X_REG_SHUTDOWN = 0x00,
    HTR3212X_REG_PWM_CHANNEL1 = 0x0D,
    HTR3212X_REG_PWM_CHANNEL2 = 0x0E,
    HTR3212X_REG_PWM_CHANNEL3 = 0x0F,
    HTR3212X_REG_PWM_CHANNEL4 = 0x10,
    HTR3212X_REG_PWM_CHANNEL5 = 0x11,
    HTR3212X_REG_PWM_CHANNEL6 = 0x12,
    HTR3212X_REG_PWM_CHANNEL7 = 0x13,
    HTR3212X_REG_PWM_CHANNEL8 = 0x14,
    HTR3212X_REG_PWM_CHANNEL9 = 0x15,
    HTR3212X_REG_PWM_CHANNEL10 = 0x16,
    HTR3212X_REG_PWM_CHANNEL11 = 0x17,
    HTR3212X_REG_PWM_CHANNEL12 = 0x18,
    HTR3212X_REG_PWM_UPDATE = 0x25,
    HTR3212X_REG_LED_CONTROL_CHANNEL1 = 0x32,
    HTR3212X_REG_LED_CONTROL_CHANNEL2 = 0x33,
    HTR3212X_REG_LED_CONTROL_CHANNEL3 = 0x34,
    HTR3212X_REG_LED_CONTROL_CHANNEL4 = 0x35,
    HTR3212X_REG_LED_CONTROL_CHANNEL5 = 0x36,
    HTR3212X_REG_LED_CONTROL_CHANNEL6 = 0x37,
    HTR3212X_REG_LED_CONTROL_CHANNEL7 = 0x38,
    HTR3212X_REG_LED_CONTROL_CHANNEL8 = 0x39,
    HTR3212X_REG_LED_CONTROL_CHANNEL9 = 0x3A,
    HTR3212X_REG_LED_CONTROL_CHANNEL10 = 0x3B,
    HTR3212X_REG_LED_CONTROL_CHANNEL11 = 0x3C,
    HTR3212X_REG_LED_CONTROL_CHANNEL12 = 0x3D,
    HTR3212X_REG_GLOBAL_CONTROL = 0x4A,
    HTR3212X_REG_OUTPUT_FREQUENCY = 0x4B,
    HTR3212X_REG_RESET = 0x4F,
} htr3212x_reg_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
static int32_t _open(driver_t **pdrv);
static void _close(driver_t **pdrv);
static int32_t _write(driver_t **pdrv, void *pdata, uint32_t offset, uint32_t length);
static int32_t __ioctl_power_on(htr3212x_describe_t *pdesc, void *args);
static int32_t __ioctl_power_off(htr3212x_describe_t *pdesc, void *args);
static int32_t __ioctl_global_on(htr3212x_describe_t *pdesc, void *args);
static int32_t __ioctl_global_off(htr3212x_describe_t *pdesc, void *args);
static int32_t __ioctl_channel_on(htr3212x_describe_t *pdesc, void *args);
static int32_t __ioctl_channel_off(htr3212x_describe_t *pdesc, void *args);
static int32_t _ioctl(driver_t **pdrv, uint32_t cmd, void *args);

/*---------- variable ----------*/
DRIVER_DEFINED(htr3212x, _open, _close, _write, NULL, _ioctl, NULL);
static const struct protocol_callback ioctl_cbs[] = {
    {IOCTL_DEVICE_POWER_ON, __ioctl_power_on},
    {IOCTL_DEVICE_POWER_OFF, __ioctl_power_off},
    {IOCTL_HTR3212X_GLOBAL_ON, __ioctl_global_on},
    {IOCTL_HTR3212X_GLOBAL_OFF, __ioctl_global_off},
    {IOCTL_HTR3212X_CHANNEL_ON, __ioctl_channel_on},
    {IOCTL_HTR3212X_CHANNEL_OFF, __ioctl_channel_off},
};

/*---------- function ----------*/
static inline bool __write_bytes(htr3212x_describe_t *pdesc, uint8_t addr, uint8_t *pdata, uint32_t length)
{
    i2c_bus_msg_t cont = {
        .type = I2C_BUS_TYPE_WRITE,
        .dev_addr = pdesc->address,
        .mem_addr = &addr,
        .mem_addr_counts = sizeof(addr),
        .buf = pdata,
        .len = length,
    };

    return (CY_EOK == device_write(pdesc->bus, &cont, 0, sizeof(cont)));
}

static bool _initialize_regs(htr3212x_describe_t *pdesc)
{
    bool retval = false;
    uint8_t data[12] = {0};

    do {
        if(true != __write_bytes(pdesc, HTR3212X_REG_RESET, (uint8_t []){0x00}, 1)) {
            xlog_tag_error(TAG, "Reset failure\n");
            break;
        }
        if(true != __write_bytes(pdesc, HTR3212X_REG_SHUTDOWN, (uint8_t []){0x01}, 1)) {
            xlog_tag_error(TAG, "Enable failure\n");
            break;
        }
        if(true != __write_bytes(pdesc, HTR3212X_REG_PWM_CHANNEL1, data, ARRAY_SIZE(data))) {
            xlog_tag_error(TAG, "Set channel pwm failure\n");
            break;
        }
        memset(data, 0x01, ARRAY_SIZE(data));
        if(true != __write_bytes(pdesc, HTR3212X_REG_LED_CONTROL_CHANNEL1, data, ARRAY_SIZE(data))) {
            xlog_tag_error(TAG, "Enable channel failure\n");
            break;
        }
        if(true != __write_bytes(pdesc, HTR3212X_REG_PWM_UPDATE, (uint8_t []){0x00}, 1)) {
            xlog_tag_error(TAG, "Update failure\n");
            break;
        }
        retval = true;
    } while(0);

    return retval;
}

static int32_t _open(driver_t **pdrv)
{
    int32_t retval = CY_E_WRONG_ARGS;
    htr3212x_describe_t *pdesc = NULL;
    void *bus = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(NULL == pdesc) {
            xlog_tag_error(TAG, "Driver not found any device\n");
            break;
        }
        retval = CY_EOK;
        if(pdesc->ops.init) {
            if(true != pdesc->ops.init()) {
                xlog_tag_error(TAG, "BSP initialize failure\n");
                retval = CY_ERROR;
                break;
            }
        }
        /* bind to i2c bus */
        if(NULL == (bus = device_open(pdesc->bus_name))) {
            xlog_tag_error(TAG, "Bind i2c bus failure\n");
            if(pdesc->ops.deinit) {
                pdesc->ops.deinit();
            }
            retval = CY_ERROR;
            break;
        }
        pdesc->bus = bus;
    } while(0);

    return retval;
}

static void _close(driver_t **pdrv)
{
    htr3212x_describe_t *pdesc = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc && pdesc->ops.deinit) {
        pdesc->ops.deinit();
    }
}

static int32_t _write(driver_t **pdrv, void *pdata, uint32_t offset, uint32_t length)
{
    int32_t retval = CY_E_WRONG_ARGS;
    htr3212x_reg_t reg = HTR3212X_REG_PWM_CHANNEL1;
    uint8_t reg_length = 0;
    htr3212x_describe_t *pdesc = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(NULL == pdata || NULL == pdesc) {
            break;
        }
        if((reg + offset) > HTR3212X_REG_PWM_CHANNEL12) {
            break;
        }
        reg += offset;
        reg_length = 12 - offset;
        reg_length = (reg_length < length) ? reg_length : length;
        if(true != __write_bytes(pdesc, reg, pdata, reg_length)) {
            break;
        }
        if(true != __write_bytes(pdesc, HTR3212X_REG_PWM_UPDATE, (uint8_t []){0x00}, 1)) {
            break;
        }
        retval = CY_EOK;
    } while(0);

    return retval;
}

static int32_t __ioctl_power_on(htr3212x_describe_t *pdesc, void *args)
{
    if(pdesc->ops.power) {
        pdesc->ops.power(true);
    }

    return (_initialize_regs(pdesc) ? CY_EOK : CY_ERROR);
}

static int32_t __ioctl_power_off(htr3212x_describe_t *pdesc, void *args)
{
    if(pdesc->ops.power) {
        pdesc->ops.power(false);
    }

    return CY_EOK;
}

static int32_t __ioctl_global_on(htr3212x_describe_t *pdesc, void *args)
{
    return (__write_bytes(pdesc, HTR3212X_REG_GLOBAL_CONTROL, (uint8_t []){0x00}, 1) ? CY_EOK : CY_ERROR);
}

static int32_t __ioctl_global_off(htr3212x_describe_t *pdesc, void *args)
{
    return (__write_bytes(pdesc, HTR3212X_REG_GLOBAL_CONTROL, (uint8_t []){0x01}, 1) ? CY_EOK : CY_ERROR);
}

static int32_t __ioctl_channel_on(htr3212x_describe_t *pdesc, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    union htr3212x_ioctl_param *param = (union htr3212x_ioctl_param *)args;
    htr3212x_reg_t reg = HTR3212X_REG_LED_CONTROL_CHANNEL1;

    do {
        if(NULL == args) {
            break;
        }
        reg += param->channel;
        if(reg > HTR3212X_REG_LED_CONTROL_CHANNEL12) {
            break;
        }
        if(true != __write_bytes(pdesc, reg, (uint8_t []){0x01}, 1)) {
            break;
        }
        if(true != __write_bytes(pdesc, HTR3212X_REG_PWM_UPDATE, (uint8_t []){0x00}, 1)) {
            break;
        }
        retval = CY_EOK;
    } while(0);

    return retval;
}

static int32_t __ioctl_channel_off(htr3212x_describe_t *pdesc, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    union htr3212x_ioctl_param *param = (union htr3212x_ioctl_param *)args;
    htr3212x_reg_t reg = HTR3212X_REG_LED_CONTROL_CHANNEL1;

    do {
        if(NULL == args) {
            break;
        }
        reg += param->channel;
        if(reg > HTR3212X_REG_LED_CONTROL_CHANNEL12) {
            break;
        }
        if(true != __write_bytes(pdesc, reg, (uint8_t []){0x00}, 1)) {
            break;
        }
        if(true != __write_bytes(pdesc, HTR3212X_REG_PWM_UPDATE, (uint8_t []){0x00}, 1)) {
            break;
        }
        retval = CY_EOK;
    } while(0);

    return retval;
}

static int32_t _ioctl(driver_t **pdrv, uint32_t cmd, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    htr3212x_describe_t *pdesc = NULL;
    int32_t (*cb)(htr3212x_describe_t *, void *) = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(NULL == pdesc) {
            xlog_tag_error(TAG, "Driver not found any describe field\n");
            break;
        }
        cb = protocol_callback_find(cmd, (void *)ioctl_cbs, ARRAY_SIZE(ioctl_cbs));
        if(NULL == cb) {
            xlog_tag_warn(TAG, "Driver not support thsi ioctl command(%08X)\n", cmd);
            break;
        }
        retval = cb(pdesc, args);
    } while(0);

    return retval;
}
