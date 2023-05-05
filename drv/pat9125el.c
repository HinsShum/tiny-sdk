/**
 * @file drv\pat9125el.c
 *
 * Copyright (C) 2023
 *
 * pat9125el.c is free software: you can redistribute it and/or modify
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
#include "pat9125el.h"
#include "driver.h"
#include "options.h"

/*---------- macro ----------*/
#define TAG                                         "PAT9125EL"
#define PRODUCT_ID                                  (0x9131)

/*---------- type define ----------*/
typedef enum {
    REG_PRODUCT_ID1 = 0x00,
    REG_PRODUCT_ID2 = 0x01,
    REG_MOTION_STATUS = 0x02,
    REG_DELTA_X_LO = 0x03,
    REG_DELTA_Y_LO = 0x04,
    REG_OPERATION_MODE = 0x05,
    REG_CONFIGURATION = 0x06,
    REG_WRITE_PROTECT = 0x09,
    REG_SLEEP1 = 0x0A,
    REG_SLEEP2 = 0x0B,
    REG_RES_X = 0x0D,
    REG_RES_Y = 0x0E,
    REG_DELTA_XY_HI = 0x12,
    REG_SHUTTER = 0x14,
    REG_FRAME_AVG = 0x17,
    REG_ORIENTATION = 0x19,
    REG_BANK_SELECTION = 0x7F
} reg_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
static int32_t pat9125el_open(driver_t **pdrv);
static void pat9125el_close(driver_t **pdrv);
static int32_t pat9125el_irq_handler(driver_t **pdrv, uint32_t irq_handler, void *args, uint32_t length);
static int32_t pat9125_ioctl(driver_t **pdrv, uint32_t cmd, void *args);
static int32_t _ioctl_set_irq_handler(pat9125el_describe_t *pdesc, void *args);
static int32_t _ioctl_get_data(pat9125el_describe_t *pdesc, void *args);

/*---------- variable ----------*/
DRIVER_DEFINED(pat9125el, pat9125el_open, pat9125el_close, NULL, NULL, pat9125_ioctl, pat9125el_irq_handler);
static struct protocol_callback ioctl_cbs[] = {
    {IOCTL_PAT9125EL_GET_DATA, _ioctl_get_data},
    {IOCTL_PAT9125EL_SET_IRQ_HANDLER, _ioctl_set_irq_handler}
};

/*---------- function ----------*/
static inline void _reg_write(pat9125el_describe_t *pdesc, uint8_t reg, uint8_t data)
{
    i2c_bus_msg_t msg = {
        .type = I2C_BUS_TYPE_WRITE,
        .dev_addr = pdesc->address,
        .mem_addr = &reg,
        .mem_addr_counts = sizeof(reg),
        .buf = &data,
        .len = sizeof(data)
    };

    device_ioctl(pdesc->bus, IOCTL_I2C_BUS_LOCK, NULL);
    device_write(pdesc->bus, &msg, 0, sizeof(msg));
    device_ioctl(pdesc->bus, IOCTL_I2C_BUS_UNLOCK, NULL);
}

static inline uint8_t _reg_read(pat9125el_describe_t *pdesc, uint8_t reg)
{
    uint8_t data = 0;
    i2c_bus_msg_t msg = {
        .type = I2C_BUS_TYPE_RANDOM_READ,
        .dev_addr = pdesc->address,
        .mem_addr = &reg,
        .mem_addr_counts = sizeof(reg),
        .buf = &data,
        .len = sizeof(data)
    };

    device_ioctl(pdesc->bus, IOCTL_I2C_BUS_LOCK, NULL);
    device_read(pdesc->bus, &msg, 0, sizeof(msg));
    device_ioctl(pdesc->bus, IOCTL_I2C_BUS_UNLOCK, NULL);

    return data;
}

static inline bool _initialize(pat9125el_describe_t *pdesc)
{
    uint16_t product_id = 0;
    bool err = false;

    /* get product id */
    product_id = _reg_read(pdesc, REG_PRODUCT_ID1);
    product_id |= ((uint16_t)_reg_read(pdesc, REG_PRODUCT_ID2) << 8);
    xlog_tag_info(TAG, "Product ID: %04X\n", product_id);
    if(product_id == PRODUCT_ID) {
        err = true;
        _reg_write(pdesc, REG_BANK_SELECTION, 0x00);   /*<< switch to bank0 */
        _reg_write(pdesc, REG_CONFIGURATION, 0x97);    /*<< software reset (i.e. set bit7 to 1), then it will reset to 0 automatically */
        __delay_ms(1);
        _reg_write(pdesc, REG_CONFIGURATION, 0x17);    /*<< ensure the sensor has left the reset state */
        _reg_write(pdesc, REG_WRITE_PROTECT, 0x5A);    /*<< disable write protect */
        _reg_write(pdesc, REG_RES_X, 0xFF);            /*<< set X-axis resolution */
        _reg_write(pdesc, REG_RES_Y, 0xFF);            /*<< set Y-axis resolution */
        _reg_write(pdesc, REG_ORIENTATION, 0x04);      /*<< set 12-bit X/Y data format */
        _reg_write(pdesc, REG_WRITE_PROTECT, 0x00);    /*<< enable write protect */
    }

    return err;
}

