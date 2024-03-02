/**
 * @file drv\htr321x.c
 *
 * Copyright (C) 2021
 *
 * htr321x.c is free software: you can redistribute it and/or modify
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
 * @author zhuodada
 *
 * @encoding utf-8
 */

/*---------- includes ----------*/
#include "htr321x.h"
#include "driver.h"
#include "errorno.h"
#include "i2c_bus.h"
#include "options.h"
#include "soft_timer.h"
#include <string.h>

/*---------- macro ----------*/
#define TAG              "HTR321x"
#define MAX              18
#define HTR321x_LEDSTART 0xFF00
/*---------- variable prototype ----------*/
typedef enum
{
    HTR321x_SHUTDOWN = 0X00,
    HTR321x_LEDPWM0 = 0X0A,
    HTR321x_LEDPWM1,
    HTR321x_LEDPWM2,
    HTR321x_LEDPWM3,
    HTR321x_LEDPWM4,
    HTR321x_LEDPWM5,
    HTR321x_LEDPWM6,
    HTR321x_LEDPWM7,
    HTR321x_LEDPWM8,
    HTR321x_LEDPWM9,
    HTR321x_LEDPWM10,
    HTR321x_LEDPWM11,
    HTR321x_LEDPWM12,
    HTR321x_LEDPWM13,
    HTR321x_LEDPWM14,
    HTR321x_LEDPWM15,
    HTR321x_LEDPWM16,
    HTR321x_LEDPWM17,
    HTR321x_PWM_UPDATA = 0X25,
    HTR321x_LED0_SWITCH = 0x2F,
    HTR321x_LED1_SWITCH,
    HTR321x_LED2_SWITCH,
    HTR321x_LED3_SWITCH,
    HTR321x_LED4_SWITCH,
    HTR321x_LED5_SWITCH,
    HTR321x_LED6_SWITCH,
    HTR321x_LED7_SWITCH,
    HTR321x_LED8_SWITCH,
    HTR321x_LED9_SWITCH,
    HTR321x_LED10_SWITCH,
    HTR321x_LED11_SWITCH,
    HTR321x_LED12_SWITCH,
    HTR321x_LED13_SWITCH,
    HTR321x_LED14_SWITCH,
    HTR321x_LED15_SWITCH,
    HTR321x_LED16_SWITCH,
    HTR321x_LED17_SWITCH,
    HTR321x_ALL_SWITCH = 0x4A,
    HTR321x_LED_FREQUENCY = 0X4B,
    HTR321x_RESET_REG = 0X4F,
} htr321x_register_adr;

volatile uint8_t temp_buf[MAX] = {0};

/*---------- function prototype ----------*/
static void    htr321x_config_for_myself(driver_t **pdrv, void *buf, uint32_t offset, uint32_t length);
static int32_t htr321x_open(driver_t **pdrv);
static void    htr321x_close(driver_t **pdrv);
static int32_t htr321x_write(driver_t **pdrv, void *buf, uint32_t offset, uint32_t length);
static int32_t htr321x_ioctl(driver_t **pdrv, uint32_t cmd, void *args);

static int32_t _ioctl_htr321x_update(driver_t **pdrv, htr321x_describe *pdesc, void *args);

DRIVER_DEFINED(htr321x, htr321x_open, htr321x_close, htr321x_write, NULL, htr321x_ioctl, NULL);
/*---------- type define ----------*/
typedef int32_t (*htr321x_cb_func_t)(driver_t **pdrv, htr321x_describe *pdesc, void *args);
typedef struct
{
    uint32_t          htr321x_cb_cmd;
    htr321x_cb_func_t cb;
} htr321x_cb;
static htr321x_cb ioctl_array[] =
    {
        {IOCTL_HTR321x_UPDATA, _ioctl_htr321x_update},
};
/*---------- function  local driver  ----------*/
static htr321x_register_adr outward_transform_special(uint32_t num)
{
    htr321x_register_adr Tnum;

    if (num < HTR321x_LEDMAX)
        Tnum = (num + HTR321x_LEDSTART + HTR321x_LEDPWM0) & 0x00FF;
    else
        Tnum = num;
    return Tnum;
}

