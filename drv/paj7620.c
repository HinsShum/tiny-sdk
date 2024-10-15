/**
 * @file drv\paj7620.c
 *
 * Copyright (C) 2024
 *
 * paj7620.c is free software: you can redistribute it and/or modify
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
#include "paj7620.h"
#include "i2c_bus.h"
#include "driver.h"
#include "options.h"
#include "errorno.h"
#include "misc.h"

/*---------- macro ----------*/
#define TAG                                         "PAJ7620"
#define PAJ7620_PARTID                              0x7620
/* bank select
 */
#define REG_BANK_SEL                                0xEF
#define REG_BANK_SEL_DATA_BANK0                     0x00
#define REG_BANK_SEL_DATA_BANK1                     0x01
/* bank0 reg
 */
#define BANK0_REG_PARTID_LOW                        0x00
#define BANK0_REG_PARTID_HIGH                       0x01
#define BANK0_REG_VERSION_ID                        0x02
#define BANK0_REG_INT_FLAG1                         0x43
#define BANK0_REG_INT_FLAG2                         0x44
/* gesture int flag bit
 */
#undef BIT
#define BIT(x)                                      (1<<(x))
#define GES_INTFLAG_BIT_LEFT                        BIT(0)
#define GES_INTFLAG_BIT_RIGHT                       BIT(1)
#define GES_INTFLAG_BIT_DOWM                        BIT(2)
#define GES_INTFLAG_BIT_UP                          BIT(3)
#define GES_INTFLAG_BIT_FORWARD                     BIT(4)
#define GES_INTFLAG_BIT_BACKWARD                    BIT(5)
#define GES_INTFLAG_BIT_CLOCKWISE                   BIT(6)
#define GES_INTFLAG_BIT_COUNT_CLOCKWISE             BIT(7)
#define GES_INTFLAG_BIT_WAVE                        BIT(8)

/*---------- type define ----------*/
/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
static int32_t _open(driver_t **pdrv);
static void _close(driver_t **pdrv);
static int32_t _ioctl(driver_t **pdrv, uint32_t cmd, void *args);
static int32_t __ioctl_set_irq_handler(paj7620_describe_t *pdesc, void *args);
static int32_t __ioctl_set_event_callback(paj7620_describe_t *pdesc, void *args);
static int32_t __ioctl_read_irq_pin(paj7620_describe_t *pdesc, void *args);
static int32_t __ioctl_interrupt_handling(paj7620_describe_t *pdesc, void *args);
static int32_t _irq_handler(driver_t **pdrv, uint32_t irq_handler, void *args, uint32_t length);

/*---------- variable ----------*/
DRIVER_DEFINED(paj7620, _open, _close, NULL, NULL, _ioctl, _irq_handler);
static const struct protocol_callback ioctl_cbs[] = {
    {IOCTL_PAJ7620_SET_IRQ_HANDLER, __ioctl_set_irq_handler},
    {IOCTL_PAJ7620_SET_EVENT_CALLBACK, __ioctl_set_event_callback},
    {IOCTL_PAJ7620_READ_IRQ_PIN, __ioctl_read_irq_pin},
    {IOCTL_PAJ7620_INTERRUPT_HANDLING, __ioctl_interrupt_handling},
};

/*---------- function ----------*/
static inline bool __write_bytes(paj7620_describe_t *pdesc, uint8_t addr, uint8_t *pdata, uint32_t length)
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

static inline bool __read_bytes(paj7620_describe_t *pdesc, uint8_t addr, uint8_t *pdata, uint32_t length)
{
    i2c_bus_msg_t cont = {
        .type = I2C_BUS_TYPE_RANDOM_READ,
        .dev_addr = pdesc->address,
        .mem_addr = &addr,
        .mem_addr_counts = sizeof(addr),
        .buf = pdata,
        .len = length,
    };

    return (CY_EOK == device_read(pdesc->bus, &cont, 0, sizeof(cont)));
}

static inline bool __write_byte(paj7620_describe_t *pdesc, uint8_t addr, uint8_t byte)
{
    return __write_bytes(pdesc, addr, &byte, sizeof(byte));
}

static inline bool __read_byte(paj7620_describe_t *pdesc, uint8_t addr, uint8_t *byte)
{
    return __read_bytes(pdesc, addr, byte, sizeof(*byte));
}

static inline bool __select_bank0(paj7620_describe_t *pdesc)
{
    return __write_byte(pdesc, REG_BANK_SEL, REG_BANK_SEL_DATA_BANK0);
}

static inline bool __select_bank1(paj7620_describe_t *pdesc)
{
    return __write_byte(pdesc, REG_BANK_SEL, REG_BANK_SEL_DATA_BANK1);
}

static bool _get_device_info(paj7620_describe_t *pdesc)
{
    uint16_t partid = 0;
    uint8_t version = 0;

    __select_bank0(pdesc);
    __read_byte(pdesc, BANK0_REG_PARTID_LOW, (uint8_t *)&partid);
    __read_byte(pdesc, BANK0_REG_PARTID_HIGH, (uint8_t *)&partid + 1);
    __read_byte(pdesc, BANK0_REG_VERSION_ID, &version);
    xlog_tag_info(TAG, "PartID: %04X, Version: %d\n", partid, version);

    return (partid == PAJ7620_PARTID);
}