static int32_t pat9125el_open(driver_t **pdrv)
{
    pat9125el_describe_t *pdesc = NULL;
    int32_t err = CY_E_WRONG_ARGS;
    void *bus = NULL;

    do {
        assert(pdrv);
        pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
        if(!pdesc) {
            xlog_tag_error(TAG, "No describe field\n");
            break;
        }
        err = CY_EOK;
        if(pdesc->ops.init) {
            if(!pdesc->ops.init()) {
                xlog_tag_error(TAG, "Initialize failure\n");
                err = CY_ERROR;
                break;
            }
        }
        /* bind i2c bus */
        if(NULL == (bus = device_open(pdesc->bus_name))) {
            xlog_tag_error(TAG, "Bind i2c bus failure\n");
            if(pdesc->ops.deinit) {
                pdesc->ops.deinit();
            }
            err = CY_ERROR;
            break;
        }
        pdesc->bus = bus;
        if(!_initialize(pdesc)) {
            xlog_tag_error(TAG, "Config reg failure\n");
            err = CY_ERROR;
            device_close(pdesc->bus);
            pdesc->bus = NULL;
            if(pdesc->ops.deinit) {
                pdesc->ops.deinit();
            }
        }
    } while(0);

    return err;
}

static void pat9125el_close(driver_t **pdrv)
{
    pat9125el_describe_t *pdesc = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc) {
        if(pdesc->bus) {
            device_close(pdesc->bus);
            pdesc->bus = NULL;
        }
        if(pdesc->ops.deinit) {
            pdesc->ops.deinit();
        }
    }
}

static int32_t pat9125el_irq_handler(driver_t **pdrv, uint32_t irq_handler, void *args, uint32_t length)
{
    pat9125el_describe_t *pdesc = NULL;
    int32_t err = CY_E_WRONG_ARGS;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc && pdesc->ops.irq_handler) {
        err = pdesc->ops.irq_handler(irq_handler, args, length);
    }

    return err;
}

static int32_t _ioctl_set_irq_handler(pat9125el_describe_t *pdesc, void *args)
{
    int32_t (*irq)(uint32_t, void *, uint32_t) = (int32_t (*)(uint32_t, void *, uint32_t))args;

    pdesc->ops.irq_handler = irq;

    return CY_EOK;
}

static int32_t _ioctl_get_data(pat9125el_describe_t *pdesc, void *args)
{
    int32_t err = CY_E_BUSY;
    int16_t shift = (sizeof(int16_t) << 3) - 12;
    int16_t dx_l = 0, dy_l = 0, dxy_h = 0, dx_h = 0, dy_h = 0;
    union pat9125el_ioctl_param *param = (union pat9125el_ioctl_param *)args;

    do {
        if(!args) {
            err = CY_E_WRONG_ARGS;
            break;
        }
        if(!pdesc->ops.data_valid()) {
            break;
        }
        if(!(_reg_read(pdesc, REG_MOTION_STATUS) & 0x80)) {
            break;
        }
        dx_l = _reg_read(pdesc, REG_DELTA_X_LO);
        dy_l = _reg_read(pdesc, REG_DELTA_Y_LO);
        dxy_h = _reg_read(pdesc, REG_DELTA_XY_HI);
        dx_h = (dxy_h << 4) & 0xF00;
        dx_h = ((int16_t)(dx_h << shift)) >> shift;        /*<< get the correct negative value */
        dy_h = (dxy_h << 8) & 0xF00;
        dy_h = ((int16_t)(dy_h << shift)) >> shift;        /*<< get the correct negative value */
        param->data.x = dx_h | dx_l;
        param->data.y = dy_h | dy_l;
        err = CY_EOK;
    } while(0);

    return err;
}

static int32_t pat9125_ioctl(driver_t **pdrv, uint32_t cmd, void *args)
{
    pat9125el_describe_t *pdesc = NULL;
    int32_t err = CY_E_WRONG_ARGS;
    int32_t (*cb)(pat9125el_describe_t *, void *) = NULL;

    do {
        assert(pdrv);
        pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
        if(!pdesc) {
            xlog_tag_error(TAG, "No describe field\n");
            break;
        }
        cb = protocol_callback_find(cmd, ioctl_cbs, ARRAY_SIZE(ioctl_cbs));
        if(!cb) {
            xlog_tag_error(TAG, "Not support this command(%08X)\n", cmd);
            break;
        }
        err = cb(pdesc, args);
    } while(0);

    return err;
}