static void htr321x_init(driver_t **pdrv)
{
    htr321x_config_for_myself(pdrv, (uint8_t *)temp_buf, HTR321x_RESET_REG, 1);
    temp_buf[0] = 1;
    htr321x_config_for_myself(pdrv, (uint8_t *)temp_buf, HTR321x_SHUTDOWN, 1);
    memset((uint8_t *)temp_buf, 1, MAX);
    htr321x_config_for_myself(pdrv, (uint8_t *)temp_buf, HTR321x_LED0_SWITCH, MAX);
    temp_buf[0] = 1;
    htr321x_config_for_myself(pdrv, (uint8_t *)temp_buf, HTR321x_LED_FREQUENCY, 1);
    temp_buf[0] = 0;
    htr321x_config_for_myself(pdrv, (uint8_t *)temp_buf, HTR321x_ALL_SWITCH, 1);

    memset((uint8_t *)temp_buf, 0, MAX);
    htr321x_config_for_myself(pdrv, (uint8_t *)temp_buf, HTR321x_LEDPWM0, MAX);
    htr321x_config_for_myself(pdrv, (uint8_t *)temp_buf, HTR321x_PWM_UPDATA, 1);
}

/*---------- function  driver  for  app ----------*/

static int32_t htr321x_open(driver_t **pdrv)
{
    htr321x_describe *pdesc = NULL;
    int32_t           retval = CY_E_WRONG_ARGS;
    void             *temp_bus = NULL;
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    assert(pdesc);
    do
    {
        if (!pdesc)
        {
            xlog_tag_error(TAG, "driver has no describe field\n");
            break;
        }
        retval = CY_EOK;
        if (pdesc->init() != true)
        {
            xlog_tag_error(TAG, "initialize failed\n");
            retval = CY_ERROR;
            break;
        }
        temp_bus = device_open(pdesc->htr321x_iic_name);
        if (temp_bus == NULL)
        {
            xlog_tag_error(TAG, "bind i2c bus failed\n");
            retval = CY_ERROR;
            break;
        }
        else
        {
            pdesc->bus = temp_bus; // bind i2c bus
            pdesc->bsp_chip_enable();
            htr321x_init(pdrv);
        }

    } while (0);
    return retval;
}
static void htr321x_close(driver_t **pdrv)
{
    htr321x_describe *pdesc = NULL;
    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if (pdesc)
    {
        if (pdesc->htr321x_iic_name)
        {
            device_close(pdesc->bus);
            pdesc->htr321x_iic_name = NULL;
        }
        pdesc->deinit();
    }
}
// buf
static int32_t htr321x_write(driver_t **pdrv, void *buf, uint32_t offset, uint32_t length)
{
    i2c_bus_msg_t     msg = {0};
    htr321x_describe *pdesc = NULL;
    uint32_t          actual_len = 0;
    int32_t           result = CY_EOK;
    uint32_t          address = 0;
    uint8_t           temp_memory_addr[2] = {0};

    assert(pdrv);
    assert(buf);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do
    {
        if (!pdesc)
        {
            xlog_tag_error(TAG, "driver has no describe field\n");
            break;
        }
        if (!pdesc->bus)
        {
            xlog_tag_error(TAG, "not bind to i2c bus\n");
            break;
        }
        msg.type = I2C_BUS_TYPE_WRITE;
        msg.dev_addr = pdesc->address;
        msg.buf = buf;
        msg.mem_addr_counts = pdesc->mem_addr_counts;
        offset = outward_transform_special(offset);
        if (pdesc->mem_addr_counts == 1)
        {
            temp_memory_addr[0] = offset & 0xFF;
        }
        else
        {
            temp_memory_addr[0] = (offset >> 8) & 0xFF;
            temp_memory_addr[1] = offset & 0xFF;
        }
        msg.mem_addr = temp_memory_addr;
        msg.len = length;
        device_ioctl(pdesc->bus, IOCTL_I2C_BUS_LOCK, NULL);
        result = device_write(pdesc->bus, &msg, 0, sizeof(msg));
        device_ioctl(pdesc->bus, IOCTL_I2C_BUS_UNLOCK, NULL);

        if (CY_EOK != result)
        {
            xlog_tag_error(TAG, "write failed\n");
            break;
        }
    } while (0);
    return (int32_t)actual_len;
}