static inline bool _configure(paj7620_describe_t *pdesc)
{
    bool retval = true;
    const uint8_t *configure_data = pdesc->configure.data;

    while(configure_data[0] != 0x00) {
        retval = __write_byte(pdesc, configure_data[1], configure_data[2]);
        if(retval != true) {
            xlog_tag_error(TAG, "Configure reg(%d) failure\n", configure_data[1]);
            break;
        }
        configure_data += configure_data[0] + 1;
    }

    return retval;
}

static int32_t _open(driver_t **pdrv)
{
    int32_t retval = CY_E_WRONG_ARGS;
    paj7620_describe_t *pdesc = NULL;
    void *bus = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(pdesc == NULL) {
            xlog_tag_error(TAG, "Driver not found any device\n");
            break;
        }
        retval = CY_EOK;
        if(pdesc->ops.init) {
            if(pdesc->ops.init() != true) {
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
        if(_get_device_info(pdesc) != true || _configure(pdesc) != true) {
            if(pdesc->ops.deinit) {
                pdesc->ops.deinit();
            }
            pdesc->bus = NULL;
            retval = CY_ERROR;
            break;
        }
    } while(0);

    return retval;
}

static void _close(driver_t **pdrv)
{
    paj7620_describe_t *pdesc = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t ,pdrv)->pdesc;
    if(pdesc && pdesc->ops.deinit) {
        pdesc->ops.deinit();
    }
}

static int32_t __ioctl_set_irq_handler(paj7620_describe_t *pdesc, void *args)
{
    pdesc->ops.irq_handler = (void *)args;

    return CY_EOK;
}

static int32_t __ioctl_set_event_callback(paj7620_describe_t *pdesc, void *args)
{
    pdesc->ops.on_event = (void *)args;

    return CY_EOK;
}

static int32_t __ioctl_read_irq_pin(paj7620_describe_t *pdesc, void *args)
{
    bool *data = (bool *)args;
    int32_t retval = CY_ERROR;

    if(args && pdesc->ops.get_irq_pin) {
        *data = pdesc->ops.get_irq_pin();
        retval = CY_EOK;
    }

    return retval;
}

static int32_t __ioctl_interrupt_handling(paj7620_describe_t *pdesc, void *args)
{
    paj7620_event_gesture_t event = PAJ7620_EVENT_GESTURE_NONE;
    uint16_t flags = 0;

    __select_bank0(pdesc);
    __read_byte(pdesc, BANK0_REG_INT_FLAG1, (uint8_t *)&flags);
    __read_byte(pdesc, BANK0_REG_INT_FLAG2, (uint8_t *)&flags + 1);
    if(flags & GES_INTFLAG_BIT_LEFT) {
        event = PAJ7620_EVENT_GESTURE_LEFT;
    } else if(flags & GES_INTFLAG_BIT_RIGHT) {
        event = PAJ7620_EVENT_GESTURE_RIGHT;
    } else if(flags & GES_INTFLAG_BIT_DOWM) {
        event = PAJ7620_EVENT_GESTURE_DOWN;
    } else if(flags & GES_INTFLAG_BIT_UP) {
        event = PAJ7620_EVENT_GESTURE_UP;
    } else if(flags & GES_INTFLAG_BIT_FORWARD) {
        event = PAJ7620_EVENT_GESTURE_FORWARD;
    } else if(flags & GES_INTFLAG_BIT_BACKWARD) {
        event = PAJ7620_EVENT_GESTURE_BACKWARD;
    } else if(flags & GES_INTFLAG_BIT_CLOCKWISE) {
        event = PAJ7620_EVENT_GESTURE_CIRCLE_CLOCKWISE;
    } else if(flags & GES_INTFLAG_BIT_COUNT_CLOCKWISE) {
        event = PAJ7620_EVENT_GESTURE_CIRCLE_COUNTER_CLOCKWISE;
    } else if(flags & GES_INTFLAG_BIT_WAVE) {
        event = PAJ7620_EVENT_GESTURE_WAVE;
    }
    if(pdesc->ops.on_event) {
        pdesc->ops.on_event(event);
    }

    return CY_EOK;
}

static int32_t _ioctl(driver_t **pdrv, uint32_t cmd, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    paj7620_describe_t *pdesc = NULL;
    int32_t (*cb)(paj7620_describe_t *, void *);

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(pdesc == NULL) {
            xlog_tag_error(TAG, "Driver not found any describe field\n");
            break;
        }
        cb = protocol_callback_find(cmd, (void *)ioctl_cbs, ARRAY_SIZE(ioctl_cbs));
        if(cb == NULL) {
            xlog_tag_warn(TAG, "Driver not support this ioctl cmd(%08X)\n", cmd);
            break;
        }
        retval = cb(pdesc, args);
    } while(0);

    return retval;
}

static int32_t _irq_handler(driver_t **pdrv, uint32_t irq_handler, void *args, uint32_t length)
{
    paj7620_describe_t *pdesc = NULL;
    int32_t retval = CY_E_WRONG_ARGS;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc && pdesc->ops.irq_handler) {
        retval = pdesc->ops.irq_handler(irq_handler, args, length);
    }

    return retval;
}
