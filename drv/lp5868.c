/**
 * @file drv\lp5868.c
 *
 * Copyright (C) 2024
 *
 * lp5868.c is free software: you can redistribute it and/or modify
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
#include "lp5868.h"
#include "i2c_bus.h"
#include "driver.h"
#include "options.h"

/*---------- macro ----------*/
#define TAG                                         "LP5868"
#define REG_DC(off)                                 ({assert((off) <= (REG_DC143 -REG_DC0)); (lp5868_reg_t)((uint16_t)REG_DC0 + off);})
#define REG_PWM_BRI(off)                            ({assert((off) <= (REG_PWM_BRI287 - REG_PWM_BRI0)); (lp5868_reg_t)((uint16_t)REG_PWM_BRI0 + off);})

/*---------- type define ----------*/
typedef enum {
    REG_CHIP_EN = 0x00,         /*<< R/W, default: 0x00 */
    REG_DEV_INITIAL = 0x01,     /*<< R/W, default: 0x5E */
    REG_DEV_CONFIG1 = 0x02,     /*<< R/W, default: 0x00 */
    REG_DEV_CONFIG2 = 0x03,     /*<< R/W, default: 0x00 */
    REG_DEV_CONFIG3 = 0x04,     /*<< R/W, default: 0x47 */
    REG_GLOBAL_BRI = 0x05,      /*<< R/W, default: 0xFF */
    REG_GROUP0_BRI = 0x06,      /*<< R/W, default: 0xFF */
    REG_GROUP1_BRI = 0x07,      /*<< R/W, default: 0xFF */
    REG_GROUP2_BRI = 0x08,      /*<< R/W, default: 0xFF */
    REG_R_CURRENT_SET = 0x09,   /*<< R/W, default: 0x40 */
    REG_G_CURRENT_SET = 0x0A,   /*<< R/W, default: 0x40 */
    REG_B_CURRENT_SET = 0x0B,   /*<< R/W, default: 0x40 */
    REG_DOT_GRP_SEL0 = 0x0C,    /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL1 = 0x0D,    /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL2 = 0x0E,    /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL3 = 0x0F,    /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL4 = 0x10,    /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL5 = 0x11,    /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL6 = 0x12,    /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL7 = 0x13,    /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL8 = 0x14,    /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL9 = 0x15,    /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL10 = 0x16,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL11 = 0x17,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL12 = 0x18,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL13 = 0x19,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL14 = 0x1A,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL15 = 0x1B,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL16 = 0x1C,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL17 = 0x1D,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL18 = 0x1E,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL19 = 0x1F,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL20 = 0x20,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL21 = 0x21,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL22 = 0x22,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL23 = 0x23,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL24 = 0x24,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL25 = 0x25,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL26 = 0x26,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL27 = 0x27,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL28 = 0x28,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL29 = 0x29,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL30 = 0x2A,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL31 = 0x2B,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL32 = 0x2C,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL33 = 0x2D,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL34 = 0x2E,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL35 = 0x2F,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL36 = 0x30,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL37 = 0x31,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL38 = 0x32,   /*<< R/W, default: 0x00 */
    REG_DOT_GRP_SEL39 = 0x33,   /*<< R/W, default: 0x00 */
    REG_DOT_ONOFF0 = 0x43,      /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF1 = 0x44,      /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF2 = 0x45,      /*<< R/W, default: 0x03 */
    REG_DOT_ONOFF3 = 0x46,      /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF4 = 0x47,      /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF5 = 0x48,      /*<< R/W, default: 0x03 */
    REG_DOT_ONOFF6 = 0x49,      /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF7 = 0x4A,      /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF8 = 0x4B,      /*<< R/W, default: 0x03 */
    REG_DOT_ONOFF9 = 0x4C,      /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF10 = 0x4D,     /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF11 = 0x4E,     /*<< R/W, default: 0x03 */
    REG_DOT_ONOFF12 = 0x4F,     /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF13 = 0x50,     /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF14 = 0x51,     /*<< R/W, default: 0x03 */
    REG_DOT_ONOFF15 = 0x52,     /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF16 = 0x53,     /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF17 = 0x54,     /*<< R/W, default: 0x03 */
    REG_DOT_ONOFF18 = 0x55,     /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF19 = 0x56,     /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF20 = 0x57,     /*<< R/W, default: 0x03 */
    REG_DOT_ONOFF21 = 0x58,     /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF22 = 0x59,     /*<< R/W, default: 0xFF */
    REG_DOT_ONOFF23 = 0x5A,     /*<< R/W, default: 0x03 */
    REG_FAULT_STATE = 0x64,     /*<< R, default: 0x00 */
    REG_DOT_LOD0 = 0x65,        /*<< R, default: 0x00 */
    REG_DOT_LOD1 = 0x66,        /*<< R, default: 0x00 */
    REG_DOT_LOD2 = 0x67,        /*<< R, default: 0x00 */
    REG_DOT_LOD3 = 0x68,        /*<< R, default: 0x00 */
    REG_DOT_LOD4 = 0x69,        /*<< R, default: 0x00 */
    REG_DOT_LOD5 = 0x6A,        /*<< R, default: 0x00 */
    REG_DOT_LOD6 = 0x6B,        /*<< R, default: 0x00 */
    REG_DOT_LOD7 = 0x6C,        /*<< R, default: 0x00 */
    REG_DOT_LOD8 = 0x6D,        /*<< R, default: 0x00 */
    REG_DOT_LOD9 = 0x6E,        /*<< R, default: 0x00 */
    REG_DOT_LOD10 = 0x6F,       /*<< R, default: 0x00 */
    REG_DOT_LOD11 = 0x70,       /*<< R, default: 0x00 */
    REG_DOT_LOD12 = 0x71,       /*<< R, default: 0x00 */
    REG_DOT_LOD13 = 0x72,       /*<< R, default: 0x00 */
    REG_DOT_LOD14 = 0x73,       /*<< R, default: 0x00 */
    REG_DOT_LOD15 = 0x74,       /*<< R, default: 0x00 */
    REG_DOT_LOD16 = 0x75,       /*<< R, default: 0x00 */
    REG_DOT_LOD17 = 0x76,       /*<< R, default: 0x00 */
    REG_DOT_LOD18 = 0x77,       /*<< R, default: 0x00 */
    REG_DOT_LOD19 = 0x78,       /*<< R, default: 0x00 */
    REG_DOT_LOD20 = 0x79,       /*<< R, default: 0x00 */
    REG_DOT_LOD21 = 0x7A,       /*<< R, default: 0x00 */
    REG_DOT_LOD22 = 0x7B,       /*<< R, default: 0x00 */
    REG_DOT_LOD23 = 0x7C,       /*<< R, default: 0x00 */
    REG_DOT_LSD0 = 0x86,        /*<< R, default: 0x00 */
    REG_DOT_LSD1 = 0x87,        /*<< R, default: 0x00 */
    REG_DOT_LSD2 = 0x88,        /*<< R, default: 0x00 */
    REG_DOT_LSD3 = 0x89,        /*<< R, default: 0x00 */
    REG_DOT_LSD4 = 0x8A,        /*<< R, default: 0x00 */
    REG_DOT_LSD5 = 0x8B,        /*<< R, default: 0x00 */
    REG_DOT_LSD6 = 0x8C,        /*<< R, default: 0x00 */
    REG_DOT_LSD7 = 0x8D,        /*<< R, default: 0x00 */
    REG_DOT_LSD8 = 0x8E,        /*<< R, default: 0x00 */
    REG_DOT_LSD9 = 0x8F,        /*<< R, default: 0x00 */
    REG_DOT_LSD10 = 0x90,       /*<< R, default: 0x00 */
    REG_DOT_LSD11 = 0x91,       /*<< R, default: 0x00 */
    REG_DOT_LSD12 = 0x92,       /*<< R, default: 0x00 */
    REG_DOT_LSD13 = 0x93,       /*<< R, default: 0x00 */
    REG_DOT_LSD14 = 0x94,       /*<< R, default: 0x00 */
    REG_DOT_LSD15 = 0x95,       /*<< R, default: 0x00 */
    REG_DOT_LSD16 = 0x96,       /*<< R, default: 0x00 */
    REG_DOT_LSD17 = 0x97,       /*<< R, default: 0x00 */
    REG_DOT_LSD18 = 0x98,       /*<< R, default: 0x00 */
    REG_DOT_LSD19 = 0x99,       /*<< R, default: 0x00 */
    REG_DOT_LSD20 = 0x9A,       /*<< R, default: 0x00 */
    REG_DOT_LSD21 = 0x9B,       /*<< R, default: 0x00 */
    REG_DOT_LSD22 = 0x9C,       /*<< R, default: 0x00 */
    REG_DOT_LSD23 = 0x9D,       /*<< R, default: 0x00 */
    REG_LOD_CLEAR = 0xA7,       /*<< W, default: 0x00 */
    REG_LSD_CLEAR = 0xA8,       /*<< W, default: 0x00 */
    REG_RESET = 0xA9,           /*<< W, default: 0x00 */
    REG_DC0 = 0x100,            /*<< R/W, default: 0x80*/
    REG_DC143 = 0x18F,          /*<< R/W, default: 0x80*/
    REG_PWM_BRI0 = 0x200,       /*<< R/W, default: 0x00*/
    REG_PWM_BRI287 = 0x31F,     /*<< R/W, default: 0x00*/
} lp5868_reg_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
static int32_t _open(driver_t **pdrv);
static void _close(driver_t **pdrv);
static int32_t _ioctl(driver_t **pdrv, uint32_t cmd, void *args);
static int32_t __ioctl_power_on(device_t *dev, void *args);
static int32_t __ioctl_power_off(device_t *dev, void *args);
static int32_t __ioctl_enable(device_t *dev, void *args);
static int32_t __ioctl_refresh(device_t *dev, void *args);
static int32_t __ioctl_clear(device_t *dev, void *args);