/*************************** cb_func_t *************************/
static void htr321x_config_for_myself(driver_t **pdrv, void *buf, uint32_t offset, uint32_t length)
{
    i2c_bus_msg_t     msg = {0};
    uint8_t           temp_memory_addr[2] = {0};
    int32_t           result = CY_EOK;
    htr321x_describe *pdesc = NULL;
    assert(pdrv);

    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    assert(pdesc);
    assert(buf);

    do
    {
        if (!pdesc)
        {
            xlog_tag_error(TAG, "driver has no describe field\n");
            break;
        }
        if (!pdesc->bus)
        {
            xlog_tag_error(TAG, "not bind to i2c bus\n");
            break;
        }
        msg.type = I2C_BUS_TYPE_WRITE;
        msg.dev_addr = pdesc->address;
        msg.buf = buf;
        msg.mem_addr_counts = pdesc->mem_addr_counts;
        if (pdesc->mem_addr_counts == 1)
        {
            temp_memory_addr[0] = offset & 0xFF;
        }
        else
        {
            temp_memory_addr[0] = (offset >> 8) & 0xFF;
            temp_memory_addr[1] = offset & 0xFF;
        }
        msg.mem_addr = temp_memory_addr;
        msg.len = length;
        device_ioctl(pdesc->bus, IOCTL_I2C_BUS_LOCK, NULL);
        result = device_write(pdesc->bus, &msg, 0, sizeof(msg));
        device_ioctl(pdesc->bus, IOCTL_I2C_BUS_UNLOCK, NULL);

        if (CY_EOK != result)
        {
            xlog_tag_error(TAG, "write failed\n");
            break;
        }
    } while (0);
}
static int32_t _ioctl_htr321x_update(driver_t **pdrv, htr321x_describe *pdesc, void *args)
{
    int32_t          retval = CY_E_WRONG_ARGS;
    volatile uint8_t temp_buff[1] = {0};
    int32_t          actual_len = 0;
    retval = CY_EOK;
    htr321x_write(pdrv, (uint8_t *)temp_buff, HTR321x_PWM_UPDATA, 1);
    return retval;
}

static htr321x_cb_func_t _ioctl_cb_func_find(uint32_t ioctl_cmd)
{
    htr321x_cb_func_t cb = NULL;

    for (uint32_t i = 0; i < ARRAY_SIZE(ioctl_array); ++i)
    {
        if (ioctl_array[i].htr321x_cb_cmd == ioctl_cmd)
        {
            cb = ioctl_array[i].cb;
            break;
        }
    }
    return cb;
}
/*************************** cb_func_t end *************************/
static int32_t htr321x_ioctl(driver_t **pdrv, uint32_t cmd, void *args)
{
    htr321x_describe *pdesc = NULL;
    int32_t           retval = CY_E_WRONG_ARGS;
    htr321x_cb_func_t cb = NULL;
    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do
    {
        if (!pdesc)
        {
            xlog_tag_error(TAG, "has no describe field\n");
            break;
        }
        if (NULL == (cb = _ioctl_cb_func_find(cmd)))
        {
            xlog_tag_error(TAG, "not support this command(%08X)\n", cmd);
            break;
        }
        retval = cb(pdrv, pdesc, args);
    } while (0);
    return retval;
}
