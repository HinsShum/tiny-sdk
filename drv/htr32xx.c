/**
 * @file drv\htr32xx.c
 *
 * Copyright (C) 2021
 *
 * htr32xx.c is free software: you can redistribute it and/or modify
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
#include "htr32xx.h"
#include "driver.h"
#include "errorno.h"
#include "i2c_bus.h"
#include "options.h"
#include "soft_timer.h"
#include <string.h>

/*---------- macro ----------*/
#define TAG              "HTR32xx"
#define HTR32xx_LEDSTART 0xFF00
/*---------- variable prototype ----------*/
typedef enum
{
    htr32xx_SHUTDOWN = 0X00,
    HTR3236_LEDPWM1 = 0X01,
    htr32xx_LEDPWM2,
    htr32xx_LEDPWM3,
    htr32xx_LEDPWM4,
    htr32xx_LEDPWM5,
    htr32xx_LEDPWM6,
    htr32xx_LEDPWM7,
    htr32xx_LEDPWM8,
    htr32xx_LEDPWM9,
    htr3218_LEDPWM10 = 0X0A,
    htr32xx_LEDPWM11,
    htr32xx_LEDPWM12,
    htr32x2_LEDPWM13 = 0X0D,
    htr32xx_LEDPWM14,
    htr32xx_LEDPWM15,
    htr32xx_LEDPWM16,
    htr32xx_LEDPWM17,
    htr32xx_LEDPWM18,
    htr32xx_LEDPWM19,
    htr32xx_LEDPWM20,
    htr32xx_LEDPWM21,
    htr32xx_LEDPWM22,
    htr32xx_LEDPWM23,
    htr32xx_LEDPWM24,
    htr32xx_LEDPWM25,
    htr32xx_LEDPWM26,
    htr32xx_LEDPWM27,
    htr32xx_LEDPWM28,
    htr32xx_LEDPWM29,
    htr32xx_LEDPWM30,
    htr32xx_LEDPWM31,
    htr32xx_LEDPWM32,
    htr32xx_LEDPWM33,
    htr32xx_LEDPWM34,
    htr32xx_LEDPWM35,
    htr32xx_LEDPWM36,
    htr32xx_PWM_UPDATA = 0X25,
    HTR3236_LED1_SWITCH = 0X26,
    htr32xx_LED2_SWITCH,
    htr32xx_LED3_SWITCH,
    htr32xx_LED4_SWITCH,
    htr32xx_LED5_SWITCH,
    htr32xx_LED6_SWITCH,
    htr32xx_LED7_SWITCH,
    htr32xx_LED8_SWITCH,
    htr32xx_LED9_SWITCH,
    htr3218_LED10_SWITCH = 0x2F,
    htr32xx_LED11_SWITCH,
    htr32xx_LED12_SWITCH,
    htr3212_LED13_SWITCH = 0x32,
    htr32xx_LED14_SWITCH,
    htr32xx_LED15_SWITCH,
    htr32xx_LED16_SWITCH,
    htr32xx_LED17_SWITCH,
    htr32xx_LED18_SWITCH,
    htr32xx_LED19_SWITCH,
    htr32xx_LED20_SWITCH,
    htr32xx_LED21_SWITCH,
    htr32xx_LED22_SWITCH,
    htr32xx_LED23_SWITCH,
    htr32xx_LED24_SWITCH,
    htr32xx_LED25_SWITCH,
    htr32xx_LED26_SWITCH,
    htr32xx_LED27_SWITCH,
    htr32xx_LED28_SWITCH,
    htr32xx_LED29_SWITCH,
    htr32xx_LED30_SWITCH,
    htr32xx_LED31_SWITCH,
    htr32xx_LED32_SWITCH,
    htr32xx_LED33_SWITCH,
    htr32xx_LED34_SWITCH,
    htr32xx_LED35_SWITCH,
    htr32xx_LED36_SWITCH,
    htr32xx_ALL_SWITCH = 0x4A,
    htr32xx_LED_FREQUENCY = 0X4B,
    htr32xx_RESET_REG = 0X4F,
} htr32xx_register_adr;

volatile uint8_t  ledx_max = 36;
volatile uint8_t  temp_buf[36] = {0};
htr32xx_register_adr ledx_pwm;
htr32xx_register_adr ledx_switch;
/*---------- function prototype ----------*/
static void    htr32xx_config_for_myself(driver_t** pdrv, void* buf, uint32_t offset, uint32_t length);
static int32_t htr32xx_open(driver_t** pdrv);
static void    htr32xx_close(driver_t** pdrv);
static int32_t htr32xx_write(driver_t** pdrv, void* buf, uint32_t offset, uint32_t length);
static int32_t htr32xx_ioctl(driver_t** pdrv, uint32_t cmd, void* args);

static int32_t _ioctl_htr32xx_update(driver_t** pdrv, htr32xx_describe* pdesc, void* args);