/*---------- variable ----------*/
DRIVER_DEFINED(lp5868, _open, _close, NULL, NULL, _ioctl, NULL);
static const struct protocol_callback _ioctl_cbs[] = {
    {IOCTL_DEVICE_POWER_ON, __ioctl_power_on},
    {IOCTL_DEVICE_POWER_OFF, __ioctl_power_off},
    {IOCTL_LP5868_ENABLE, __ioctl_enable},
    {IOCTL_LP5868_REFRESH, __ioctl_refresh},
    {IOCTL_LP5868_CLEAR, __ioctl_clear},
};

/*---------- function ----------*/
static int32_t _open(driver_t **pdrv)
{
    int32_t retval = CY_E_WRONG_ARGS;
    lp5868_describe_t *pdesc = NULL;
    void *bus = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(pdesc == NULL) {
            xlog_tag_error(TAG, "Driver not found any device\n");
            break;
        }
        retval = CY_ERROR;
        if(pdesc->ops.init) {
            if(pdesc->ops.init() != true) {
                xlog_tag_error(TAG, "BSP initialize failure\n");
                break;
            }
        }
        /* bind to i2c bus */
        if(NULL == (bus = device_open(pdesc->bus_name))) {
            xlog_tag_error(TAG, "Bind i2c bus failure\n");
            if(pdesc->ops.deinit) {
                pdesc->ops.deinit();
            }
            break;
        }
        pdesc->bus = bus;
        retval = CY_EOK;
    } while(0);

    return retval;
}

static void _close(driver_t **pdrv)
{
    lp5868_describe_t *pdesc = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc && pdesc->ops.deinit) {
        pdesc->ops.deinit();
    }
}

static int32_t __ioctl_power_on(device_t *dev, void *args)
{
    int32_t retval = CY_EOK;
    lp5868_describe_t *pdesc = dev->pdesc;

    if(!__device_attrib_ispower(dev->attribute)) {
        if(pdesc->ops.power_ctl == NULL || pdesc->ops.power_ctl(true) != true) {
            retval = CY_ERROR;
        } else {
            __device_attrib_setpower(dev->attribute, DEVICE_ATTRIB_POWER_ON);
            __delay_us(500);
        }
    }

    return retval;
}

static int32_t __ioctl_power_off(device_t *dev, void *args)
{
    int32_t retval = CY_EOK;
    lp5868_describe_t *pdesc = dev->pdesc;

    if(__device_attrib_ispower(dev->attribute)) {
        if(pdesc->ops.power_ctl == NULL || pdesc->ops.power_ctl(false) != true) {
            retval = CY_ERROR;
        } else {
            __device_attrib_setpower(dev->attribute, DEVICE_ATTRIB_POWER_OFF);
        }
    }

    return retval;
}

static int32_t __ioctl_enable(device_t *dev, void *args)
{
    lp5868_describe_t *pdesc = dev->pdesc;
    i2c_bus_msg_t cont = {.type = I2C_BUS_TYPE_WRITE};
    uint16_t dc_nums = REG_DC143 - REG_DC0 + 1;

    /* enable chip */
    cont.dev_addr = pdesc->address;
    cont.mem_addr = (uint8_t []){REG_CHIP_EN};
    cont.mem_addr_counts = 1;
    cont.buf = (uint8_t []){0x01};
    cont.len = 1;
    device_write(pdesc->bus, &cont, 0, sizeof(cont));
    __delay_us(200);
    /* initailize chip */
    cont.mem_addr = (uint8_t []){REG_DEV_INITIAL};
    cont.mem_addr_counts = 1;
    cont.buf = (uint8_t []){((8 << 3) | (0 << 1) | 1)};
    cont.len = 1;
    device_write(pdesc->bus, &cont, 0, sizeof(cont));
    /* set dc point */
    cont.buf = __malloc(dc_nums);
    if(cont.buf) {
        memset(cont.buf, 0xFF, dc_nums);
        cont.len = dc_nums;
        cont.mem_addr = (uint8_t []){(uint8_t)(REG_DC0 & 0xFF)};
        cont.mem_addr_counts = 1;
        cont.dev_addr = pdesc->address | ((((uint16_t)REG_DC0 >> 8) & 0x03) << 1);
        device_write(pdesc->bus, &cont, 0, sizeof(cont));
        __free(cont.buf);
    }

    return CY_EOK;
}