DRIVER_DEFINED(htr32xx, htr32xx_open, htr32xx_close, htr32xx_write, NULL, htr32xx_ioctl, NULL);
/*---------- type define ----------*/
typedef int32_t(*htr32xx_cb_func_t)(driver_t** pdrv, htr32xx_describe* pdesc, void* args);
typedef struct
{
    uint32_t htr32xx_cb_cmd;
    htr32xx_cb_func_t cb;
} htr32xx_cb;
static htr32xx_cb ioctl_array[] =
{
    {IOCTL_HTR32xx_UPDATA, _ioctl_htr32xx_update},
};
/*---------- function  local driver  ----------*/
static htr32xx_register_adr outward_transform_special(uint32_t num)
{
    htr32xx_register_adr Tnum;

    if (num < HTR32xx_LEDMAX)
        Tnum = (num + HTR32xx_LEDSTART + ledx_pwm) & 0x00FF;
    else
        Tnum = num;
    return Tnum;
}

static void htr32xx_init(driver_t** pdrv)
{
    htr32xx_config_for_myself(pdrv, (uint8_t*)temp_buf, htr32xx_RESET_REG, 1);
    temp_buf[0] = 1;
    htr32xx_config_for_myself(pdrv, (uint8_t*)temp_buf, htr32xx_SHUTDOWN, 1);
    memset((uint8_t*)temp_buf, 1, ledx_max);
    htr32xx_config_for_myself(pdrv, (uint8_t*)temp_buf, ledx_switch, ledx_max);
    temp_buf[0] = 1;
    htr32xx_config_for_myself(pdrv, (uint8_t*)temp_buf, htr32xx_LED_FREQUENCY, 1);
    temp_buf[0] = 0;
    htr32xx_config_for_myself(pdrv, (uint8_t*)temp_buf, htr32xx_ALL_SWITCH, 1);

    memset((uint8_t*)temp_buf, 0, ledx_max);
    htr32xx_config_for_myself(pdrv, (uint8_t*)temp_buf, ledx_pwm, ledx_max);
    htr32xx_config_for_myself(pdrv, (uint8_t*)temp_buf, htr32xx_PWM_UPDATA, 1);
}

/*---------- function  driver  for  app ----------*/

static int32_t htr32xx_open(driver_t** pdrv)
{
    htr32xx_describe* pdesc = NULL;
    int32_t retval = CY_E_WRONG_ARGS;
    void* temp_bus = NULL;
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    assert(pdesc);
    ledx_max = pdesc->htr32xx_ledx_max;
    if(strcmp(pdesc->htr32xx_dev_name,HTR3236_NAME) == 0)
    {
        ledx_pwm = HTR3236_LEDPWM1;
        ledx_switch = HTR3236_LED1_SWITCH;
    }
    else if(strcmp(pdesc->htr32xx_dev_name,HTR3218_NAME) == 0)
    {
        ledx_pwm = htr3218_LEDPWM10;
        ledx_switch = htr3218_LED10_SWITCH;
    }  
    else if(strcmp(pdesc->htr32xx_dev_name,HTR3212_NAME) == 0)
    {
        ledx_pwm = htr32x2_LEDPWM13;
        ledx_switch = htr3212_LED13_SWITCH;
    }
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
        temp_bus = device_open(pdesc->htr32xx_iic_name);
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
            htr32xx_init(pdrv);
        }

    } while (0);
    return retval;
}
static void htr32xx_close(driver_t** pdrv)
{
    htr32xx_describe* pdesc = NULL;
    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if (pdesc)
    {
        if (pdesc->htr32xx_iic_name)
        {
            device_close(pdesc->bus);
            pdesc->htr32xx_iic_name = NULL;
        }
        pdesc->deinit();
    }
}
// buf
static int32_t htr32xx_write(driver_t** pdrv, void* buf, uint32_t offset, uint32_t length)
{
    i2c_bus_msg_t     msg = { 0 };
    htr32xx_describe* pdesc = NULL;
    uint32_t          actual_len = 0;
    int32_t           result = CY_EOK;
    uint32_t          address = 0;
    uint8_t           temp_memory_addr[2] = { 0 };

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
static void htr32xx_config_for_myself(driver_t** pdrv, void* buf, uint32_t offset, uint32_t length)
{
    i2c_bus_msg_t     msg = { 0 };
    uint8_t           temp_memory_addr[2] = { 0 };
    int32_t           result = CY_EOK;
    htr32xx_describe* pdesc = NULL;
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
static int32_t _ioctl_htr32xx_update(driver_t** pdrv, htr32xx_describe* pdesc, void* args)
{
    int32_t          retval = CY_E_WRONG_ARGS;
    volatile uint8_t temp_buff[1] = { 0 };
    int32_t          actual_len = 0;
    retval = CY_EOK;
    htr32xx_write(pdrv, (uint8_t*)temp_buff, htr32xx_PWM_UPDATA, 1);
    return retval;
}

static htr32xx_cb_func_t _ioctl_cb_func_find(uint32_t ioctl_cmd)
{
    htr32xx_cb_func_t cb = NULL;

    for (uint32_t i = 0; i < ARRAY_SIZE(ioctl_array); ++i)
    {
        if (ioctl_array[i].htr32xx_cb_cmd == ioctl_cmd)
        {
            cb = ioctl_array[i].cb;
            break;
        }
    }
    return cb;
}
/*************************** cb_func_t end *************************/
static int32_t htr32xx_ioctl(driver_t** pdrv, uint32_t cmd, void* args)
{
    htr32xx_describe* pdesc = NULL;
    int32_t           retval = CY_E_WRONG_ARGS;
    htr32xx_cb_func_t cb = NULL;
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