static int32_t __ioctl_refresh(device_t *dev, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    lp5868_describe_t *pdesc = dev->pdesc;
    lp5868_refresh_param_t *param = (lp5868_refresh_param_t *)args;
    i2c_bus_msg_t cont = {.type = I2C_BUS_TYPE_WRITE};
    uint16_t reg = 0;

    do {
        if(args == NULL) {
            break;
        }
        if(param->pdata == NULL) {
            break;
        }
        if(param->x >= pdesc->resolution.column) {
            break;
        }
        if(param->y >= pdesc->resolution.row) {
            break;
        }
        reg = REG_PWM_BRI(param->y * pdesc->resolution.column + param->x);
        if(reg > REG_PWM_BRI287) {
            break;
        }
        cont.dev_addr = pdesc->address | (((reg >> 8) & 0x03) << 1);
        cont.mem_addr = (uint8_t []){(uint8_t)(reg & 0xFF)};
        cont.mem_addr_counts = 1;
        cont.buf = param->pdata;
        cont.len = param->length;
        device_write(pdesc->bus, &cont, 0, sizeof(cont));
        retval = CY_EOK;
    } while(0);

    return retval;
}

static int32_t __ioctl_clear(device_t *dev, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    lp5868_describe_t *pdesc = dev->pdesc;
    lp5868_clear_param_t *param = (lp5868_clear_param_t *)args;
    i2c_bus_msg_t cont = {.type = I2C_BUS_TYPE_WRITE};

    do {
        if(args == NULL) {
            break;
        }
        cont.len = pdesc->resolution.row * pdesc->resolution.column;
        cont.buf = __malloc(cont.len);
        if(cont.buf == NULL) {
            break;
        }
        memset(cont.buf, param->data, cont.len);
        cont.dev_addr = pdesc->address | ((((uint16_t)REG_PWM_BRI0 >> 8) & 0x03) << 1);
        cont.mem_addr = (uint8_t []){(uint8_t)(REG_PWM_BRI0 & 0xFF)};
        cont.mem_addr_counts = 1;
        device_write(pdesc->bus, &cont, 0, sizeof(cont));
        __free(cont.buf);
        retval = CY_EOK;
    } while(0);

    return retval;
}

static int32_t _ioctl(driver_t **pdrv, uint32_t cmd, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    device_t *dev = NULL;
    int32_t (*cb)(device_t *, void *);

    assert(pdrv);
    dev = container_of(pdrv, device_t, pdrv);
    do {
        if(dev->pdesc == NULL) {
            xlog_tag_error(TAG, "Driver not found any describe field\n");
            break;
        }
        cb = protocol_callback_find(cmd, (void *)_ioctl_cbs, ARRAY_SIZE(_ioctl_cbs));
        if(cb == NULL) {
            xlog_tag_warn(TAG, "Driver not support this ioctl cmd(%08X)\n", cmd);
            break;
        }
        retval = cb(dev, args);
    } while(0);

    return retval;
}
